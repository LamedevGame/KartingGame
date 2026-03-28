// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "InputGameInterface.h"
#include "GameInfoInterface.h"
#include "KartPawn.generated.h"

class UChaosWheeledVehicleMovementComponent;
class UCameraComponent;
class USpringArmComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UUserWidget;

UCLASS()
class RACINGGAME_API AKartPawn : public AWheeledVehiclePawn, public IInputGameInterface, public IGameInfoInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> FrontSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FrontCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> BackSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> BackCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Driver;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> NicknamePoint;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUserWidget> NicknameWidget;

	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

	bool bFrontCameraActive = true;
	bool bRaceFinished = false;

	FVector PreviousLocalVelocity = FVector::ZeroVector;
	FVector FilteredLocalAcceleration = FVector::ZeroVector;
	float FilteredCentrifugalAccel = 0.0f;

	float PreviousAbsSpeed = 0.0f;

	bool bAutoBraking = false;

	float CameraYaw = 0.0f;
	float CameraPitch = 0.0f;

	float FrontCameraYaw = 0.0f;
	float FrontCameraPitch = 0.0f;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FPS")
	float FrontCameraYawLimit = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|FPS")
	float FrontCameraPitchLimit = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nickname")
	TSubclassOf<UUserWidget> NicknameWidgetClass;

	/** Distance (cm) at which scale = 1.0 */
	UPROPERTY(EditDefaultsOnly, Category = "Nickname", meta = (ClampMin = "1.0"))
	float NicknameReferenceDistance = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Nickname", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float NicknameMinScale = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Nickname", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float NicknameMaxScale = 3.f;

public:
	AKartPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_PlayerState() override;
	virtual void PossessedBy(AController* NewController) override;

	/** Called when the replicated player color becomes available. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Racing")
	void OnPlayerColorReady();

private:

	// IGameInfoInterface
	virtual void OnLapCompleted_Implementation() override;
	virtual void OnRaceFinished_Implementation() override;
	virtual void OnAllPlayersFinished_Implementation() override {}
	virtual void OnPlayerConnected_Implementation(const FString& PlayerName) override {}
	virtual void OnLapDataUpdated_Implementation() override {}

	// IInputGameInterface
	virtual void InputMove_Implementation(FVector2D Value) override;
	virtual void InputBrake_Implementation(float Value) override;
	virtual void InputHandbrake_Implementation(bool bPressed) override;
	virtual void InputMoveCompleted_Implementation() override;
	virtual void InputBrakeCompleted_Implementation() override;
	virtual void InputLookAround_Implementation(FVector2D Value) override;
	virtual void InputToggleCamera_Implementation() override;

	void ActivateFirstPersonCamera();
	void ActivateThirdPersonCamera();

	UFUNCTION(Server, Unreliable)
	void ServerUpdateHeadRotation(FRotator NewRotation);

	UFUNCTION(Server, Unreliable)
	void ServerUpdateVehicleInput(float NewSteering, float NewThrottle, float NewBrake);

public:

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Driver")
	FRotator HeadRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Vehicle")
	float SteeringInput = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Vehicle")
	float ThrottleInput = 0.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Vehicle")
	float BrakeInput = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Physics")
	FVector BodyOffset = FVector::ZeroVector;

	/** 1 when absolute speed is increasing, -1 when decreasing, 0 when stationary */
	UPROPERTY(BlueprintReadOnly, Category = "Physics")
	int32 SpeedTrend = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing")
	int32 KartNumber = 0;

	FORCEINLINE TObjectPtr<UChaosWheeledVehicleMovementComponent> GetChaosVehicleMovement() const { return ChaosVehicleMovement; }
};
