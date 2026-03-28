// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameInfoInterface.h"
#include "RacingPlayerState.h"
#include "RacingGameMode.generated.h"

UCLASS()
class RACINGGAME_API ARacingGameMode : public AGameModeBase, public IGameInfoInterface
{
	GENERATED_BODY()

public:
	ARacingGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing")
	int32 TotalLaps = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing")
	TArray<FLinearColor> PlayerColors;

	UFUNCTION(BlueprintCallable, Category = "Racing")
	void BeginCountdown();

	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	virtual void InitGameState() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	// IGameInfoInterface
	virtual void OnLapCompleted_Implementation() override {}
	virtual void OnRaceFinished_Implementation() override {}
	virtual void OnAllPlayersFinished_Implementation() override;
	virtual void OnPlayerConnected_Implementation(const FString& PlayerName) override {}
	virtual void OnLapDataUpdated_Implementation() override {}

private:
	UPROPERTY()
	TArray<AActor*> SortedPlayerStarts;

	UPROPERTY()
	TMap<AController*, int32> PlayerToStartIndex;

	int32 NextPlayerIndex = 0;

	TArray<FLinearColor> AvailableColors;

	void CachePlayerStarts();
};
