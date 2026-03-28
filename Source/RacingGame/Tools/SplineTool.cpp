// Copyright Epic Games, Inc. All Rights Reserved.

#include "SplineTool.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

ASplineTool::ASplineTool()
{
	PrimaryActorTick.bCanEverTick = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	RootComponent = Spline;

	InstancedMeshComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("InstancedMeshes"));
	InstancedMeshComponent->SetupAttachment(Spline);
	InstancedMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

void ASplineTool::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildMeshes();
	RebuildInstances();
}

void ASplineTool::BeginPlay()
{
	Super::BeginPlay();
	RebuildMeshes();
	RebuildInstances();
}

void ASplineTool::RebuildMeshes()
{
	TInlineComponentArray<USplineMeshComponent*> Old;
	GetComponents<USplineMeshComponent>(Old);
	for (USplineMeshComponent* C : Old)
	{
		C->DestroyComponent();
	}

	if (!Mesh) return;

	const float SplineLen = Spline->GetSplineLength();
	if (SplineLen <= 0.f) return;

	const int32 Count = FMath::FloorToInt(SplineLen / SegmentLength);

	for (int32 i = 0; i < Count; i++)
	{
		CreateSegment(i * SegmentLength, (i + 1) * SegmentLength);
	}

	if (bScaleLastToFit)
	{
		const float Tail = SplineLen - Count * SegmentLength;
		if (Tail > KINDA_SMALL_NUMBER)
		{
			CreateSegment(Count * SegmentLength, SplineLen);
		}
	}
}

void ASplineTool::CreateSegment(float StartDist, float EndDist)
{
	USplineMeshComponent* SMC = NewObject<USplineMeshComponent>(this);
	SMC->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	SMC->SetStaticMesh(Mesh);
	SMC->SetMobility(EComponentMobility::Movable);
	SMC->SetForwardAxis(ESplineMeshAxis::X);
	SMC->SetupAttachment(Spline);

	const FVector P0 = Spline->GetLocationAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::Local);
	const FVector P1 = Spline->GetLocationAtDistanceAlongSpline(EndDist, ESplineCoordinateSpace::Local);
	const FVector T0 = Spline->GetTangentAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::Local).GetClampedToMaxSize(EndDist - StartDist);
	const FVector T1 = Spline->GetTangentAtDistanceAlongSpline(EndDist, ESplineCoordinateSpace::Local).GetClampedToMaxSize(EndDist - StartDist);

	SMC->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	SMC->SetStartAndEnd(P0, T0, P1, T1);
	SMC->SetStartScale(MeshScale);
	SMC->SetEndScale(MeshScale);

	AddInstanceComponent(SMC);
	SMC->RegisterComponent();
}

void ASplineTool::RebuildInstances()
{
	InstancedMeshComponent->ClearInstances();

	if (!InstanceMesh) return;

	InstancedMeshComponent->SetStaticMesh(InstanceMesh);

	const float SplineLen = Spline->GetSplineLength();
	if (SplineLen <= 0.f) return;

	const int32 Count = FMath::FloorToInt(SplineLen / InstanceSpacing) + 1;
	const FQuat RotOffset = InstanceRotationOffset.Quaternion();

	for (int32 i = 0; i < Count; i++)
	{
		const float Dist = FMath::Min(i * InstanceSpacing, SplineLen);

		const FVector Location = Spline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::Local);
		const FQuat Rotation = Spline->GetQuaternionAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::Local);

		FTransform T;
		T.SetLocation(Location + Rotation.RotateVector(InstanceOffset));
		T.SetRotation(Rotation * RotOffset);
		T.SetScale3D(InstanceScale);

		InstancedMeshComponent->AddInstance(T);
	}
}
