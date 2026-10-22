#pragma once

#include "CoreMinimal.h"
#include "IGGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GA_Player_RockBatteringRam.generated.h"

class ACharacter;
class UCameraComponent;
class UCameraShakeBase;
class UCurveFloat;
class UNiagaraComponent;
class UNiagaraSystem;
class UParticleSystem;
class USoundBase;
class UAbilityTask_WaitGameplayEvent;

/** The Blueprint child supplies tuning and assets only. */
UCLASS()
class ISLANDGAMEPROTOTYPE_API UGA_Player_RockBatteringRam : public UIGGameplayAbility
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Ground", meta = (ClampMin = "0"))
	float SkillMaxWalkSpeed = 1500.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Ground", meta = (ClampMin = "0"))
	float SkillMaxAcceleration = 10000.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Ram|Ground")
	FScalableFloat MaxDuration = FScalableFloat(6.f);
	
	UPROPERTY(EditDefaultsOnly, Category="Ram|Ground")
	FScalableFloat ChargeTime = FScalableFloat(0.f);
	
	/** Base damage per second spent rushing, before the upgrade multiplier. */
	UPROPERTY(EditDefaultsOnly, Category="Ram|Ground", meta = (ClampMin = "0"))
	float BaseGroundDamagePerSecond = 15.f;

	/** Upgrade multiplier applied to the base ground damage rate. */
	UPROPERTY(EditDefaultsOnly, Category="Ram|Ground")
	FScalableFloat GroundDamageMultiplier = FScalableFloat(1.f);
	
	UPROPERTY(EditDefaultsOnly, Category="Ram|Ground")
	FScalableFloat BaseGroundKnockback = FScalableFloat(500.f);
	
	/** Degrees per second at full horizontal input. */
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Ground")
	FScalableFloat DeviationVelocity = FScalableFloat(90.f);
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Ground", meta = (ClampMin = "0", ClampMax = "180"))
	float StopAngle = 45.f;
	
	/** Optional normalized speed ramp; linear when unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Ground")
	TObjectPtr<UCurveFloat> SpeedCurve;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Ground", meta = (ClampMin = "0", ClampMax = "90"))
	float KnockbackDeflectionAngle = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air", meta = (ClampMin = "0"))
	float MinimumHeightToUseAirSkill = 200.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air", meta = (ClampMin = "1"))
	float GroundTraceDistance = 3000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air", meta = (ClampMin = "1"))
	float DiveSpeed = 5000.f;
	
	/** Ends a dive that never reaches a landing surface. */
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air", meta = (ClampMin = "0.1"))
	float MaxAirDuration = 10.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	FScalableFloat AirDamagePerMeter = FScalableFloat(5.f);
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	float MinAirImpactDamage = 10.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	float MaxAirImpactDamage = 150.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	FScalableFloat AirImpactRadiusPerMeter = FScalableFloat(50.f);
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	float MinAirImpactRadius = 100.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	float MaxAirImpactRadius = 400.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	FScalableFloat AirKnockbackPerMeter = FScalableFloat(500.f);
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	float MinAirKnockback = 300.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Air")
	float MaxAirKnockback = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Ram|Effects")
	TSubclassOf<UGameplayEffect> GroundBuffEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Effects")
	TSubclassOf<UGameplayEffect> HitImmunityEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Effects")
	FGameplayTag HitImmunityTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Effects")
	TSubclassOf<UGameplayEffect> DamageEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Effects")
	FGameplayTag DamageSetByCallerTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Effects")
	TSubclassOf<UGameplayEffect> LastHitEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Visuals")
	TObjectPtr<UNiagaraSystem> GroundWindSystem;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Visuals")
	TObjectPtr<UNiagaraSystem> HandRockSystem;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Visuals")
	FName HandRockSocket = NAME_None;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Visuals")
	TObjectPtr<UNiagaraSystem> AirWindSystem;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Visuals")
	TObjectPtr<UParticleSystem> GroundImpactParticles;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Visuals")
	TObjectPtr<UParticleSystem> AirImpactParticles;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Visuals")
	TObjectPtr<USoundBase> GroundImpactSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Visuals")
	TObjectPtr<USoundBase> AirImpactSound;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Camera")
	TSubclassOf<UCameraShakeBase> GroundCameraShake;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Camera")
	TSubclassOf<UCameraShakeBase> GroundImpactCameraShake;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Camera")
	TSubclassOf<UCameraShakeBase> AirImpactCameraShake;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Camera")
	float GroundFOV = 110.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ram|Camera", meta = (ClampMin = "0"))
	float FOVInterpSpeed = 10.f;

