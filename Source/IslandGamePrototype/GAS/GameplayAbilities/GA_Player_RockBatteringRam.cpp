#include "GA_Player_RockBatteringRam.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "IslandGamePrototype/Characters/IGCharacterBase.h"
#include "IslandGamePrototype/GameplayMessages.h"
#include "IslandGamePrototype/GameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/UnrealType.h"

namespace
{
	// Compatibility with the existing player Blueprint's input, animation enum, and
	// per-attribute upgrade struct. No Blueprint ability functions are invoked.
	FNumericProperty* FindNumber(const UStruct* Type, FName Name)
	{
		for (TFieldIterator<FNumericProperty> It(Type); It; ++It)
		{
			if (It->GetFName() == Name || It->GetName().StartsWith(Name.ToString() + TEXT("_")))
			{
				return *It;
			}
		}
		return nullptr;
	}

	float ReadNumber(const UObject* Object, FName Name)
	{
		if (Object)
		{
			if (const FNumericProperty* Property = FindNumber(Object->GetClass(), Name))
			{
				const void* Value = Property->ContainerPtrToValuePtr<void>(Object);
				return Property->IsFloatingPoint() ? Property->GetFloatingPointPropertyValue(Value) : Property->GetSignedIntPropertyValue(Value);
			}
		}
		return 0.f;
	}

	void DestroyParticles(TObjectPtr<UNiagaraComponent>& Component)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
		Component = nullptr;
	}
}

UGA_Player_RockBatteringRam::UGA_Player_RockBatteringRam()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	SetAssetTags(FGameplayTagContainer(IslandGameplayTags::Ability_Skill_RockBatteringRam));
	ActivationOwnedTags.AddTag(IslandGameplayTags::Ability_Skill_RockBatteringRam);
	CooldownTags.AddTag(IslandGameplayTags::Cooldown_RockBatteringRam);
}

bool UGA_Player_RockBatteringRam::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
													 const FGameplayAbilityActorInfo* ActorInfo,
													 const FGameplayTagContainer* SourceTags,
													 const FGameplayTagContainer* TargetTags,
													 FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}
	
	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character || !Character->GetCharacterMovement() || !Character->GetCapsuleComponent())
	{
		return false;
	}
	
	FHitResult Hit;
	return !Character->GetCharacterMovement()->IsFalling()
		|| (TraceGround(Character, Hit) && Hit.Distance > MinimumHeightToUseAirSkill);
}

void UGA_Player_RockBatteringRam::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
												  const FGameplayAbilityActorInfo* ActorInfo,
												  const FGameplayAbilityActivationInfo ActivationInfo,
												  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	Player = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	bEnding = false;
	bStarted = false;
	bGroundActive = false;
	bAirActive = false;
	Elapsed = 0.f;
	AirHitCharacters.Reset();
	
	if (!Player || !Player->GetCharacterMovement())
	{
		FinishRam(true);
		return;
	}
	
	FHitResult Ground;
	const bool bAir = Player->GetCharacterMovement()->IsFalling();
	
	if (bAir && (!TraceGround(Player, Ground) || Ground.Distance <= MinimumHeightToUseAirSkill))
	{
		FinishRam(true);
		return;
	}
	
	if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
	{
		FinishRam(true);
		return;
	}

	OriginalMaxWalkSpeed = Player->GetCharacterMovement()->MaxWalkSpeed;
	OriginalMaxAcceleration = Player->GetCharacterMovement()->MaxAcceleration;
	OriginalPawnResponse = Player->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn);
	bOriginalControllerYaw = Player->bUseControllerRotationYaw;
	OriginalAnimationState = ReadNumber(Player, TEXT("E_PlayerAnimationState"));
	Camera = Player->FindComponentByClass<UCameraComponent>();
	OriginalFOV = Camera ? Camera->FieldOfView : 0.f;
	bStarted = true;

	// Listen on this avatar's ASC. The routed event tag selects the ability;
	// InstigatorTags carries the command, as SendGameplayEventToActor replaces EventTag.
	EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this,
		IslandGameplayTags::Ability_Skill_RockBatteringRam, Player, false, true);
	EventTask->EventReceived.AddDynamic(this, &ThisClass::OnRamEvent);
	EventTask->ReadyForActivation();
	
	if (bAir)
	{
		StartAir(Ground.Distance);
	}
	else
	{
		StartGround();
	}
	
	if (IsActive() && !bEnding)
	{
		UAbilityTask_RockBatteringRamTick::Create(this)->ReadyForActivation();
	}
}

