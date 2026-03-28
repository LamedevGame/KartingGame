// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "RacingPlayerState.generated.h"

UCLASS()
class RACINGGAME_API ARacingPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing=OnRep_PlayerColor, BlueprintReadOnly, Category = "Racing")
	FLinearColor PlayerColor = FLinearColor::White;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_PlayerColor();
};
