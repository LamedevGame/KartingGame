// Copyright Epic Games, Inc. All Rights Reserved.

#include "KartPlayerController.h"
#include "InputGameInterface.h"
#include "GameInfoInterface.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Blueprint/UserWidget.h"

void AKartPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	// Show default game widget immediately
	if (DefaultGameWidgetClass)
	{
		if (!DefaultGameWidgetInstance)
		{
			DefaultGameWidgetInstance = CreateWidget<UUserWidget>(this, DefaultGameWidgetClass);
		}

		DefaultGameWidgetInstance->AddToViewport();
	}

	if (PreStartGameWidgetClass)
	{
		PreStartGameWidgetInstance = CreateWidget<UUserWidget>(this, PreStartGameWidgetClass);
		PreStartGameWidgetInstance->AddToViewport();

		if (HasAuthority())
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(PreStartGameWidgetInstance->TakeWidget());
			InputMode.SetHideCursorDuringCapture(false);
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
		}
		else
		{
			FInputModeGameOnly InputMode;
			SetInputMode(InputMode);
		}
	}
}

void AKartPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, 0);
	}

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent);

	EIC->BindAction(MoveAction,   ETriggerEvent::Triggered, this, &AKartPlayerController::OnMove);
	EIC->BindAction(MoveAction,   ETriggerEvent::Completed, this, &AKartPlayerController::OnMoveCompleted);

	EIC->BindAction(BrakeAction,  ETriggerEvent::Triggered, this, &AKartPlayerController::OnBrake);
	EIC->BindAction(BrakeAction,  ETriggerEvent::Completed, this, &AKartPlayerController::OnBrakeCompleted);

	EIC->BindAction(HandbrakeAction,  ETriggerEvent::Started,   this, &AKartPlayerController::OnHandbrakeStarted);
	EIC->BindAction(HandbrakeAction,  ETriggerEvent::Completed, this, &AKartPlayerController::OnHandbrakeCompleted);

	EIC->BindAction(LookAroundAction,    ETriggerEvent::Triggered, this, &AKartPlayerController::OnLookAround);
	EIC->BindAction(ToggleCameraAction,  ETriggerEvent::Started,   this, &AKartPlayerController::OnToggleCamera);
}

void AKartPlayerController::OnMove(const FInputActionValue& Value)
{
	if (!bRaceStarted) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		IInputGameInterface::Execute_InputMove(ControlledPawn, Value.Get<FVector2D>());
	}
}

void AKartPlayerController::OnMoveCompleted(const FInputActionValue& Value)
{
	if (!bRaceStarted) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		IInputGameInterface::Execute_InputMoveCompleted(ControlledPawn);
	}
}

void AKartPlayerController::OnBrake(const FInputActionValue& Value)
{
	if (!bRaceStarted) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		IInputGameInterface::Execute_InputBrake(ControlledPawn, Value.Get<float>());
	}
}

void AKartPlayerController::OnBrakeCompleted(const FInputActionValue& Value)
{
	if (!bRaceStarted) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		IInputGameInterface::Execute_InputBrakeCompleted(ControlledPawn);
	}
}

void AKartPlayerController::OnHandbrakeStarted(const FInputActionValue& Value)
{
	if (!bRaceStarted) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		IInputGameInterface::Execute_InputHandbrake(ControlledPawn, true);
	}
}

void AKartPlayerController::OnHandbrakeCompleted(const FInputActionValue& Value)
{
	if (!bRaceStarted) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		IInputGameInterface::Execute_InputHandbrake(ControlledPawn, false);
	}
}

void AKartPlayerController::OnLookAround(const FInputActionValue& Value)
{
	if (APawn* ControlledPawn = GetPawn())
	{
		IInputGameInterface::Execute_InputLookAround(ControlledPawn, Value.Get<FVector2D>());
	}
}

void AKartPlayerController::OnToggleCamera(const FInputActionValue& Value)
{
	if (!bRaceStarted) return;
	if (APawn* ControlledPawn = GetPawn())
	{
		IInputGameInterface::Execute_InputToggleCamera(ControlledPawn);
	}
}

void AKartPlayerController::OnLapCompleted_Implementation()
{
	if (APawn* ControlledPawn = GetPawn())
	{
		IGameInfoInterface::Execute_OnLapCompleted(ControlledPawn);
	}

	ClientShowLapCompleted();
}

void AKartPlayerController::OnRaceFinished_Implementation()
{
	if (APawn* ControlledPawn = GetPawn())
	{
		IGameInfoInterface::Execute_OnRaceFinished(ControlledPawn);
	}

	ClientShowFinish();
}

