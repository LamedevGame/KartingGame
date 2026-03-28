// Copyright Epic Games, Inc. All Rights Reserved.

#include "KartPawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AKartPawn::AKartPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// Driver skeletal mesh
	Driver = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Driver"));
	Driver->SetupAttachment(GetMesh(), FName("driver"));

	// Nickname world anchor
	NicknamePoint = CreateDefaultSubobject<USceneComponent>(TEXT("NicknamePoint"));
	NicknamePoint->SetupAttachment(GetMesh());
	NicknamePoint->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));

	// Front camera (hood)
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("FrontSpringArm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FrontCamera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = true;

	// Back camera (rear view)
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("BackSpringArm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = false;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("BackCamera"));
	BackCamera->SetupAttachment(BackSpringArm);
	BackCamera->bAutoActivate = false;
}

void AKartPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AKartPawn, HeadRotation);
	DOREPLIFETIME(AKartPawn, SteeringInput);
	DOREPLIFETIME(AKartPawn, ThrottleInput);
	DOREPLIFETIME(AKartPawn, BrakeInput);
}

void AKartPawn::BeginPlay()
{
	Super::BeginPlay();

	if (bFrontCameraActive)
	{
		ActivateFirstPersonCamera();
	}
	else
	{
		ActivateThirdPersonCamera();
	}

	if (NicknameWidgetClass && GetNetMode() != NM_DedicatedServer)
	{
		APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
		if (LocalPC && LocalPC->IsLocalController())
		{
			NicknameWidget = CreateWidget<UUserWidget>(LocalPC, NicknameWidgetClass);
			NicknameWidget->AddToViewport();
			NicknameWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
			if (GetPlayerState())
			{
				OnPlayerColorReady();
			}
		}
	}
}

void AKartPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (NicknameWidget)
	{
		NicknameWidget->RemoveFromParent();
		NicknameWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AKartPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector WorldVelocity = GetVelocity();
	const FVector CurrentLocalVelocity = GetActorTransform().InverseTransformVector(WorldVelocity);

	const FVector LocalAcceleration = (CurrentLocalVelocity - PreviousLocalVelocity) / DeltaTime;
	PreviousLocalVelocity = CurrentLocalVelocity;

	// Low-pass filter for longitudinal acceleration
	FilteredLocalAcceleration = FMath::VInterpTo(FilteredLocalAcceleration, LocalAcceleration, DeltaTime, 5.0f);

	const float ForwardSpeed = ChaosVehicleMovement->GetForwardSpeed();
	const float AbsSpeed = FMath::Abs(ForwardSpeed);
	if (AbsSpeed < 1.0f)
	{
		SpeedTrend = 0;
	}
	else if (AbsSpeed < PreviousAbsSpeed)
	{
		SpeedTrend = -1;
	}
	else
	{
		SpeedTrend = 1;
	}
	PreviousAbsSpeed = AbsSpeed;

	// Centrifugal acceleration: v * yaw_rate
	const float YawRateRad = FMath::DegreesToRadians(GetMesh()->GetPhysicsAngularVelocityInDegrees().Z);
	const float CentrifugalAccel = ForwardSpeed * YawRateRad;

	// Low-pass filter for centrifugal acceleration
	FilteredCentrifugalAccel = FMath::FInterpTo(FilteredCentrifugalAccel, CentrifugalAccel, DeltaTime, 5.0f);

	FVector TargetOffset;
	TargetOffset.X = FMath::Clamp(-FilteredLocalAcceleration.X * 0.001f, -1.0f, 1.0f);
	TargetOffset.Y = FMath::Clamp(-FilteredCentrifugalAccel * 0.001f, -1.0f, 1.0f);
	TargetOffset.Z = 0.0f;

	BodyOffset = FMath::VInterpTo(BodyOffset, TargetOffset, DeltaTime, 10.0f);

	if (NicknameWidget)
	{
		APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
		if (LocalPC && LocalPC->IsLocalController())
		{
			FVector2D ScreenPos;
			const FVector NicknameWorldPos = NicknamePoint->GetComponentLocation();
			const bool bOnScreen = UGameplayStatics::ProjectWorldToScreen(
				LocalPC, NicknameWorldPos, ScreenPos);

			NicknameWidget->SetVisibility(bOnScreen
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Hidden);

			if (bOnScreen)
			{
				NicknameWidget->SetPositionInViewport(ScreenPos);

				const float Distance = FVector::Dist(
					LocalPC->PlayerCameraManager->GetCameraLocation(), NicknameWorldPos);
				const float Scale = FMath::Clamp(NicknameReferenceDistance / FMath::Max(Distance, 1.f),
					NicknameMinScale, NicknameMaxScale);
				NicknameWidget->SetRenderScale(FVector2D(Scale));
			}
		}
	}
}

void AKartPawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	OnPlayerColorReady();
}

void AKartPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	OnPlayerColorReady();
}

void AKartPawn::OnLapCompleted_Implementation()
{
}

