// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameInfoInterface.h"
#include "KartPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
struct FInputActionValue;

UCLASS()
class RACINGGAME_API AKartPlayerController : public APlayerController, public IGameInfoInterface
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> BrakeAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> HandbrakeAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAroundAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ToggleCameraAction;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> PreStartGameWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> DefaultGameWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> LapCompletedWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> FinishWidgetClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> AllFinishedWidgetClass;


public:

	/** Start the pre-race countdown on this player's client. */
	void BeginCountdown();

protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:

	// IGameInfoInterface
	virtual void OnLapCompleted_Implementation() override;
	virtual void OnRaceFinished_Implementation() override;
	virtual void OnAllPlayersFinished_Implementation() override;
	virtual void OnPlayerConnected_Implementation(const FString& PlayerName) override;
	virtual void OnLapDataUpdated_Implementation() override;

	UFUNCTION(Client, Reliable)
	void ClientShowLapCompleted();

	UFUNCTION(Client, Reliable)
	void ClientShowFinish();

	UFUNCTION(Client, Reliable)
	void ClientShowAllFinished();

	UFUNCTION(Client, Reliable)
	void ClientStartCountdown();

	void CountdownTick();
	void StartRace();
	void HideLapCompletedWidget();
	void ShowAllFinishedWidget();

	UPROPERTY()
	TObjectPtr<UUserWidget> PreStartGameWidgetInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> DefaultGameWidgetInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> LapCompletedWidgetInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> FinishWidgetInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> AllFinishedWidgetInstance;

	FTimerHandle LapWidgetTimerHandle;
	FTimerHandle AllFinishedTimerHandle;
	FTimerHandle CountdownTimerHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Racing", meta = (AllowPrivateAccess = "true"))
	int32 CountdownValue = 0;

	bool bRaceStarted = false;

	void OnMove(const FInputActionValue& Value);
	void OnMoveCompleted(const FInputActionValue& Value);

	void OnBrake(const FInputActionValue& Value);
	void OnBrakeCompleted(const FInputActionValue& Value);
	void OnHandbrakeStarted(const FInputActionValue& Value);
	void OnHandbrakeCompleted(const FInputActionValue& Value);
	void OnLookAround(const FInputActionValue& Value);
	void OnToggleCamera(const FInputActionValue& Value);
};
