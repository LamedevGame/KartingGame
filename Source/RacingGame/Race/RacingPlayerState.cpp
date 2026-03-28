// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Vehicles/KartPawn.h"

void ARacingPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARacingPlayerState, PlayerColor);
}

void ARacingPlayerState::OnRep_PlayerColor()
{
	if (AKartPawn* Kart = Cast<AKartPawn>(GetPawn()))
	{
		Kart->OnPlayerColorReady();
	}
}