void AKartPlayerController::ClientShowLapCompleted_Implementation()
{
	if (APawn* ControlledPawn = GetPawn())
	{
		IGameInfoInterface::Execute_OnLapCompleted(ControlledPawn);
	}

	if (!LapCompletedWidgetClass)
	{
		return;
	}

	if (!LapCompletedWidgetInstance)
	{
		LapCompletedWidgetInstance = CreateWidget<UUserWidget>(this, LapCompletedWidgetClass);
	}

	LapCompletedWidgetInstance->AddToViewport();

	GetWorldTimerManager().ClearTimer(LapWidgetTimerHandle);
	GetWorldTimerManager().SetTimer(LapWidgetTimerHandle, this, &AKartPlayerController::HideLapCompletedWidget, 3.0f, false);
}

void AKartPlayerController::ClientShowFinish_Implementation()
{
	if (APawn* ControlledPawn = GetPawn())
	{
		IGameInfoInterface::Execute_OnRaceFinished(ControlledPawn);
	}

	if (!FinishWidgetClass)
	{
		return;
	}

	// Hide lap widget if visible
	if (LapCompletedWidgetInstance && LapCompletedWidgetInstance->IsInViewport())
	{
		LapCompletedWidgetInstance->RemoveFromParent();
		GetWorldTimerManager().ClearTimer(LapWidgetTimerHandle);
	}

	if (!FinishWidgetInstance)
	{
		FinishWidgetInstance = CreateWidget<UUserWidget>(this, FinishWidgetClass);
	}

	FinishWidgetInstance->AddToViewport();
}

void AKartPlayerController::OnAllPlayersFinished_Implementation()
{
	ClientShowAllFinished();
}

void AKartPlayerController::ClientShowAllFinished_Implementation()
{
	GetWorldTimerManager().SetTimer(AllFinishedTimerHandle, this, &AKartPlayerController::ShowAllFinishedWidget, 3.0f, false);
}

void AKartPlayerController::ShowAllFinishedWidget()
{
	// Hide all race widgets
	if (DefaultGameWidgetInstance && DefaultGameWidgetInstance->IsInViewport())
	{
		DefaultGameWidgetInstance->RemoveFromParent();
	}

	if (FinishWidgetInstance && FinishWidgetInstance->IsInViewport())
	{
		FinishWidgetInstance->RemoveFromParent();
	}

	if (LapCompletedWidgetInstance && LapCompletedWidgetInstance->IsInViewport())
	{
		LapCompletedWidgetInstance->RemoveFromParent();
		GetWorldTimerManager().ClearTimer(LapWidgetTimerHandle);
	}

	// Show all-finished widget
	if (AllFinishedWidgetClass)
	{
		if (!AllFinishedWidgetInstance)
		{
			AllFinishedWidgetInstance = CreateWidget<UUserWidget>(this, AllFinishedWidgetClass);
		}

		AllFinishedWidgetInstance->AddToViewport();

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(AllFinishedWidgetInstance->TakeWidget());
		SetInputMode(InputMode);
		SetShowMouseCursor(true);
	}
}

void AKartPlayerController::OnPlayerConnected_Implementation(const FString& PlayerName)
{
}

void AKartPlayerController::OnLapDataUpdated_Implementation()
{
}

void AKartPlayerController::BeginCountdown()
{
	ClientStartCountdown();
}

void AKartPlayerController::ClientStartCountdown_Implementation()
{
	CountdownValue = 5;
	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &AKartPlayerController::CountdownTick, 1.0f, true);
}

void AKartPlayerController::CountdownTick()
{
	CountdownValue--;

	if (CountdownValue <= 1 && CountdownValue > 0)
	{
		// Allow driving on the last countdown index
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
	}

	if (CountdownValue <= 0)
	{
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		StartRace();
	}
}

void AKartPlayerController::StartRace()
{
	bRaceStarted = true;

	// Hide pre-start widget
	if (PreStartGameWidgetInstance && PreStartGameWidgetInstance->IsInViewport())
	{
		PreStartGameWidgetInstance->RemoveFromParent();
	}

	// Switch to game input mode and hide cursor
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	SetShowMouseCursor(false);
}

void AKartPlayerController::HideLapCompletedWidget()
{
	if (LapCompletedWidgetInstance && LapCompletedWidgetInstance->IsInViewport())
	{
		LapCompletedWidgetInstance->RemoveFromParent();
	}
}