void UGA_Player_RockBatteringRam::EndAbility(const FGameplayAbilitySpecHandle Handle,
											 const FGameplayAbilityActorInfo* ActorInfo,
											 const FGameplayAbilityActivationInfo ActivationInfo,
											 bool bReplicateEndAbility,
											 bool bWasCancelled)
{
	if (bEnding || !IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility,
			Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
		return;
	}
	bEnding = true;
	const bool bWasGround = bGroundActive;
	bGroundActive = false;
	bAirActive = false;
	if (EventTask)
	{
		EventTask->EndTask();
		EventTask = nullptr;
	}
	if (bStarted)
	{
		if (IsValid(Player))
		{
			Player->LandedDelegate.RemoveDynamic(this, &ThisClass::OnLanded);
			UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
			Capsule->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::OnCapsuleOverlap);
			Capsule->OnComponentHit.RemoveDynamic(this, &ThisClass::OnCapsuleHit);
			Capsule->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
			if (bWasGround)
			{
				Player->GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
				Player->GetCharacterMovement()->MaxAcceleration = OriginalMaxAcceleration;
				Player->bUseControllerRotationYaw = bOriginalControllerYaw;
				SetAnimationState(OriginalAnimationState);
			}
			else if (bWasCancelled)
			{
				Player->GetCharacterMovement()->ClearAccumulatedForces();
				Player->GetCharacterMovement()->StopMovementImmediately();
			}
			if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
			{
				if (PC->PlayerCameraManager && ActiveCameraShake)
				{
					PC->PlayerCameraManager->StopCameraShake(ActiveCameraShake);
				}
			}
		}
		if (Camera && bWasGround)
		{
			Camera->SetFieldOfView(OriginalFOV);
		}
		if (BuffHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(BuffHandle);
		}
		BuffHandle.Invalidate();
		DestroyParticles(WindParticles);
		DestroyParticles(RockParticles);
		CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, false);
		if (IsLocallyControlled() && UGameplayMessageSubsystem::HasInstance(this))
		{
			FFloatMessage Message;
			Message.Value = Evaluate(CooldownDuration, TEXT("Cooldown"));
			UGameplayMessageSubsystem::Get(this).BroadcastMessage(IslandGameplayTags::Cooldown_RockBatteringRam, Message);
		}
	}
	bStarted = false;
	ActiveCameraShake = nullptr;
	Camera = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	Player = nullptr;
}

void UGA_Player_RockBatteringRam::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
												const FGameplayAbilityActorInfo* ActorInfo,
												const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!CooldownGameplayEffectClass)
	{
		return;
	}
	const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, GetAbilityLevel());
	if (Spec.IsValid())
	{
		Spec.Data->DynamicGrantedTags.AppendTags(CooldownTags);
		Spec.Data->SetDuration(FMath::Max(0.f, Evaluate(CooldownDuration, TEXT("Cooldown"))), true);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}

