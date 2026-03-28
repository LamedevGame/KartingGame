// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RacingLapTrigger.generated.h"

class UBoxComponent;
class AKartPawn;

UCLASS()
class RACINGGAME_API ARacingLapTrigger : public AActor
{
	GENERATED_BODY()

public:
	ARacingLapTrigger();

	/** Return the world time when the actor last completed a lap (0 if never). */
	UFUNCTION(BlueprintCallable, Category = "Racing|Lap")
	double GetLastPassTime(AActor* RacingActor) const;

	/** Return seconds elapsed since the actor's last lap completion (0 if never). */
	UFUNCTION(BlueprintCallable, Category = "Racing|Lap")
	double GetTimeSinceLastPass(AActor* RacingActor) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> EntryBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> ExitBox;

private:
	UFUNCTION()
	void OnEntryOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnExitOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Actors that have passed through EntryBox (correct direction)
	TSet<TWeakObjectPtr<AActor>> EnteredActors;

	// Actors that passed Exit without Entry (wrong direction)
	TSet<TWeakObjectPtr<AActor>> WrongExitActors;

	// Map: Actor -> Last time they completed a lap
	TMap<TWeakObjectPtr<AActor>, double> LastPassTimes;
};