void AKartPawn::OnRaceFinished_Implementation()
{
	bRaceFinished = true;

	ChaosVehicleMovement->SetThrottleInput(0.0f);
	ChaosVehicleMovement->SetSteeringInput(0.0f);
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AKartPawn::InputMove_Implementation(FVector2D Value)
{
	if (bRaceFinished) return;
	ChaosVehicleMovement->SetSteeringInput(Value.X);

	// If dedicated brake is held (not from our own auto-braking), don't override throttle/gear
	if (!bAutoBraking && ChaosVehicleMovement->GetBrakeInput() > 0.0f) return;

	const float ForwardSpeed = ChaosVehicleMovement->GetForwardSpeed();

	// Going forward, pressing backward — brake first until nearly stopped
	if (Value.Y < 0.0f && ForwardSpeed > 5.0f)
	{
		bAutoBraking = true;
		ChaosVehicleMovement->SetThrottleInput(0.0f);
		ChaosVehicleMovement->SetBrakeInput(1.0f);
		ServerUpdateVehicleInput(Value.X, 0.0f, 1.0f);
		return;
	}

	// Going backward, pressing forward — brake first until nearly stopped
	if (Value.Y > 0.0f && ForwardSpeed < -5.0f)
	{
		bAutoBraking = true;
		ChaosVehicleMovement->SetThrottleInput(0.0f);
		ChaosVehicleMovement->SetBrakeInput(1.0f);
		ServerUpdateVehicleInput(Value.X, 0.0f, 1.0f);
		return;
	}

	// Speed is low enough (or same direction) — switch gear and apply throttle
	bAutoBraking = false;
	ChaosVehicleMovement->SetBrakeInput(0.0f);
	ChaosVehicleMovement->SetTargetGear(Value.Y, true);
	const float Throttle = (Value.Y == 0) ? 0.0f : 1.0f;
	ChaosVehicleMovement->SetThrottleInput(Throttle);

	ServerUpdateVehicleInput(Value.X, Throttle, 0.0f);
}

void AKartPawn::InputMoveCompleted_Implementation()
{
	bAutoBraking = false;
	ChaosVehicleMovement->SetBrakeInput(0.0f);
	ChaosVehicleMovement->SetThrottleInput(0.0f);
	ChaosVehicleMovement->SetSteeringInput(0.0f);

	ServerUpdateVehicleInput(0.0f, 0.0f, 0.0f);
}

void AKartPawn::InputBrake_Implementation(float Value)
{
	if (bRaceFinished) return;
	ChaosVehicleMovement->SetTargetGear(0.0f, true);
	ChaosVehicleMovement->SetThrottleInput(0.0f);
	ChaosVehicleMovement->SetBrakeInput(Value);

	ServerUpdateVehicleInput(SteeringInput, 0.0f, Value);
}

void AKartPawn::InputBrakeCompleted_Implementation()
{
	ChaosVehicleMovement->SetBrakeInput(0.0f);

	ServerUpdateVehicleInput(SteeringInput, 0.0f, 0.0f);
}

void AKartPawn::InputHandbrake_Implementation(bool bPressed)
{
	if (bRaceFinished) return;
	ChaosVehicleMovement->SetHandbrakeInput(bPressed);
}

void AKartPawn::InputLookAround_Implementation(FVector2D Value)
{
	if (bFrontCameraActive)
	{
		FrontCameraYaw   = FMath::Clamp(FrontCameraYaw   + Value.X, -FrontCameraYawLimit,   FrontCameraYawLimit);
		FrontCameraPitch = FMath::Clamp(FrontCameraPitch - Value.Y, -FrontCameraPitchLimit, FrontCameraPitchLimit);
		FrontSpringArm->SetRelativeRotation(FRotator(FrontCameraPitch, FrontCameraYaw, 0.0f));

		ServerUpdateHeadRotation(FRotator(0.0f, -FrontCameraPitch, -FrontCameraYaw));
	}
	else
	{
		CameraYaw   += Value.X;
		CameraPitch  = FMath::Clamp(CameraPitch - Value.Y, -80.0f, 80.0f);
		BackSpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}

void AKartPawn::InputToggleCamera_Implementation()
{
	bFrontCameraActive = !bFrontCameraActive;

	if (bFrontCameraActive)
	{
		ActivateFirstPersonCamera();
	}
	else
	{
		ActivateThirdPersonCamera();
	}
}

void AKartPawn::ActivateFirstPersonCamera()
{
	FrontCamera->SetActive(true);
	BackCamera->SetActive(false);

	FrontCameraYaw = 0.0f;
	FrontCameraPitch = 0.0f;
	FrontSpringArm->SetRelativeRotation(FRotator::ZeroRotator);

	if (IsLocallyControlled())
	{
		Driver->HideBoneByName(FName("head"), PBO_None);
	}
}

void AKartPawn::ActivateThirdPersonCamera()
{
	FrontCamera->SetActive(false);
	BackCamera->SetActive(true);

	CameraYaw = 0.0f;
	CameraPitch = 0.0f;
	BackSpringArm->SetRelativeRotation(FRotator::ZeroRotator);

	ServerUpdateHeadRotation(FRotator::ZeroRotator);

	if (IsLocallyControlled())
	{
		Driver->UnHideBoneByName(FName("head"));
	}
}

void AKartPawn::ServerUpdateHeadRotation_Implementation(FRotator NewRotation)
{
	HeadRotation = NewRotation;
}

void AKartPawn::ServerUpdateVehicleInput_Implementation(float NewSteering, float NewThrottle, float NewBrake)
{
	SteeringInput = NewSteering;
	ThrottleInput = NewThrottle;
	BrakeInput = NewBrake;
}