void UGA_Player_RockBatteringRam::TickRam(float DeltaTime)
{
	if (!IsActive() || bEnding)
	{
		return;
	}
	if (!IsValid(Player) || !Player->GetCharacterMovement())
	{
		FinishRam(true);
		return;
	}
	Elapsed += DeltaTime;
	if (Elapsed >= Duration)
	{
		FinishRam(bAirActive);
		return;
	}
	if (bGroundActive)
	{
		const float Charge = FMath::Clamp(Evaluate(ChargeTime, TEXT("GroundChargeTime")), 0.f, Duration);
		const float Alpha = FMath::Clamp(Elapsed / FMath::Max(KINDA_SMALL_NUMBER, Charge > 0.f ? Charge : Duration), 0.f, 1.f);
		const float Ramp = SpeedCurve ? FMath::Clamp(SpeedCurve->GetFloatValue(Alpha), 0.f, 1.f) : Alpha;
		Player->GetCharacterMovement()->MaxWalkSpeed = FMath::Lerp(OriginalMaxWalkSpeed, SkillMaxWalkSpeed, Ramp);
		Player->AddActorWorldRotation(FRotator(0.f, FMath::Clamp(ReadNumber(Player, TEXT("MoveActionValueX")), -1.f, 1.f) * TurnSpeed * DeltaTime, 0.f));
		Player->AddMovementInput(Player->GetActorForwardVector(), 1.f);
		if (IsLocallyControlled() && Camera)
		{
			Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, GroundFOV, DeltaTime, FOVInterpSpeed));
		}
	}
}

void UGA_Player_RockBatteringRam::StartGround()
{
	bGroundActive = true;
	Duration = FMath::Max(KINDA_SMALL_NUMBER, Evaluate(MaxDuration, TEXT("GroundMaxDuration")));
	GroundDamageRate = FMath::Max(0.f, BaseGroundDamagePerSecond)
		* FMath::Max(0.f, Evaluate(GroundDamageMultiplier, TEXT("GroundDamageMultiplier")));
	GroundKnockback = Evaluate(BaseGroundKnockback, TEXT("GroundKnockback"));
	TurnSpeed = Evaluate(DeviationVelocity, TEXT("GroundDeviationVelocity"));
	Player->GetCharacterMovement()->MaxAcceleration = SkillMaxAcceleration;
	Player->bUseControllerRotationYaw = false;
	SetAnimationState(3);
	UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	Capsule->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnCapsuleOverlap);
	Capsule->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnCapsuleHit);
	if (GroundBuffEffect)
	{
		BuffHandle = ApplyGameplayEffectToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
			GroundBuffEffect.GetDefaultObject(), GetAbilityLevel());
	}
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			if (PC->PlayerCameraManager && GroundCameraShake)
			{
				ActiveCameraShake = PC->PlayerCameraManager->StartCameraShake(GroundCameraShake);
			}
		}
		if (GroundWindSystem)
		{
			WindParticles = UNiagaraFunctionLibrary::SpawnSystemAttached(GroundWindSystem, Capsule, NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		}
		if (HandRockSystem)
		{
			RockParticles = UNiagaraFunctionLibrary::SpawnSystemAttached(HandRockSystem, Player->GetMesh(), HandRockSocket,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		}
	}
	// Changing collision responses may synchronously dispatch existing overlaps.
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void UGA_Player_RockBatteringRam::StartAir(float Height)
{
	bAirActive = true;
	Duration = FMath::Max(.1f, MaxAirDuration);
	const float Meters = Height / 100.f;
	AirDamage = FMath::Clamp(Meters * Evaluate(AirDamagePerMeter, TEXT("AirDamagePerMeter")), MinAirImpactDamage, MaxAirImpactDamage);
	AirRadius = FMath::Clamp(Meters * Evaluate(AirImpactRadiusPerMeter, TEXT("AirImpactRadiusPerMeter")), MinAirImpactRadius, MaxAirImpactRadius);
	AirKnockback = FMath::Clamp(Meters * Evaluate(AirKnockbackPerMeter, TEXT("AirKnockbackPerMeter")), MinAirKnockback, MaxAirKnockback);
	Player->LandedDelegate.AddUniqueDynamic(this, &ThisClass::OnLanded);
	Player->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	if (IsLocallyControlled() && AirWindSystem)
	{
		WindParticles = UNiagaraFunctionLibrary::SpawnSystemAttached(AirWindSystem, Player->GetCapsuleComponent(), NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
	}
	Player->GetCharacterMovement()->StopMovementImmediately();
	Player->LaunchCharacter(FVector(0.f, 0.f, -DiveSpeed), true, true);
}

void UGA_Player_RockBatteringRam::FinishRam(bool bCancelled)
{
	if (IsActive() && !bEnding)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bCancelled);
	}
}

