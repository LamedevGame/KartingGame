#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RacingGameInstance.generated.h"

UCLASS()
class RACINGGAME_API URacingGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Number of laps required to finish the race. */
	UPROPERTY(BlueprintReadWrite, Category = "Racing|Settings")
	int32 TotalLaps = 3;
};