private:
	friend class UAbilityTask_RockBatteringRamTick;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Player;
	
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;
	
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> WindParticles;
	
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> RockParticles;
	
	UPROPERTY(Transient)
	TObjectPtr<UCameraShakeBase> ActiveCameraShake;
	
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> EventTask;
	
	FActiveGameplayEffectHandle BuffHandle;
	
	TSet<TWeakObjectPtr<ACharacter>> AirHitCharacters;
	
	float Elapsed = 0.f;
	
	float Duration = 0.f;
	
	float GroundDamageRate = 0.f;
	
	float GroundKnockback = 0.f;
	
	float TurnSpeed = 0.f;
	
	float AirDamage = 0.f;
	
	float AirRadius = 0.f;
	
	float AirKnockback = 0.f;
	
	float OriginalMaxWalkSpeed = 0.f;
	
	float OriginalMaxAcceleration = 0.f;
	
	float OriginalFOV = 0.f;
	
	int64 OriginalAnimationState = 0;
	
	ECollisionResponse OriginalPawnResponse = ECR_Block;
	
	bool bOriginalControllerYaw = true;
	
	bool bGroundActive = false;
	
	bool bAirActive = false;
	
	bool bStarted = false;
	
	bool bEnding = false;
	
public:
	UGA_Player_RockBatteringRam();
	
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
									const FGameplayAbilityActorInfo* ActorInfo,
									const FGameplayTagContainer* SourceTags = nullptr,
									const FGameplayTagContainer* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								 const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo,
								 const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility,
							bool bWasCancelled) override;
	
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
							   const FGameplayAbilityActorInfo* ActorInfo,
							   const FGameplayAbilityActivationInfo ActivationInfo) const override;
	
private:
	void TickRam(float DeltaTime);
	
	void StartGround();
	
	void StartAir(float Height);
	
	void FinishRam(bool bCancelled = false);
	
	bool TraceGround(ACharacter* Character, FHitResult& Hit) const;
	
	float Evaluate(const FScalableFloat& Value, FName UpgradeAttribute) const;
	
	FVector GetKnockback(ACharacter* Target, float Strength, bool bGround) const;
	
	void ImpactCharacter(ACharacter* Target, float Damage, float Knockback, bool bGround);
	
	void PlayImpact(const FVector& Location, bool bGround);
	
	void SetAnimationState(int64 State);
	
	UFUNCTION()
	void OnRamEvent(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnLanded(const FHitResult& Hit);
	
	UFUNCTION()
	void OnCapsuleOverlap(UPrimitiveComponent* OverlappedComponent,
						  AActor* OtherActor,
						  UPrimitiveComponent* OtherComp,
						  int32 OtherBodyIndex,
						  bool bFromSweep,
						  const FHitResult& SweepResult);	
		
	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComponent,
					  AActor* OtherActor,
					  UPrimitiveComponent* OtherComp, 
					  FVector NormalImpulse, 
					  const FHitResult& Hit);
};

/** Ticks only while the owning ability is active; GAS owns its lifetime. */
UCLASS()
class UAbilityTask_RockBatteringRamTick : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	UAbilityTask_RockBatteringRamTick();
	
	static UAbilityTask_RockBatteringRamTick* Create(UGA_Player_RockBatteringRam* OwningAbility);
	
	virtual void TickTask(float DeltaTime) override;
};
