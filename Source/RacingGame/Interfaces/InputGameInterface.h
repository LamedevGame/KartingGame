// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InputGameInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInputGameInterface : public UInterface
{
	GENERATED_BODY()
};

class RACINGGAME_API IInputGameInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kart|Input")
	void InputMove(FVector2D Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kart|Input")
	void InputBrake(float Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kart|Input")
	void InputHandbrake(bool bPressed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kart|Input")
	void InputMoveCompleted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kart|Input")
	void InputBrakeCompleted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kart|Input")
	void InputLookAround(FVector2D Value);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Kart|Input")
	void InputToggleCamera();
};
