// Fill out your copyright notice in the Description page of Project Settings.

#include "IGAttributeSet.h"

#include "IGCharacterBase.h"

UIGAttributeSet::UIGAttributeSet() {}

void UIGAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) {
	Super::PreAttributeChange(Attribute, NewValue);
	
	// Remake later (Universal method for changing attribute max value)
	if (Attribute == GetMaxHealthAttribute())
		SetHealth(NewValue);
}

void UIGAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) {
	Super::PostGameplayEffectExecute(Data);
	
	float DeltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive) {
		// If this was additive, store the raw delta value to be passed along later
		DeltaValue = Data.EvaluatedData.Magnitude;
	}

	AActor* TargetActor = nullptr;
	AIGCharacterBase* TargetCharacter = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid()) {
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetCharacter = Cast<AIGCharacterBase>(TargetActor);
	}
	
	if (Data.EvaluatedData.Attribute == GetDamageAttribute()) {
		// Store a local copy of the amount of damage done and clear the damage attribute
		const float LocalDamage = GetDamage() * (1 - GetDamageResistance());
		SetDamage(0.0f);
		
		// Apply the health change and then clamp it
		const float NewHealth = GetHealth() - LocalDamage;
		const float DeltaHealth = NewHealth - GetHealth();
		SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));

		if (TargetCharacter)
			TargetCharacter->OnHealthChanged(DeltaHealth);
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		if (TargetCharacter)
			TargetCharacter->OnHealthChanged(DeltaValue);
	}
	else if (Data.EvaluatedData.Attribute == GetDamageResistanceAttribute()) {
		SetDamageResistance(FMath::Clamp(GetDamageResistance(), 0.0f, 1.0f));

		if (TargetCharacter)
			TargetCharacter->OnDamageResistanceChanged(DeltaValue);
	}
}
