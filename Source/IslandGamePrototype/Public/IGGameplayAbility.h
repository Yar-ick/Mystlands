// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "IGGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class ISLANDGAMEPROTOTYPE_API UIGGameplayAbility : public UGameplayAbility {
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cooldown")
	FScalableFloat CooldownDuration;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Cooldown")
	FGameplayTagContainer CooldownTags;

	// Temp container that we will return the pointer to in GetCooldownTags().
	// This will be a union of our CooldownTags and the Cooldown GE's cooldown tags.
	UPROPERTY(Transient)
	FGameplayTagContainer TempCooldownTags;
	
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) const override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	
	// На память
	UFUNCTION(BlueprintCallable, Category = Ability)
	FGameplayAbilitySpecHandle GetAbilitySpecHandle() const;
};
