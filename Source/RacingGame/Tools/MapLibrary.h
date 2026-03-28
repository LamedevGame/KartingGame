// Map helper — writes actor positions into a 64x2 texture
// so a material shader can draw dots entirely on the GPU.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MapLibrary.generated.h"

class UTexture2D;

UCLASS()
class RACINGGAME_API UMapLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Creates a 64x2 transient texture used as the map data buffer.
	 * Row 0 = positions, Row 1 = colors.
	 * Call once and store the result. Pass it to UpdateMapPositions and to the map material.
	 */
	UFUNCTION(BlueprintCallable, Category = "Map")
	static UTexture2D* CreateMapTexture();

	/**
	 * Writes normalized actor positions into the map texture.
	 *
	 * Row 0: each pixel stores one actor: R = X (0-1), G = Y (0-1), B = active (1 or 0).
	 * Row 1: each pixel stores the per-actor dot color (RGB).
	 *        If the actor implements IMapDotColorInterface, its color is used;
	 *        otherwise DefaultDotColor is used.
	 * Positions are normalized so 0.5 = center of map.
	 * Actors outside WorldRadius are skipped.
	 *
	 * @param MapTexture       64x2 texture created by CreateMapTexture
	 * @param ActorClass       Which actor class to find and draw
	 * @param CenterLocation   World-space center of the map (your pawn location)
	 * @param CenterYaw        Yaw of the center actor (used when bRotateWithYaw = true)
	 * @param WorldRadius      World-space radius visible on the map (cm)
	 * @param DefaultDotColor  Fallback color for actors that don't implement IMapDotColorInterface
	 * @param bRotateWithYaw   If true, map rotates so CenterYaw is always up
	 */
	UFUNCTION(BlueprintCallable, Category = "Map", meta = (WorldContext = "WorldContextObject"))
	static void UpdateMapPositions(
		UObject* WorldContextObject,
		UTexture2D* MapTexture,
		TSubclassOf<AActor> ActorClass,
		FVector CenterLocation,
		float CenterYaw,
		float WorldRadius,
		FLinearColor DefaultDotColor = FLinearColor::White,
		bool bRotateWithYaw = false
	);
};
