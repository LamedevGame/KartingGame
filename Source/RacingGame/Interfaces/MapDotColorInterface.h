// Interface for actors that provide a custom color on the minimap.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MapDotColorInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMapDotColorInterface : public UInterface
{
	GENERATED_BODY()
};

class RACINGGAME_API IMapDotColorInterface
{
	GENERATED_BODY()

public:
	/** Return the color this actor should use on the minimap. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Map")
	FLinearColor GetMapDotColor();
};
