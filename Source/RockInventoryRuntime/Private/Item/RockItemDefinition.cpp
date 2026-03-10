// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Item/RockItemDefinition.h"

#include "RockInventoryLogging.h"
#include "Engine/AssetManager.h"
#include "Misc/RockInventoryDeveloperSettings.h"

const TArray<FRockItemFragmentInstance>& URockItemDefinition::GetAllFragments() const
{
	return Fragments;
}

FPrimaryAssetId URockItemDefinition::GetPrimaryAssetId() const
{
	if (ItemId != NAME_None)
	{
		return FPrimaryAssetId("RockItemDefinition", ItemId);
	}
	return FPrimaryAssetId("RockItemDefinition", GetFName());
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

UClass* URockItemDefinition::GetWorldItemClass()
{
	// If you need different behavior, make a subclass of URockItemDefinition and override this function.
	// Such as adding a fragment lookup or having the data be part of ItemDefinition.
	return GetDefault<URockInventoryDeveloperSettings>()->DefaultWorldItemClass;
}

void URockItemDefinition::RebuildStatTags()
{
	StatTags = FGameplayTagStackContainer();
	for (const FGameplayTagStack& StatTag : StatTagDefaults)
	{
		if (StatTag.GetTag().IsValid())
		{
			StatTags.SetStack(StatTag.GetTag(), StatTag.GetStackCount(), true);
		}
		else
		{
			UE_LOG(LogRockInventory, Warning, TEXT("RebuildStatTags - Invalid tag in StatTagDefaults for item %s"), *GetName());
		}
	}
}

void URockItemDefinition::SetDefaultItemId()
{
	if (ItemId == NAME_None)
	{
		ItemId = GetFName();
		UE_LOG(LogRockInventory, Warning, TEXT("SetDefaultItemId - ItemId was not set for item %s, defaulting to %s"), *GetName(), *ItemId.ToString());
	}
}

void URockItemDefinition::PostLoad()
{
	Super::PostLoad();
	RebuildStatTags();
	SetDefaultItemId();
}

void URockItemDefinition::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	static const FName StatTagDefaultsName = GET_MEMBER_NAME_CHECKED(URockItemDefinition, StatTagDefaults);
	if (PropertyChangedEvent.GetPropertyName() == StatTagDefaultsName ||
		PropertyChangedEvent.GetMemberPropertyName() == StatTagDefaultsName)
	{
		RebuildStatTags();
	}
}
