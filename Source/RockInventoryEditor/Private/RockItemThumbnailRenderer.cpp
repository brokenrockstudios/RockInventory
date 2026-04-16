// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "RockItemThumbnailRenderer.h"

#include "Item/RockItemDefinition.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "ThumbnailRendering/StaticMeshThumbnailRenderer.h"
#include "ThumbnailRendering/TextureThumbnailRenderer.h"

bool URockItemThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	const auto Mode = GetDefault<URockInventoryDeveloperSettings>()->ItemDefinitionThumbnailMode;
	if (Mode == ERockThumbnailMode::Default)
	{
		return false;
	}

	if (const URockItemDefinition* Def = Cast<URockItemDefinition>(Object))
	{
		switch (Mode)
		{
		case ERockThumbnailMode::Icon: return !Def->IconData.Icon.IsNull();
		case ERockThumbnailMode::Mesh: return !Def->ItemMesh.IsNull();
		case ERockThumbnailMode::Auto: return !Def->IconData.Icon.IsNull() || !Def->ItemMesh.IsNull();
		default: return false;
		}
	}
	return false;
}

void URockItemThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* Target, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	const URockItemDefinition* Def = Cast<URockItemDefinition>(Object);
	if (!Def) return;

	const auto Mode = GetDefault<URockInventoryDeveloperSettings>()->ItemDefinitionThumbnailMode;

	switch (Mode)
	{
	case ERockThumbnailMode::Icon:
		DrawIcon(Def, X, Y, Width, Height, Target, Canvas, bAdditionalViewFamily);
		break;
	case ERockThumbnailMode::Mesh:
		DrawMesh(Def, X, Y, Width, Height, Target, Canvas, bAdditionalViewFamily);
		break;
	case ERockThumbnailMode::Auto:
		if (!DrawIcon(Def, X, Y, Width, Height, Target, Canvas, bAdditionalViewFamily))
			DrawMesh(Def, X, Y, Width, Height, Target, Canvas, bAdditionalViewFamily);
		break;
	default:
		break;
	}
}

bool URockItemThumbnailRenderer::DrawIcon(
	const URockItemDefinition* Def, int32 X, int32 Y, uint32 Width, uint32 Height,
	FRenderTarget* Target, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UTexture2D* Texture = Def->IconData.Icon.LoadSynchronous();
	if (!Texture) return false;
	
	if (!TextureRenderer)
	{
		TextureRenderer = NewObject<UTextureThumbnailRenderer>(this);
	}
	TextureRenderer->Draw(Texture, X, Y, Width, Height, Target, Canvas, bAdditionalViewFamily);
	return true;
}

bool URockItemThumbnailRenderer::DrawMesh(
	const URockItemDefinition* Def, int32 X, int32 Y, uint32 Width, uint32 Height,
	FRenderTarget* Target, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UStaticMesh* Mesh = Def->ItemMesh.LoadSynchronous();
	if (!Mesh) return false;

	if (!MeshRenderer)
	{
		MeshRenderer = NewObject<UStaticMeshThumbnailRenderer>(this);
	}

	MeshRenderer->Draw(Mesh, X, Y, Width, Height, Target, Canvas, bAdditionalViewFamily);
	return true;
}
