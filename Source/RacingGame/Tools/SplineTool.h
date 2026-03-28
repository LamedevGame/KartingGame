// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SplineTool.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class RACINGGAME_API ASplineTool : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> Spline;

protected:

	// ── Deformable mesh (SplineMesh) ──

	UPROPERTY(EditAnywhere, Category = "Spline Tool|Deform")
	TObjectPtr<UStaticMesh> Mesh;

	/** Length of one mesh segment along X */
	UPROPERTY(EditAnywhere, Category = "Spline Tool|Deform", meta = (ClampMin = "1.0"))
	float SegmentLength = 200.f;

	UPROPERTY(EditAnywhere, Category = "Spline Tool|Deform")
	FVector2D MeshScale = FVector2D::UnitVector;

	UPROPERTY(EditAnywhere, Category = "Spline Tool|Deform")
	bool bScaleLastToFit = true;

	// ── Instances along spline (HISM, no deformation) ──

	UPROPERTY(EditAnywhere, Category = "Spline Tool|Instances")
	TObjectPtr<UStaticMesh> InstanceMesh;

	/** Spacing between instances along the spline */
	UPROPERTY(EditAnywhere, Category = "Spline Tool|Instances", meta = (ClampMin = "1.0"))
	float InstanceSpacing = 300.f;

	UPROPERTY(EditAnywhere, Category = "Spline Tool|Instances")
	FVector InstanceScale = FVector::OneVector;

	/** Additional offset relative to the spline point (spline local space) */
	UPROPERTY(EditAnywhere, Category = "Spline Tool|Instances")
	FVector InstanceOffset = FVector::ZeroVector;

	/** Additional rotation applied to each instance */
	UPROPERTY(EditAnywhere, Category = "Spline Tool|Instances")
	FRotator InstanceRotationOffset = FRotator::ZeroRotator;

private:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> InstancedMeshComponent;

public:
	ASplineTool();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

private:

	void RebuildMeshes();
	void CreateSegment(float StartDist, float EndDist);
	void RebuildInstances();
};
