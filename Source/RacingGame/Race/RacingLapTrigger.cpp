// Copyright Epic Games, Inc. All Rights Reserved.

#include "RacingLapTrigger.h"
#include "Components/BoxComponent.h"
#include "Vehicles/KartPawn.h"
#include "RacingGameState.h"

ARacingLapTrigger::ARacingLapTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	// Entry box (must pass through first)
	EntryBox = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryBox"));
	EntryBox->SetBoxExtent(FVector(50.0f, 500.0f, 100.0f));
	EntryBox->SetCollisionProfileName(TEXT("Trigger"));
	EntryBox->SetGenerateOverlapEvents(true);
	RootComponent = EntryBox;

	// Exit box (validates lap completion)
	ExitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitBox"));
	ExitBox->SetBoxExtent(FVector(50.0f, 500.0f, 100.0f));
	ExitBox->SetCollisionProfileName(TEXT("Trigger"));
	ExitBox->SetGenerateOverlapEvents(true);
	ExitBox->SetupAttachment(RootComponent);
	ExitBox->SetRelativeLocation(FVector(200.0f, 0.0f, 0.0f));

	EntryBox->OnComponentBeginOverlap.AddDynamic(this, &ARacingLapTrigger::OnEntryOverlap);
	ExitBox->OnComponentBeginOverlap.AddDynamic(this, &ARacingLapTrigger::OnExitOverlap);
}

void ARacingLapTrigger::OnEntryOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AKartPawn* Kart = Cast<AKartPawn>(OtherActor);
	if (!Kart)
	{
		return;
	}

	// Exit → Entry = wrong direction, reset timer
	if (WrongExitActors.Contains(Kart))
	{
		WrongExitActors.Remove(Kart);
		LastPassTimes.Remove(Kart);
		return;
	}

	EnteredActors.Add(Kart);
}

void ARacingLapTrigger::OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AKartPawn* Kart = Cast<AKartPawn>(OtherActor);
	if (!Kart)
	{
		return;
	}

	// Exit without Entry - mark as potential wrong direction
	if (!EnteredActors.Contains(Kart))
	{
		WrongExitActors.Add(Kart);
		return;
	}

	EnteredActors.Remove(Kart);

	const double CurrentTime = GetWorld()->GetTimeSeconds();

	if (const double* LastTime = LastPassTimes.Find(Kart))
	{
		const double LapTime = CurrentTime - *LastTime;

		if (ARacingGameState* GS = GetWorld()->GetGameState<ARacingGameState>())
		{
			GS->RegisterLapTime(Kart, Kart->KartNumber, LapTime);
		}
	}
	else
	{
		if (ARacingGameState* GS = GetWorld()->GetGameState<ARacingGameState>())
		{
			GS->StartRace(Kart, Kart->KartNumber);
		}
	}

	LastPassTimes.Add(Kart, CurrentTime);
}

double ARacingLapTrigger::GetLastPassTime(AActor* RacingActor) const
{
	if (const double* Time = LastPassTimes.Find(RacingActor))
	{
		return *Time;
	}
	return 0.0;
}

double ARacingLapTrigger::GetTimeSinceLastPass(AActor* RacingActor) const
{
	if (const double* LastTime = LastPassTimes.Find(RacingActor))
	{
		return GetWorld()->GetTimeSeconds() - *LastTime;
	}
	return 0.0;
}
