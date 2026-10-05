// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryEditor.h"

#include "RockItemThumbnailRenderer.h"
#include "Editor.h"
#include "Item/RockItemDefinition.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ThumbnailRendering/ThumbnailManager.h"


#define LOCTEXT_NAMESPACE "FRockInventoryEditorModule"

void FRockInventoryEditorModule::StartupModule()
{
	UThumbnailManager::Get().RegisterCustomRenderer(URockItemDefinition::StaticClass(), URockItemThumbnailRenderer::StaticClass());

	// The item registry is an engine subsystem and outlives PIE, so tell it when item assets may have changed.
	PreBeginPIEHandle = FEditorDelegates::PreBeginPIE.AddLambda([this](bool) { MarkItemRegistryDirty(); });

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetAddedHandle = AssetRegistry.OnAssetAdded().AddRaw(this, &FRockInventoryEditorModule::OnAssetChanged);
	AssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddRaw(this, &FRockInventoryEditorModule::OnAssetChanged);
	AssetRenamedHandle = AssetRegistry.OnAssetRenamed().AddRaw(this, &FRockInventoryEditorModule::OnAssetRenamed);
}

void FRockInventoryEditorModule::ShutdownModule()
{
	FEditorDelegates::PreBeginPIE.Remove(PreBeginPIEHandle);
	if (FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
	{
		IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
		AssetRegistry.OnAssetAdded().Remove(AssetAddedHandle);
		AssetRegistry.OnAssetRemoved().Remove(AssetRemovedHandle);
		AssetRegistry.OnAssetRenamed().Remove(AssetRenamedHandle);
	}
}

void FRockInventoryEditorModule::MarkItemRegistryDirty()
{
	if (URockItemRegistrySubsystem* Registry = URockItemRegistrySubsystem::GetInstance())
	{
		Registry->MarkDirty();
	}
}

void FRockInventoryEditorModule::OnAssetChanged(const FAssetData& AssetData)
{
	if (AssetData.IsInstanceOf(URockItemDefinition::StaticClass()))
	{
		MarkItemRegistryDirty();
	}
}

void FRockInventoryEditorModule::OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
	OnAssetChanged(AssetData);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRockInventoryEditorModule, RockInventoryEditor)
