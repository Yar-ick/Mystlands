#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "IslandGamePrototype/Characters/IGCharacterBase.h"
#include "IslandGamePrototype/GameplayTags.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/UObjectIterator.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRockBatteringRamLifecycleTest, "Mystlands.Abilities.RockBatteringRam.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRockBatteringRamLifecycleTest::RunTest(const FString& Parameters)
{
	UClass* RamClass = LoadClass<UGameplayAbility>(nullptr,
		TEXT("/Game/GAS/Player/Skills/RockBatteringRam/GA_Player_RockBatteringRam.GA_Player_RockBatteringRam_C"));
	if (!TestNotNull(TEXT("Load the migrated ability Blueprint"), RamClass))
	{
		return false;
	}
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Initialization);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	AIGCharacterBase* Player = World->SpawnActor<AIGCharacterBase>();
	AIGCharacterBase* Target = World->SpawnActor<AIGCharacterBase>(FVector(100.f, 0.f, 100.f), FRotator::ZeroRotator);
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	ASC->AddAttributeSetSubobject(Player->AttributeSet.Get());
	TargetASC->AddAttributeSetSubobject(Target->AttributeSet.Get());
	ASC->InitAbilityActorInfo(Player, Player);
	TargetASC->InitAbilityActorInfo(Target, Target);
	TargetASC->SetNumericAttributeBase(UIGAttributeSet::GetMaxHealthAttribute(), 500.f);
	TargetASC->SetNumericAttributeBase(UIGAttributeSet::GetHealthAttribute(), 500.f);
	TargetASC->SetNumericAttributeBase(UIGAttributeSet::GetDamageResistanceAttribute(), 0.f);
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(RamClass, 1));
	const FGameplayTagContainer CooldownTags(IslandGameplayTags::Cooldown_RockBatteringRam);
	const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
	auto IsActive = [&]() { return ASC->FindAbilitySpecFromHandle(Handle)->IsActive(); };
	auto ResetCooldown = [&]() { ASC->RemoveActiveEffects(CooldownQuery); };
	auto Cancel = [&]()
	{
		FGameplayEventData Payload;
		Payload.InstigatorTags.AddTag(IslandGameplayTags::Ability_Skill_Cancel);
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Player, IslandGameplayTags::Ability_Skill_RockBatteringRam, Payload);
	};
	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Walking);
	Movement->MaxWalkSpeed = 612.f;
	Movement->MaxAcceleration = 2048.f;
	Player->bUseControllerRotationYaw = true;
	Player->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	TestTrue(TEXT("Ground ability activates through GAS"), ASC->TryActivateAbility(Handle));
	TestTrue(TEXT("Ground ability is active"), IsActive());
	TestEqual(TEXT("Ground mode overlaps pawns"), Player->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Overlap);
	ASC->TickComponent(.5f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Native task ramps movement speed"), Movement->MaxWalkSpeed > 612.f);
	FHitResult Hit;
	UParticleSystem* GroundImpactTemplate = LoadObject<UParticleSystem>(nullptr,
		TEXT("/Game/VFX/ParticleSystems/Player/Rock/BatteringRam/PS_Player_RockBatteringRam_ObstacleCrash.PS_Player_RockBatteringRam_ObstacleCrash"));
	auto CountGroundImpacts = [&]()
	{
		int32 Count = 0;
		for (TObjectIterator<UParticleSystemComponent> It; It; ++It)
		{
			if (It->GetWorld() == World && It->Template == GroundImpactTemplate)
			{
				++Count;
			}
		}
		return Count;
	};
	const int32 ImpactsBeforeHit = CountGroundImpacts();
	Player->GetCapsuleComponent()->OnComponentBeginOverlap.Broadcast(Player->GetCapsuleComponent(), Target,
		Target->GetCapsuleComponent(), 0, false, Hit);
	TestTrue(TEXT("Ground impact deals damage"), Target->GetHealth() < 500.f);
	if (FApp::CanEverRender())
	{
		TestEqual(TEXT("An accepted enemy hit spawns one impact effect"), CountGroundImpacts(), ImpactsBeforeHit + 1);
	}
	const float HealthAfterHit = Target->GetHealth();
	for (int32 Repeat = 0; Repeat < 12; ++Repeat)
	{
		Player->GetCapsuleComponent()->OnComponentBeginOverlap.Broadcast(Player->GetCapsuleComponent(), Target,
			Target->GetCapsuleComponent(), 0, false, Hit);
	}
	TestEqual(TEXT("Hit immunity prevents duplicate ground damage"), Target->GetHealth(), HealthAfterHit);
	if (FApp::CanEverRender())
	{
		TestEqual(TEXT("Hit immunity also prevents repeated impact particles and sound dispatch"), CountGroundImpacts(), ImpactsBeforeHit + 1);
	}
	TargetASC->RemoveActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
		FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Buff.RockBatteringRamImmunity")))));
	Player->GetCapsuleComponent()->OnComponentBeginOverlap.Broadcast(Player->GetCapsuleComponent(), Target,
		Target->GetCapsuleComponent(), 0, false, Hit);
	TestTrue(TEXT("Enemy can be hit again after immunity ends"), Target->GetHealth() < HealthAfterHit);
	if (FApp::CanEverRender())
	{
		TestEqual(TEXT("A new accepted hit plays another impact"), CountGroundImpacts(), ImpactsBeforeHit + 2);
	}
	FGameplayEventData Empty;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Player, IslandGameplayTags::Ability_Skill_RockBatteringRam, Empty);
	TestTrue(TEXT("An event without the cancel command does not stop the ability"), IsActive());
	{
		FScopedTargetListLock Lock(*ASC, *ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
		Cancel();
		TestTrue(TEXT("Cancel waits for the GAS target-list lock"), IsActive());
	}
	TestFalse(TEXT("Cancel gameplay event ends ground rush"), IsActive());
	TestEqual(TEXT("Restore original speed"), Movement->MaxWalkSpeed, 612.f);
	TestEqual(TEXT("Restore original acceleration"), Movement->MaxAcceleration, 2048.f);
	TestTrue(TEXT("Restore controller rotation"), Player->bUseControllerRotationYaw);
	TestEqual(TEXT("Restore original pawn response"), Player->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
	TestEqual(TEXT("Apply one cooldown on end"), ASC->GetActiveEffects(CooldownQuery).Num(), 1);
	Cancel();
	TestEqual(TEXT("Late cancel does not add a second cooldown"), ASC->GetActiveEffects(CooldownQuery).Num(), 1);
	TestTrue(TEXT("Cooldown can be queried without an ability instance"),
		UIGGameplayAbility::GetCooldownTimeRemainingForActor(Player, IslandGameplayTags::Cooldown_RockBatteringRam) > 0.f);
	ResetCooldown();
	TestTrue(TEXT("Ground ability can reactivate"), ASC->TryActivateAbility(Handle));
	Hit.ImpactNormal = FVector::ForwardVector;
	Player->GetCapsuleComponent()->OnComponentHit.Broadcast(Player->GetCapsuleComponent(), nullptr, nullptr, FVector::ZeroVector, Hit);
	TestFalse(TEXT("A steep obstacle ends the ground rush"), IsActive());
	ResetCooldown();
	TestTrue(TEXT("Ground ability starts again for duration test"), ASC->TryActivateAbility(Handle));
	ASC->TickComponent(100.f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Maximum duration ends the rush"), IsActive());
	ResetCooldown();

	AActor* Floor = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
	Floor->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(2000.f, 2000.f, 10.f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponent();
	Floor->SetActorLocation(FVector(0.f, 0.f, -10.f));
	Player->SetActorLocation(FVector(0.f, 0.f, 100.f));
	Movement->SetMovementMode(MOVE_Falling);
	TestFalse(TEXT("Air activation below minimum height is rejected"), ASC->TryActivateAbility(Handle));
	TestEqual(TEXT("Rejected air activation does not start cooldown"), ASC->GetActiveEffects(CooldownQuery).Num(), 0);
	Player->SetActorLocation(FVector(0.f, 0.f, 1000.f));
	TestTrue(TEXT("Air ability activates above the minimum height"), ASC->TryActivateAbility(Handle));
	Cancel();
	TestFalse(TEXT("Cancel gameplay event ends air dive too"), IsActive());
	TestTrue(TEXT("Air cancellation clears pending dive launch"), Movement->PendingLaunchVelocity.IsNearlyZero());
	TestEqual(TEXT("Air cancellation restores pawn collision"), Player->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
	ResetCooldown();
	TestTrue(TEXT("Air ability can reactivate"), ASC->TryActivateAbility(Handle));
	Player->SetActorLocation(FVector(0.f, 0.f, 100.f));
	Target->SetActorLocation(FVector(100.f, 0.f, 100.f));
	const float BeforeLanding = Target->GetHealth();
	Player->LandedDelegate.Broadcast(Hit);
	TestFalse(TEXT("Landing ends the air dive"), IsActive());
	TestTrue(TEXT("Landing applies radial damage"), Target->GetHealth() < BeforeLanding);
	const float AfterLanding = Target->GetHealth();
	Player->LandedDelegate.Broadcast(Hit);
	TestEqual(TEXT("Landing delegate is removed after end"), Target->GetHealth(), AfterLanding);
	TestEqual(TEXT("Landing commits a single cooldown"), ASC->GetActiveEffects(CooldownQuery).Num(), 1);
	ResetCooldown();
	Player->SetActorLocation(FVector(0.f, 0.f, 1000.f));
	Movement->SetMovementMode(MOVE_Falling);
	TestTrue(TEXT("Air ability starts again for timeout test"), ASC->TryActivateAbility(Handle));
	ASC->TickComponent(100.f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("A dive without a landing times out"), IsActive());
	TestTrue(TEXT("Air timeout clears pending launch"), Movement->PendingLaunchVelocity.IsNearlyZero());

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRockBatteringRamBlueprintIntegrationTest,
	"Mystlands.Abilities.RockBatteringRam.BlueprintIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRockBatteringRamBlueprintIntegrationTest::RunTest(const FString& Parameters)
{
	UClass* PlayerClass = LoadClass<AIGCharacterBase>(nullptr, TEXT("/Game/Blueprints/BP_Player.BP_Player_C"));
	UClass* RamClass = LoadClass<UGameplayAbility>(nullptr,
		TEXT("/Game/GAS/Player/Skills/RockBatteringRam/GA_Player_RockBatteringRam.GA_Player_RockBatteringRam_C"));
	UClass* SkillClass = LoadClass<UActorComponent>(nullptr,
		TEXT("/Game/Blueprints/AC_Player_SkillSystem.AC_Player_SkillSystem_C"));
	if (!TestNotNull(TEXT("Player Blueprint"), PlayerClass) || !TestNotNull(TEXT("Ability Blueprint"), RamClass)
		|| !TestNotNull(TEXT("Skill-system Blueprint"), SkillClass))
	{
		return false;
	}
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Initialization);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	AIGCharacterBase* Player = World->SpawnActor<AIGCharacterBase>(PlayerClass);
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	ASC->AddAttributeSetSubobject(Player->AttributeSet.Get());
	ASC->InitAbilityActorInfo(Player, Player);
	Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(RamClass, 1));
	const FByteProperty* Animation = FindFProperty<FByteProperty>(PlayerClass, TEXT("E_PlayerAnimationState"));
	const uint8 OriginalState = Animation->GetPropertyValue_InContainer(Player);
	UActorComponent* SkillSystem = Player->GetComponentByClass(SkillClass);
	if (TestNotNull(TEXT("Player has its skill-system component"), SkillSystem))
	{
		FObjectProperty* PlayerProperty = FindFProperty<FObjectProperty>(SkillClass, TEXT("Player"));
		PlayerProperty->SetObjectPropertyValue_InContainer(SkillSystem, Player);
		TestTrue(TEXT("Activate ram on the actual player Blueprint"), ASC->TryActivateAbility(Handle));
		TestEqual(TEXT("Native ability selects the running animation state"), Animation->GetPropertyValue_InContainer(Player), uint8(3));
		UFunction* Move = Player->FindFunction(TEXT("HandleMovement"));
		FStructOnScope MoveParameters(Move);
		FNumericProperty* Horizontal = FindFProperty<FNumericProperty>(Move, TEXT("ActionValueX"));
		Horizontal->SetFloatingPointPropertyValue(Horizontal->ContainerPtrToValuePtr<void>(MoveParameters.GetStructMemory()), .5);
		{
			// The fixture deliberately skips BeginPlay to avoid starting HUD and save-game systems.
			FEditorScriptExecutionGuard ScriptGuard;
			Player->ProcessEvent(Move, MoveParameters.GetStructMemory());
		}
		const FNumericProperty* Input = FindFProperty<FNumericProperty>(PlayerClass, TEXT("MoveActionValueX"));
		const double SteeringInput = Input->GetFloatingPointPropertyValue(Input->ContainerPtrToValuePtr<void>(Player));
		TestEqual(TEXT("Player movement graph stores horizontal input"), SteeringInput, .5);
		const float BeforeYaw = Player->GetActorRotation().Yaw;
		ASC->TickComponent(.1f, LEVELTICK_All, nullptr);
		AddInfo(FString::Printf(TEXT("Steering input %.3f, yaw %.3f -> %.3f, active %d, speed %.3f"), SteeringInput,
			BeforeYaw, Player->GetActorRotation().Yaw, ASC->FindAbilitySpecFromHandle(Handle)->IsActive(), Player->GetCharacterMovement()->MaxWalkSpeed));
		TestTrue(TEXT("Native steering consumes the player's Blueprint input"), Player->GetActorRotation().Yaw > BeforeYaw);
		UClass* EnemyClass = LoadClass<AIGCharacterBase>(nullptr,
			TEXT("/Game/AI/Characters/Rock/BP_RockMeleeEnemy.BP_RockMeleeEnemy_C"));
		if (TestNotNull(TEXT("Rock melee enemy Blueprint"), EnemyClass))
		{
			AIGCharacterBase* Enemy = World->SpawnActor<AIGCharacterBase>(EnemyClass, FVector(1000.f, 0.f, 100.f), FRotator::ZeroRotator);
			UAbilitySystemComponent* EnemyASC = Enemy->GetAbilitySystemComponent();
			EnemyASC->AddAttributeSetSubobject(Enemy->AttributeSet.Get());
			EnemyASC->InitAbilityActorInfo(Enemy, Enemy);
			EnemyASC->SetNumericAttributeBase(UIGAttributeSet::GetMaxHealthAttribute(), 500.f);
			EnemyASC->SetNumericAttributeBase(UIGAttributeSet::GetHealthAttribute(), 500.f);
			EnemyASC->SetNumericAttributeBase(UIGAttributeSet::GetDamageResistanceAttribute(), 0.f);
			FHitResult Hit;
			Player->GetCapsuleComponent()->OnComponentBeginOverlap.Broadcast(Player->GetCapsuleComponent(), Enemy,
				Enemy->GetCapsuleComponent(), 0, false, Hit);
			TestEqual(TEXT("Ground hit preserves the original 15 damage per second before upgrade multipliers"), Enemy->GetHealth(), 498.5f);
			EnemyASC->SetNumericAttributeBase(UIGAttributeSet::GetDamageResistanceAttribute(), .25f);
			EnemyASC->RemoveActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
				FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Buff.RockBatteringRamImmunity")))));
			ASC->TickComponent(.4f, LEVELTICK_All, nullptr);
			Player->GetCapsuleComponent()->OnComponentBeginOverlap.Broadcast(Player->GetCapsuleComponent(), Enemy,
				Enemy->GetCapsuleComponent(), 0, false, Hit);
			TestEqual(TEXT("Charged ground damage still respects enemy damage resistance"), Enemy->GetHealth(), 492.875f);
		}
		UFunction* Cancel = SkillSystem->FindFunction(TEXT("CancelAbility"));
		FStructOnScope CancelParameters(Cancel);
		SkillSystem->ProcessEvent(Cancel, CancelParameters.GetStructMemory());
		TestFalse(TEXT("Skill-system Blueprint cancels ram through its gameplay event"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
		TestEqual(TEXT("Native cleanup restores the player's animation state"), Animation->GetPropertyValue_InContainer(Player), OriginalState);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRockBatteringRamAnimationTimeoutTest,
	"Mystlands.Abilities.RockBatteringRam.AnimationTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRockBatteringRamAnimationTimeoutTest::RunTest(const FString& Parameters)
{
	UClass* PlayerClass = LoadClass<AIGCharacterBase>(nullptr, TEXT("/Game/Blueprints/BP_Player.BP_Player_C"));
	UClass* RamClass = LoadClass<UGameplayAbility>(nullptr,
		TEXT("/Game/GAS/Player/Skills/RockBatteringRam/GA_Player_RockBatteringRam.GA_Player_RockBatteringRam_C"));
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).RequiresHitProxies(false).CreateNavigation(false).CreateAISystem(false)
		.ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Initialization);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	AIGCharacterBase* Player = World->SpawnActor<AIGCharacterBase>(PlayerClass);
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	ASC->AddAttributeSetSubobject(Player->AttributeSet.Get());
	ASC->InitAbilityActorInfo(Player, Player);
	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Walking);
	USkeletalMeshComponent* Mesh = Player->GetMesh();
	Mesh->bEnableUpdateRateOptimizations = false;
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh->InitAnim(true);
	UAnimInstance* Anim = Mesh->GetAnimInstance();
	if (TestNotNull(TEXT("Player animation instance"), Anim))
	{
		FindFProperty<FObjectProperty>(Anim->GetClass(), TEXT("Player"))->SetObjectPropertyValue_InContainer(Anim, Player);
		FVector* Acceleration = FindFProperty<FStructProperty>(Movement->GetClass(), TEXT("Acceleration"))->ContainerPtrToValuePtr<FVector>(Movement);
		Movement->Velocity = FVector(500.f, 0.f, 0.f);
		*Acceleration = FVector(500.f, 0.f, 0.f);
		const int32 MachineIndex = Anim->GetStateMachineIndex(TEXT("Locomotion"));
		auto TickAnimation = [&]()
		{
			FEditorScriptExecutionGuard ScriptGuard;
			for (int32 Frame = 0; Frame < 30; ++Frame)
			{
				Mesh->TickAnimation(1.f / 60.f, false);
				Mesh->RefreshBoneTransforms();
			}
		};
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(RamClass, 1));
		TestTrue(TEXT("Activate ram for collision-free timeout"), ASC->TryActivateAbility(Handle));
		TickAnimation();
		TestEqual(TEXT("Animation enters the ram running state"), Anim->GetCurrentStateName(MachineIndex), FName(TEXT("RockBatteringRamRunning")));
		// Reproduce an uninterrupted rush that expires as the player stops moving.
		Movement->Velocity = FVector::ZeroVector;
		*Acceleration = FVector::ZeroVector;
		ASC->TickComponent(100.f, LEVELTICK_All, nullptr);
		TestFalse(TEXT("Ram ability ends at maximum duration"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
		const FByteProperty* Animation = FindFProperty<FByteProperty>(PlayerClass, TEXT("E_PlayerAnimationState"));
		TestEqual(TEXT("Timeout restores the player's combat animation enum"), Animation->GetPropertyValue_InContainer(Player), uint8(0));
		TickAnimation();
		TestEqual(TEXT("Collision-free timeout returns directly from ram to idle"), Anim->GetCurrentStateName(MachineIndex), FName(TEXT("Idle")));
		ASC->RemoveActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
			FGameplayTagContainer(IslandGameplayTags::Cooldown_RockBatteringRam)));
		Movement->Velocity = FVector(500.f, 0.f, 0.f);
		*Acceleration = FVector(500.f, 0.f, 0.f);
		TestTrue(TEXT("Ram can activate again after returning to idle"), ASC->TryActivateAbility(Handle));
		TickAnimation();
		TestEqual(TEXT("Second rush enters the ram animation"), Anim->GetCurrentStateName(MachineIndex), FName(TEXT("RockBatteringRamRunning")));
		ASC->TickComponent(100.f, LEVELTICK_All, nullptr);
		TickAnimation();
		TestEqual(TEXT("Timeout still returns to combat locomotion when movement continues"), Anim->GetCurrentStateName(MachineIndex), FName(TEXT("Combat Walk/Run")));
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
