// Map helper — writes actor positions into a 64x2 texture

#include "Tools/MapLibrary.h"

#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Interfaces/MapDotColorInterface.h"

static constexpr int32 MapTexWidth = 64;

UTexture2D* UMapLibrary::CreateMapTexture()
{
	// 64x2: row 0 = positions, row 1 = colors
	UTexture2D* Tex = UTexture2D::CreateTransient(MapTexWidth, 2, PF_B8G8R8A8);
	if (!Tex)
	{
		return nullptr;
	}

	Tex->Filter = TF_Nearest;
	Tex->SRGB = false;
	Tex->CompressionSettings = TC_VectorDisplacementmap;
	Tex->LODGroup = TEXTUREGROUP_UI;
	Tex->AddressX = TA_Clamp;
	Tex->AddressY = TA_Clamp;

	// Initialize all pixels to inactive (2 rows)
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memzero(RawData, MapTexWidth * 2 * 4);
	Mip.BulkData.Unlock();
	Tex->UpdateResource();

	return Tex;
}

void UMapLibrary::UpdateMapPositions(
	UObject* WorldContextObject,
	UTexture2D* MapTexture,
	TSubclassOf<AActor> ActorClass,
	FVector CenterLocation,
	float CenterYaw,
	float WorldRadius,
	FLinearColor DefaultDotColor,
	bool bRotateWithYaw)
{
	if (!MapTexture || !ActorClass || WorldRadius <= 0.f)
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return;
	}

	// Pre-compute yaw rotation
	float CosYaw = 1.f;
	float SinYaw = 0.f;
	if (bRotateWithYaw)
	{
		const float YawRad = FMath::DegreesToRadians(-CenterYaw);
		CosYaw = FMath::Cos(YawRad);
		SinYaw = FMath::Sin(YawRad);
	}

	// Lock texture pixels
	FTexture2DMipMap& Mip = MapTexture->GetPlatformData()->Mips[0];
	void* RawData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FColor* Pixels = static_cast<FColor*>(RawData);

	// Clear all pixels in both rows (B=0 means inactive)
	FMemory::Memzero(Pixels, MapTexWidth * 2 * sizeof(FColor));

	// Pointer to color row (row 1)
	FColor* ColorPixels = Pixels + MapTexWidth;

	const FColor DefaultColorByte = DefaultDotColor.ToFColor(false);

	int32 DotIndex = 0;

	for (TActorIterator<AActor> It(World, ActorClass); It && DotIndex < MapTexWidth; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		const FVector Delta = Actor->GetActorLocation() - CenterLocation;

		float RelX = Delta.X;
		float RelY = Delta.Y;

		if (bRotateWithYaw)
		{
			const float RotX = RelX * CosYaw - RelY * SinYaw;
			const float RotY = RelX * SinYaw + RelY * CosYaw;
			RelX = RotX;
			RelY = RotY;
		}

		const float NormX =  RelY / WorldRadius;
		const float NormY = -RelX / WorldRadius;

		// Skip actors outside map circle
		if (NormX * NormX + NormY * NormY > 1.f)
		{
			continue;
		}

		// Remap -1..1 to 0..255 for texture storage
		const uint8 PosX = static_cast<uint8>(FMath::Clamp((NormX * 0.5f + 0.5f) * 255.f, 0.f, 255.f));
		const uint8 PosY = static_cast<uint8>(FMath::Clamp((NormY * 0.5f + 0.5f) * 255.f, 0.f, 255.f));

		// Row 0: R=X, G=Y, B=active, A=unused
		Pixels[DotIndex] = FColor(PosX, PosY, 255, 255);

		// Row 1: per-actor color (from interface or default)
		if (Actor->Implements<UMapDotColorInterface>())
		{
			ColorPixels[DotIndex] = IMapDotColorInterface::Execute_GetMapDotColor(Actor).ToFColor(false);
		}
		else
		{
			ColorPixels[DotIndex] = DefaultColorByte;
		}

		DotIndex++;
	}

	Mip.BulkData.Unlock();
	MapTexture->UpdateResource();
}
