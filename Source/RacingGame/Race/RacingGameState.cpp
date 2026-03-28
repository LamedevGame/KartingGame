// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingGameState.h"
#include "RacingPlayerState.h"
#include "Interfaces/GameInfoInterface.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

ARacingGameState::ARacingGameState()
{
	bReplicates = true;
}

void ARacingGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARacingGameState, AllLapData);
	DOREPLIFETIME(ARacingGameState, TotalLaps);
}

void ARacingGameState::RegisterPlayer(APlayerState* PlayerState, int32 KartNumber)
{
	if (!HasAuthority() || !PlayerState)
	{
		return;
	}

	FPlayerLapData* Data = FindOrAddPlayerData(PlayerState);
	if (!Data)
	{
		return;
	}

	Data->KartNumber = KartNumber;

	if (ARacingPlayerState* RPS = Cast<ARacingPlayerState>(PlayerState))
	{
		Data->PlayerColor = RPS->PlayerColor;
	}

	BroadcastLapDataUpdated();
}

void ARacingGameState::StartRace(APawn* RacingPawn, int32 KartNumber)
{
	if (!HasAuthority() || !RacingPawn)
	{
		return;
	}

	APlayerState* PS = RacingPawn->GetPlayerState();
	if (!PS)
	{
		return;
	}

	FPlayerLapData* Data = FindOrAddPlayerData(PS);
	if (!Data)
	{
		return;
	}

	Data->KartNumber = KartNumber;
	Data->CurrentLap = 1;
	BroadcastLapDataUpdated();
}

void ARacingGameState::RegisterLapTime(APawn* RacingPawn, int32 KartNumber, float LapTime)
{
	if (!HasAuthority() || !RacingPawn)
	{
		return;
	}

	APlayerState* PS = RacingPawn->GetPlayerState();
	if (!PS)
	{
		return;
	}

	FPlayerLapData* Data = FindOrAddPlayerData(PS);
	if (!Data)
	{
		return;
	}

	if (Data->bFinished)
	{
		return;
	}

	Data->KartNumber = KartNumber;
	Data->LastLapTime = LapTime;
	Data->AllLapTimes.Add(LapTime);

	if (Data->BestLapTime <= 0.0f || LapTime < Data->BestLapTime)
	{
		Data->BestLapTime = LapTime;
	}

	AController* Controller = RacingPawn->GetController();

	Data->CurrentLap++;

	if (Controller)
	{
		IGameInfoInterface::Execute_OnLapCompleted(Controller);
	}

	if (TotalLaps > 0 && Data->CurrentLap > TotalLaps)
	{
		Data->bFinished = true;
		Data->FinishPosition = NextFinishPosition++;

		float Total = 0.0f;
		for (float T : Data->AllLapTimes)
		{
			Total += T;
		}
		Data->TotalRaceTime = Total;

		if (Controller)
		{
			IGameInfoInterface::Execute_OnRaceFinished(Controller);
		}

		// Check if all players finished
		bool bAllFinished = true;
		for (const FPlayerLapData& LapData : AllLapData)
		{
			if (!LapData.bFinished)
			{
				bAllFinished = false;
				break;
			}
		}

		if (bAllFinished)
		{
			if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
			{
				IGameInfoInterface::Execute_OnAllPlayersFinished(GM);
			}

			for (const FPlayerLapData& LapData : AllLapData)
			{
				if (LapData.PlayerState)
				{
					if (AController* PC = Cast<AController>(LapData.PlayerState->GetOwner()))
					{
						IGameInfoInterface::Execute_OnAllPlayersFinished(PC);
					}
				}
			}
		}
	}

	BroadcastLapDataUpdated();
}

void ARacingGameState::CancelLastLap(APawn* RacingPawn)
{
	if (!HasAuthority() || !RacingPawn)
	{
		return;
	}

	APlayerState* PS = RacingPawn->GetPlayerState();
	if (!PS)
	{
		return;
	}

	FPlayerLapData* Data = FindOrAddPlayerData(PS);
	if (!Data || Data->CurrentLap <= 0)
	{
		return;
	}

	Data->CurrentLap--;

	if (Data->AllLapTimes.Num() > 0)
	{
		Data->AllLapTimes.RemoveAt(Data->AllLapTimes.Num() - 1);
	}

	// Recalculate best lap time
	Data->BestLapTime = 0.0f;
	for (float Time : Data->AllLapTimes)
	{
		if (Data->BestLapTime <= 0.0f || Time < Data->BestLapTime)
		{
			Data->BestLapTime = Time;
		}
	}

	if (Data->AllLapTimes.Num() > 0)
	{
		Data->LastLapTime = Data->AllLapTimes.Last();
	}
	else
	{
		Data->LastLapTime = 0.0f;
	}

	BroadcastLapDataUpdated();
}

void ARacingGameState::OnRep_AllLapData()
{
	BroadcastLapDataUpdated();
}

FPlayerLapData ARacingGameState::GetLapData(APlayerState* PlayerState) const
{
	for (const FPlayerLapData& Data : AllLapData)
	{
		if (Data.PlayerState == PlayerState)
		{
			return Data;
		}
	}
	return FPlayerLapData();
}

TArray<FPlayerLapData> ARacingGameState::GetSortedByBestTime() const
{
	TArray<FPlayerLapData> Sorted = AllLapData;
	Sorted.Sort([](const FPlayerLapData& A, const FPlayerLapData& B)
	{
		if (A.BestLapTime <= 0.0f) return false;
		if (B.BestLapTime <= 0.0f) return true;
		return A.BestLapTime < B.BestLapTime;
	});
	return Sorted;
}

TArray<FPlayerLapData> ARacingGameState::GetSortedByCurrentLap() const
{
	TArray<FPlayerLapData> Sorted = AllLapData;
	Sorted.Sort([](const FPlayerLapData& A, const FPlayerLapData& B)
	{
		return A.CurrentLap > B.CurrentLap;
	});
	return Sorted;
}

void ARacingGameState::BroadcastLapDataUpdated()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			IGameInfoInterface::Execute_OnLapDataUpdated(PC);
		}
	}
}

FPlayerLapData* ARacingGameState::FindOrAddPlayerData(APlayerState* PlayerState)
{
	for (FPlayerLapData& Data : AllLapData)
	{
		if (Data.PlayerState == PlayerState)
		{
			return &Data;
		}
	}

	FPlayerLapData NewData;
	NewData.PlayerState = PlayerState;
	AllLapData.Add(NewData);
	return &AllLapData.Last();
}
