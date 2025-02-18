// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "IGAbilitySystemComponent.h"
#include "IGAttributeSet.h"
#include "IGGameplayAbility.h"
#include "IGCharacterBase.generated.h"

UCLASS()
class ISLANDGAMEPROTOTYPE_API AIGCharacterBase : public ACharacter, public IAbilitySystemInterface {
	GENERATED_BODY()

public:
	AIGCharacterBase();

	virtual void PossessedBy(AController* NewController) override;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/***********	Gameplay Ability System		***********/
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UIGAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UIGAttributeSet> AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Abilities)
	TArray<TSubclassOf<UIGGameplayAbility>> GameplayAbilities; // Abilities to grant to this character on creation

	
	void AddStartupGameplayAbilities(); // Apply the startup gameplay abilities and effects
	

	UFUNCTION(BlueprintCallable)
	virtual bool IsAlive() const;

	UFUNCTION(BlueprintCallable)
	virtual float GetHealth() const;

	UFUNCTION(BlueprintCallable)
	virtual float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable)
	virtual float GetDamageResistance() const;


	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*
	 * Called when health is changed
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float DeltaValue);

	/*
	 * Called when damage resistance is changed
	 */
	UFUNCTION(BlueprintImplementableEvent)
	void OnDamageResistanceChanged(float DeltaValue);
};
