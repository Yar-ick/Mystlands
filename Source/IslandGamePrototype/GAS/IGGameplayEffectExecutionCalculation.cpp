// Fill out your copyright notice in the Description page of Project Settings.
#include "IGGameplayEffectExecutionCalculation.h"

#include "AbilitySystemComponent.h"

void UIGGameplayEffectExecutionCalculation::CustomExecute_Implementation(
	const FGameplayEffectSpec& OwningSpec,
	UAbilitySystemComponent* SourceAbilitySystemComponent,
	AActor* SourceActor
) const {}

void UIGGameplayEffectExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput
) const {
	UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();
	
	CustomExecute(
		ExecutionParams.GetOwningSpec(),
		SourceAbilitySystemComponent,
		SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor_Direct() : nullptr
	);
}