bool UGA_Player_RockBatteringRam::TraceGround(ACharacter* Character, FHitResult& Hit) const
{
	const FVector Start = Character->GetMesh() ? Character->GetMesh()->GetComponentLocation() : Character->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RockBatteringRamGround), false, Character);
	return Character->GetWorld()->LineTraceSingleByChannel(Hit, Start,
		Start - FVector::UpVector * GroundTraceDistance, GroundTraceChannel, Params);
}

float UGA_Player_RockBatteringRam::Evaluate(const FScalableFloat& Value, FName UpgradeAttribute) const
{
	float Level = GetAbilityLevel();
	if (Player)
	{
		if (const FStructProperty* Levels = FindFProperty<FStructProperty>(Player->GetClass(), TEXT("RockBatteringRamAttributeLevel")))
		{
			if (const FNumericProperty* Attribute = FindNumber(Levels->Struct, UpgradeAttribute))
			{
				Level = Attribute->GetSignedIntPropertyValue(Attribute->ContainerPtrToValuePtr<void>(Levels->ContainerPtrToValuePtr<void>(Player)));
			}
		}
	}
	return Value.GetValueAtLevel(Level);
}

FVector UGA_Player_RockBatteringRam::GetKnockback(ACharacter* Target, float Strength, bool bGround) const
{
	FVector Direction = (Target->GetActorLocation() - Player->GetActorLocation()).GetSafeNormal2D();
	if (bGround)
	{
		const FVector Forward = Player->GetActorForwardVector().GetSafeNormal2D();
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, Direction), -1.f, 1.f)));
		if (Angle < KnockbackDeflectionAngle)
		{
			const float Side = FVector::CrossProduct(Forward, Direction).Z < 0.f ? -1.f : 1.f;
			Direction += Player->GetActorRightVector() * Side;
		}
	}
	return FVector(Direction.X * Strength, Direction.Y * Strength, bGround ? Strength : Strength * .5f);
}

void UGA_Player_RockBatteringRam::ImpactCharacter(ACharacter* Target, float Damage, float Knockback, bool bGround)
{
	if (!IsValid(Target) || Target == Player || !HasAuthority(&CurrentActivationInfo))
	{
		return;
	}
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC || (bGround && HitImmunityTag.IsValid() && TargetASC->HasMatchingGameplayTag(HitImmunityTag)))
	{
		return;
	}
	if (bGround && HitImmunityEffect)
	{
		const FGameplayEffectSpecHandle Immunity = MakeOutgoingGameplayEffectSpec(HitImmunityEffect, GetAbilityLevel());
		if (Immunity.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*Immunity.Data.Get());
		}
	}
	if (UCharacterMovementComponent* Movement = Target->GetCharacterMovement())
	{
		if (!bGround || !Movement->IsFalling())
		{
			Movement->StopMovementImmediately();
			Target->LaunchCharacter(GetKnockback(Target, Knockback, bGround), true, true);
		}
	}
	const AIGCharacterBase* TargetCharacter = Cast<AIGCharacterBase>(Target);
	if (LastHitEffect && TargetCharacter && Damage >= TargetCharacter->GetHealth())
	{
		const FGameplayEffectSpecHandle LastHit = MakeOutgoingGameplayEffectSpec(LastHitEffect, GetAbilityLevel());
		if (LastHit.IsValid())
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*LastHit.Data.Get());
		}
	}
	if (DamageEffect && DamageSetByCallerTag.IsValid())
	{
		const FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(DamageSetByCallerTag, FMath::Max(0.f, Damage));
			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void UGA_Player_RockBatteringRam::PlayImpact(const FVector& Location, bool bGround)
{
	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (UParticleSystem* Particles = bGround ? GroundImpactParticles : AirImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Particles, Location);
	}
	if (IsLocallyControlled())
	{
		if (USoundBase* Sound = bGround ? GroundImpactSound : AirImpactSound)
		{
			UGameplayStatics::PlaySound2D(this, Sound);
		}
		if (APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr)
		{
			const TSubclassOf<UCameraShakeBase> Shake = bGround ? GroundImpactCameraShake : AirImpactCameraShake;
			if (PC->PlayerCameraManager && Shake)
			{
				PC->PlayerCameraManager->StartCameraShake(Shake);
			}
		}
	}
}

