#include "IGCharacterBase.h"
#include "IGGameplayAbility.h"

AIGCharacterBase::AIGCharacterBase() {
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UIGAbilitySystemComponent>("AbilitySystemComponent");
	AttributeSet = CreateDefaultSubobject<UIGAttributeSet>("AttributeSet");
}

void AIGCharacterBase::PossessedBy(AController* NewController) {
	Super::PossessedBy(NewController);

	// Initialize abilities
	if (AbilitySystemComponent) {
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AddStartupGameplayAbilities();
	}
}

void AIGCharacterBase::BeginPlay() {
	Super::BeginPlay();
}

void AIGCharacterBase::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AIGCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AIGCharacterBase::AddStartupGameplayAbilities() {
	check(AbilitySystemComponent);

	if (!AbilitySystemComponent->isAbilitiesInitialized)
		return;
	
	for (TSubclassOf<UIGGameplayAbility>& StartupAbility : GameplayAbilities) {
		AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(
				StartupAbility,
				1,
				INDEX_NONE,
				this
			)
		);
	}
	
	AbilitySystemComponent->isAbilitiesInitialized = true;
}


bool AIGCharacterBase::IsAlive() const {
	if (GetHealth() > 0.0f)
		return true;

	return false;
}

float AIGCharacterBase::GetHealth() const {
	if (!AttributeSet)
		return 0.0f;

	return AttributeSet->GetHealth();
}

float AIGCharacterBase::GetMaxHealth() const {
	if (!AttributeSet)
		return 0.0f;

	return AttributeSet->GetMaxHealth();
}

float AIGCharacterBase::GetDamageResistance() const {
	if (!AttributeSet)
		return 0.0f;

	return AttributeSet->GetDamageResistance();
}


UAbilitySystemComponent* AIGCharacterBase::GetAbilitySystemComponent() const {
	return AbilitySystemComponent;
}
