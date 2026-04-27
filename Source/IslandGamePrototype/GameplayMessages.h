#pragma once

#include "GameplayMessages.generated.h"

USTRUCT(BlueprintType)
struct FFloatMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 1.f;
};
