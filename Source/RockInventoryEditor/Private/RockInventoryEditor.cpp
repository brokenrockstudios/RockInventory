// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryEditor.h"

#include "RockItemThumbnailRenderer.h"
#include "Item/RockItemDefinition.h"
#include "ThumbnailRendering/ThumbnailManager.h"


#define LOCTEXT_NAMESPACE "FRockInventoryEditorModule"

void FRockInventoryEditorModule::StartupModule()
{
	UThumbnailManager::Get().RegisterCustomRenderer(URockItemDefinition::StaticClass(), URockItemThumbnailRenderer::StaticClass());
}

void FRockInventoryEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRockInventoryEditorModule, RockInventoryEditor)
