// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "RacingGameState.generated.h"

USTRUCT(BlueprintType)
struct FPlayerLapData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int32 KartNumber = 0;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor PlayerColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentLap = 0;

	UPROPERTY(BlueprintReadOnly)
	float LastLapTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float BestLapTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	TArray<float> AllLapTimes;

	UPROPERTY(BlueprintReadOnly)
	bool bFinished = false;

	UPROPERTY(BlueprintReadOnly)
	int32 FinishPosition = 0;

	UPROPERTY(BlueprintReadOnly)
	float TotalRaceTime = 0.0f;
};

UCLASS()
class RACINGGAME_API ARacingGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ARacingGameState();

	/** Add a player to the race standings. */
	void RegisterPlayer(APlayerState* PlayerState, int32 KartNumber);

	/** Mark a player's race as started and set their lap to 1. */
	void StartRace(APawn* RacingPawn, int32 KartNumber);

	/** Record a completed lap time and advance the player's lap counter. */
	void RegisterLapTime(APawn* RacingPawn, int32 KartNumber, float LapTime);

	/** Undo the last recorded lap for the given pawn. */
	void CancelLastLap(APawn* RacingPawn);

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Racing")
	int32 TotalLaps = 0;

	/** Return lap data for a specific player. */
	UFUNCTION(BlueprintCallable, Category = "Racing")
	FPlayerLapData GetLapData(APlayerState* PlayerState) const;

	/** Return all lap data sorted by best lap time. */
	UFUNCTION(BlueprintCallable, Category = "Racing")
	TArray<FPlayerLapData> GetAllLapData() const { return GetSortedByBestTime(); }

	/** Return all lap data sorted by best lap time (ascending). */
	UFUNCTION(BlueprintCallable, Category = "Racing")
	TArray<FPlayerLapData> GetSortedByBestTime() const;

	/** Return all lap data sorted by current lap (descending). */
	UFUNCTION(BlueprintCallable, Category = "Racing")
	TArray<FPlayerLapData> GetSortedByCurrentLap() const;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_AllLapData();

	UPROPERTY(ReplicatedUsing=OnRep_AllLapData)
	TArray<FPlayerLapData> AllLapData;

	int32 NextFinishPosition = 1;

	FPlayerLapData* FindOrAddPlayerData(APlayerState* PlayerState);

	void BroadcastLapDataUpdated();
};
