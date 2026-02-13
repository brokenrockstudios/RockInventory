// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Item/RockItemDefinition.h"

#include "RockInventoryLogging.h"
#include "Engine/AssetManager.h"

const TArray<FRockItemFragmentInstance>& URockItemDefinition::GetAllFragments() const
{
	return Fragments;
}

void URockItemDefinition::RegisterItemDefinition(const URockItemDefinition* NewItem)
{
	checkf(false, TEXT("RegisterItemDefinition is not tested yet!"));

	if (!NewItem)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RegisterItemDefinition called with null item."));
		return;
	}

	const FPrimaryAssetId AssetId = NewItem->GetPrimaryAssetId();
	if (!AssetId.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RegisterItemDefinition: Invalid PrimaryAssetId for item %s."), *NewItem->GetName());
		return;
	}

	const FSoftObjectPath ItemPath(NewItem);
	if (!ItemPath.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RegisterItemDefinition: Invalid SoftObjectPath for item %s."), *NewItem->GetName());
		return;
	}

	// Empty bundle data (optional, unless you're using bundled loading)
	const FAssetBundleData EmptyBundleData;
	UAssetManager::Get().AddDynamicAsset(AssetId, ItemPath, EmptyBundleData);

	// Consider manually registering with RockItemRegistrySubsystem if needed
	// URockItemRegistrySubsystem::GetInstance()->RegisterRuntimeItemDefinition(NewItem);

	UE_LOG(LogRockInventory, Warning, TEXT("Registered item %s with ID %s"), *NewItem->GetName(), *AssetId.ToString());
}
