// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "IGAbilitySystemComponent.generated.h"

UCLASS()
class ISLANDGAMEPROTOTYPE_API UIGAbilitySystemComponent : public UAbilitySystemComponent {
	GENERATED_BODY()
	
public:
	bool isAbilitiesInitialized; // If true we have initialized our abilities
};
