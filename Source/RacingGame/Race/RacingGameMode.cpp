// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingGameMode.h"
#include "RacingGameState.h"
#include "KartPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/KartPawn.h"
#include "GameSetup/RacingGameInstance.h"

ARacingGameMode::ARacingGameMode()
{
	DefaultPawnClass = AKartPawn::StaticClass();
	GameStateClass = ARacingGameState::StaticClass();
	PlayerStateClass = ARacingPlayerState::StaticClass();
}

void ARacingGameMode::PostLogin(APlayerController* NewPlayer)
{
	if (ARacingPlayerState* PS = NewPlayer->GetPlayerState<ARacingPlayerState>())
	{
		if (AvailableColors.Num() == 0)
		{
			AvailableColors = PlayerColors;
		}

		if (AvailableColors.Num() > 0)
		{
			const int32 RandomIndex = FMath::RandRange(0, AvailableColors.Num() - 1);
			PS->PlayerColor = AvailableColors[RandomIndex];
			AvailableColors.RemoveAt(RandomIndex);
		}
	}

	Super::PostLogin(NewPlayer);

	// Notify all players about the new connection
	const FString PlayerName = NewPlayer->PlayerState ? NewPlayer->PlayerState->GetPlayerName() : TEXT("Unknown");
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			IGameInfoInterface::Execute_OnPlayerConnected(PC, PlayerName);
		}
	}
}

void ARacingGameMode::InitGameState()
{
	Super::InitGameState();

	if (URacingGameInstance* GI = GetGameInstance<URacingGameInstance>())
	{
		TotalLaps = GI->TotalLaps;
	}

	if (ARacingGameState* RS = GetGameState<ARacingGameState>())
	{
		RS->TotalLaps = TotalLaps;
	}
}

void ARacingGameMode::CachePlayerStarts()
{
	if (SortedPlayerStarts.Num() > 0)
	{
		return;
	}

	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	// Sort by name
	FoundStarts.Sort([](const AActor& A, const AActor& B)
	{
		return A.GetName() < B.GetName();
	});

	SortedPlayerStarts = FoundStarts;
}

AActor* ARacingGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	CachePlayerStarts();

	if (SortedPlayerStarts.Num() == 0)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	// Check if this player already has an assigned start
	if (const int32* ExistingIndex = PlayerToStartIndex.Find(Player))
	{
		return SortedPlayerStarts[*ExistingIndex % SortedPlayerStarts.Num()];
	}

	const int32 Index = NextPlayerIndex % SortedPlayerStarts.Num();
	PlayerToStartIndex.Add(Player, NextPlayerIndex);
	NextPlayerIndex++;

	return SortedPlayerStarts[Index];
}

void ARacingGameMode::BeginCountdown()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AKartPlayerController* KPC = Cast<AKartPlayerController>(It->Get()))
		{
			KPC->BeginCountdown();
		}
	}
}

void ARacingGameMode::OnAllPlayersFinished_Implementation()
{
}

APawn* ARacingGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	APawn* SpawnedPawn = Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);

	if (AKartPawn* Kart = Cast<AKartPawn>(SpawnedPawn))
	{
		if (const int32* Index = PlayerToStartIndex.Find(NewPlayer))
		{
			Kart->KartNumber = *Index + 1;
		}
		else
		{
			Kart->KartNumber = 1;
		}

		if (ARacingGameState* GS = GetGameState<ARacingGameState>())
		{
			GS->RegisterPlayer(NewPlayer->PlayerState, Kart->KartNumber);
		}
	}

	return SpawnedPawn;
}