void UGA_Player_RockBatteringRam::SetAnimationState(int64 State)
{
	if (Player)
	{
		if (FNumericProperty* Property = FindNumber(Player->GetClass(), TEXT("E_PlayerAnimationState")))
		{
			Property->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Player), State);
		}
	}
}

void UGA_Player_RockBatteringRam::OnRamEvent(FGameplayEventData Payload)
{
	if (Payload.InstigatorTags.HasTagExact(IslandGameplayTags::Ability_Skill_Cancel))
	{
		FinishRam(true);
	}
}

void UGA_Player_RockBatteringRam::OnLanded(const FHitResult& Hit)
{
	if (!bAirActive || bEnding || !Player)
	{
		return;
	}
	// Capsule collisions and overlap queries can report several components per actor.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RockBatteringRamImpact), false, Player);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Player->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(AirRadius), Params);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACharacter* Target = Cast<ACharacter>(Overlap.GetActor());
		if (Target && !AirHitCharacters.Contains(Target))
		{
			AirHitCharacters.Add(Target);
			ImpactCharacter(Target, AirDamage, AirKnockback, false);
		}
	}
	PlayImpact(Hit.ImpactPoint, false);
	FinishRam();
}

void UGA_Player_RockBatteringRam::OnCapsuleOverlap(UPrimitiveComponent* OverlappedComponent,
												   AActor* OtherActor,
												   UPrimitiveComponent* OtherComp,
												   int32 OtherBodyIndex,
												   bool bFromSweep,
												   const FHitResult& SweepResult)
{
	if (!bGroundActive || bEnding || !Cast<UCapsuleComponent>(OtherComp))
	{
		return;
	}
	ACharacter* Target = Cast<ACharacter>(OtherActor);
	if (!Target || Target == Player)
	{
		return;
	}
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	// Immunity gates the entire impact, including its particles, sound, and camera shake.
	if (!TargetASC || (HitImmunityTag.IsValid() && TargetASC->HasMatchingGameplayTag(HitImmunityTag)))
	{
		return;
	}
	ImpactCharacter(Target, Elapsed * GroundDamageRate, GroundKnockback, true);
	PlayImpact(bFromSweep ? FVector(SweepResult.ImpactPoint) : Target->GetActorLocation(), true);
}

void UGA_Player_RockBatteringRam::OnCapsuleHit(UPrimitiveComponent* HitComponent,
											   AActor* OtherActor,
											   UPrimitiveComponent* OtherComp,
											   FVector NormalImpulse,
											   const FHitResult& Hit)
{
	if (bGroundActive && !bEnding)
	{
		const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector), -1.f, 1.f)));
		if (Angle > StopAngle)
		{
			PlayImpact(Hit.ImpactPoint, true);
			FinishRam();
		}
	}
}


UAbilityTask_RockBatteringRamTick::UAbilityTask_RockBatteringRamTick()
{
	bTickingTask = true;
}

UAbilityTask_RockBatteringRamTick* UAbilityTask_RockBatteringRamTick::Create(UGA_Player_RockBatteringRam* OwningAbility)
{
	return NewAbilityTask<UAbilityTask_RockBatteringRamTick>(OwningAbility);
}

void UAbilityTask_RockBatteringRamTick::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	if (UGA_Player_RockBatteringRam* Ram = Cast<UGA_Player_RockBatteringRam>(Ability))
	{
		Ram->TickRam(DeltaTime);
	}
}
