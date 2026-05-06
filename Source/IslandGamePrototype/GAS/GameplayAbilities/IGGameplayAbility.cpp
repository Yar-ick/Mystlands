#include "IGGameplayAbility.h"

void UIGGameplayAbility::ApplyCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo
) const {
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	
	if (CooldownGE) {
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());
		SpecHandle.Data.Get()->DynamicGrantedTags.AppendTags(CooldownTags);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Cooldown")), CooldownDuration.GetValueAtLevel(GetAbilityLevel()));
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

const FGameplayTagContainer* UIGGameplayAbility::GetCooldownTags() const {
	FGameplayTagContainer* MutableTags = const_cast<FGameplayTagContainer*>(&TempCooldownTags);
	MutableTags->Reset(); // MutableTags writes to the TempCooldownTags on the CDO so clear it in case the ability cooldown tags change (moved to a different slot)
	const FGameplayTagContainer* ParentTags = Super::GetCooldownTags();
	
	if (ParentTags)
		MutableTags->AppendTags(*ParentTags);
	
	MutableTags->AppendTags(CooldownTags);
	return MutableTags;
}

FGameplayAbilitySpecHandle UIGGameplayAbility::GetAbilitySpecHandle() const {
	return Super::GetCurrentAbilitySpecHandle();
}
