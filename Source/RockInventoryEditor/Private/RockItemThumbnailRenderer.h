// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemDefinition.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"
#include "RockItemThumbnailRenderer.generated.h"

class UTextureThumbnailRenderer;
class UStaticMeshThumbnailRenderer;
/**
 * 
 */
UCLASS()
class URockItemThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_BODY()
public:
	virtual bool CanVisualizeAsset(UObject* Object) override;
	virtual void Draw(
		UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height,
		FRenderTarget* Target, FCanvas* Canvas, bool bAdditionalViewFamily) override;
private:
	UPROPERTY()
	TObjectPtr<UStaticMeshThumbnailRenderer> MeshRenderer = nullptr;
	UPROPERTY()
	TObjectPtr<UTextureThumbnailRenderer> TextureRenderer = nullptr;

	bool DrawIcon(const URockItemDefinition* Def, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Target, FCanvas* Canvas, bool bAdditionalViewFamily);
	bool DrawMesh(const URockItemDefinition* Def, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Target, FCanvas* Canvas, bool bAdditionalViewFamily);
};
