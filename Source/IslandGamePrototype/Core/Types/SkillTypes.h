#pragma once

#include "CoreMinimal.h"

#include "SkillTypes.generated.h"

UENUM(BlueprintType)
enum class EHookInteractionType : uint8
{
	EHIT_Attractor			UMETA(DisplayName = "Attractor"),
	EHIT_Attractor_Ledge	UMETA(DisplayName = "AttractorLedge"),
	EHIT_Attractor_Booster	UMETA(DisplayName = "AttractorBooster"),
	EHIT_Enemy				UMETA(DisplayName = "Enemy"),
	EHIT_EnemyReverse		UMETA(DisplayName = "EnemyReverse"),
	EHIT_Environment		UMETA(DisplayName = "Environment")
};