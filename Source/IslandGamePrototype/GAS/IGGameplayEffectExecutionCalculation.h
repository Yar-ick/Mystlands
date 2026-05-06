// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "IGGameplayEffectExecutionCalculation.generated.h"

/**
 * 
 */
UCLASS()
class ISLANDGAMEPROTOTYPE_API UIGGameplayEffectExecutionCalculation : public UGameplayEffectExecutionCalculation {
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, Category="Calculation")
	void CustomExecute(const FGameplayEffectSpec& OwningSpec, UAbilitySystemComponent* SourceAbilitySystemComponent, AActor* SourceActor) const;
	
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

};
