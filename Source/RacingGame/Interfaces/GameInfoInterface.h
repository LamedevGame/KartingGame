// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameInfoInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGameInfoInterface : public UInterface
{
	GENERATED_BODY()
};

class RACINGGAME_API IGameInfoInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Racing")
	void OnLapCompleted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Racing")
	void OnRaceFinished();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Racing")
	void OnAllPlayersFinished();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Racing")
	void OnPlayerConnected(const FString& PlayerName);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Racing")
	void OnLapDataUpdated();
};
