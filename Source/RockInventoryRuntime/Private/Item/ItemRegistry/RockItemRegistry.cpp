// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/StreamableManager.h"
#include "Item/RockItemDefinition.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "ProfilingDebugging/ScopedTimers.h"
#include "UObject/Package.h"

// Define a log category for easier debugging
DEFINE_LOG_CATEGORY_STATIC(LogRockItemRegistry, Log, All);

FString IRockItemDefinitionProvider::DescribeItemSource(const URockItemDefinition* Definition) const
{
	return GetPathNameSafe(Definition);
}

URockItemRegistrySubsystem* URockItemRegistrySubsystem::GetInstance()
{
	return GEngine ? GEngine->GetEngineSubsystem<URockItemRegistrySubsystem>() : nullptr;
}

void URockItemRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Nothing is loaded here: the registry builds on the first lookup (see EnsureUpToDate).
	bDirty = true;
}

void URockItemRegistrySubsystem::Deinitialize()
{
	LoadedById.Empty();
	TestDefinitions.Empty();
	ItemDefinitionMap.Empty();
	Providers.Empty();
	ProviderDefinitions.Empty();
	bDirty = true;
	Super::Deinitialize();
}

void URockItemRegistrySubsystem::RegisterForTest(URockItemDefinition* Definition)
{
	if (Definition && !Definition->ItemId.IsNone())
	{
		TestDefinitions.Add(Definition->ItemId, Definition);
		OnRefreshed.Broadcast();
	}
}

void URockItemRegistrySubsystem::UnregisterForTest(const URockItemDefinition* Definition)
{
	if (!Definition)
	{
		return;
	}
	const TObjectPtr<URockItemDefinition>* Found = TestDefinitions.Find(Definition->ItemId);
	if (Found && Found->Get() == Definition)
	{
		TestDefinitions.Remove(Definition->ItemId);
	}
}

void URockItemRegistrySubsystem::MarkDirty()
{
	bDirty = true;
}

void URockItemRegistrySubsystem::RegisterProvider(IRockItemDefinitionProvider* Provider)
{
	if (Provider && !Providers.Contains(Provider))
	{
		Providers.Add(Provider);
		MarkDirty();
	}
}

void URockItemRegistrySubsystem::UnregisterProvider(IRockItemDefinitionProvider* Provider)
{
	if (Providers.Remove(Provider) > 0)
	{
		MarkDirty();
	}
}

bool URockItemRegistrySubsystem::EnsureUpToDate() const
{
	if (!bDirty)
	{
		return true;
	}
	if (!UAssetManager::IsInitialized())
	{
		UE_LOG(LogRockItemRegistry, Warning, TEXT("Item registry used before the Asset Manager was initialized."));
		return false;
	}
	// Lookups are logically const; the refresh only fills a cache.
	const_cast<URockItemRegistrySubsystem*>(this)->Refresh();
	return true;
}

void URockItemRegistrySubsystem::Refresh()
{
	double TimeRefreshing = 0.0;
	int32 NumLoaded = 0;
	int32 NumRemoved = 0;
	{
		FScopedDurationTimer Timer(TimeRefreshing);
		UAssetManager& AssetManager = UAssetManager::Get();

		TArray<FPrimaryAssetId> CurrentIds;
		AssetManager.GetPrimaryAssetIdList(ItemDefinitionAssetType, CurrentIds);
		// A stable order makes "first one wins" on a duplicate ItemId deterministic.
		CurrentIds.Sort([](const FPrimaryAssetId& A, const FPrimaryAssetId& B) { return A.ToString() < B.ToString(); });

		// Drop assets that are gone. Everything already loaded stays as it is.
		for (auto It = LoadedById.CreateIterator(); It; ++It)
		{
			if (!CurrentIds.Contains(It.Key()))
			{
				It.RemoveCurrent();
				++NumRemoved;
			}
		}

		// Load only the ids we do not hold yet.
		TArray<FPrimaryAssetId> NewIds;
		for (const FPrimaryAssetId& AssetId : CurrentIds)
		{
			if (!LoadedById.Contains(AssetId))
			{
				NewIds.Add(AssetId);
			}
		}

		if (NewIds.Num() > 0)
		{
			// A null handle is not a failure: the Asset Manager returns null when everything is already in the requested state.
			if (const TSharedPtr<FStreamableHandle> Handle = AssetManager.LoadPrimaryAssets(NewIds))
			{
				Handle->WaitUntilComplete();
			}

			for (const FPrimaryAssetId& AssetId : NewIds)
			{
				UObject* LoadedAsset = AssetManager.GetPrimaryAssetObject(AssetId);
				if (URockItemDefinition* ItemDef = Cast<URockItemDefinition>(LoadedAsset))
				{
					LoadedById.Add(AssetId, ItemDef);
					++NumLoaded;
				}
				else if (LoadedAsset)
				{
					UE_LOG(LogRockItemRegistry, Warning,
						TEXT("Asset '%s' associated with PrimaryAssetId '%s' is not a URockItemDefinition. Skipping."),
						*GetPathNameSafe(LoadedAsset), *AssetId.ToString());
				}
				else
				{
					UE_LOG(LogRockItemRegistry, Warning, TEXT("Failed to load item definition PrimaryAssetId '%s'."), *AssetId.ToString());
				}
			}
		}

		// Rebuild the lookup. This also picks up an ItemId that was changed on an already loaded definition.
		// Sources are consulted in order (assets, then providers); the first definition for an ItemId wins.
		ItemDefinitionMap.Reset();
		ProviderDefinitions.Reset();
		TMap<FName, FString> SourceById;
		auto AddDefinition = [this, &SourceById](URockItemDefinition* ItemDef, const FString& Source)
		{
			if (IsUnknownItemDefinition(ItemDef))
			{
				// The placeholder is not an item: it stays out of the lookup (and out of its None ItemId warning).
			}
			else if (ItemDef->ItemId.IsNone())
			{
				UE_LOG(LogRockItemRegistry, Warning, TEXT("Item Definition '%s' has a None ItemId. Skipping."), *Source);
			}
			else if (const TObjectPtr<URockItemDefinition>* Existing = ItemDefinitionMap.Find(ItemDef->ItemId))
			{
				// Duplicate ItemId found! This is usually an error in data setup.
				UE_LOG(LogRockItemRegistry, Error,
					TEXT("Duplicate ItemId '%s' found! '%s' conflicts with existing '%s'. Ignoring the new one."),
					*ItemDef->ItemId.ToString(), *Source, *SourceById.FindRef(ItemDef->ItemId));
			}
			else
			{
				ItemDefinitionMap.Add(ItemDef->ItemId, ItemDef);
				SourceById.Add(ItemDef->ItemId, Source);
			}
		};
		for (const FPrimaryAssetId& AssetId : CurrentIds)
		{
			const TObjectPtr<URockItemDefinition>* Found = LoadedById.Find(AssetId);
			if (URockItemDefinition* ItemDef = Found ? Found->Get() : nullptr)
			{
				AddDefinition(ItemDef, GetPathNameSafe(ItemDef));
			}
		}
		for (const IRockItemDefinitionProvider* Provider : Providers)
		{
			TArray<URockItemDefinition*> Offered;
			Provider->EnumerateItemDefinitions(Offered);
			for (URockItemDefinition* ItemDef : Offered)
			{
				if (ItemDef)
				{
					ProviderDefinitions.AddUnique(ItemDef);
					AddDefinition(ItemDef, Provider->DescribeItemSource(ItemDef));
				}
			}
		}
	}

	bDirty = false;
	UE_LOG(LogRockItemRegistry, Display, TEXT("Item registry refreshed in %.3f seconds: %d loaded, %d removed, %d definitions in total."),
		TimeRefreshing, NumLoaded, NumRemoved, ItemDefinitionMap.Num());
	OnRefreshed.Broadcast();
}

URockItemDefinition* URockItemRegistrySubsystem::FindDefinition(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		UE_LOG(LogRockItemRegistry, Warning, TEXT("Attempted to FindDefinition with None ItemID."));
		return nullptr;
	}
	URockItemDefinition* Found = TryFindDefinition(ItemID);
	if (!Found && !bDirty)
	{
		UE_LOG(LogRockItemRegistry, Warning, TEXT("Could not find Item Definition with ID '%s'."), *ItemID.ToString());
	}
	return Found;
}

URockItemDefinition* URockItemRegistrySubsystem::TryFindDefinition(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return nullptr;
	}
	if (const TObjectPtr<URockItemDefinition>* TestDef = TestDefinitions.Find(ItemID))
	{
		return *TestDef;
	}
	if (!EnsureUpToDate())
	{
		return nullptr;
	}
	if (const TObjectPtr<URockItemDefinition>* FoundDefPtr = ItemDefinitionMap.Find(ItemID))
	{
		return *FoundDefPtr;
	}
	return nullptr;
}

URockItemDefinition* URockItemRegistrySubsystem::GetUnknownItemDefinition() const
{
	if (const URockInventoryDeveloperSettings* Settings = GetDefault<URockInventoryDeveloperSettings>())
	{
		if (URockItemDefinition* Configured = Settings->UnknownItemDefinition.LoadSynchronous())
		{
			return Configured;
		}
	}
	if (!TransientUnknownDefinition)
	{
		URockItemDefinition* Default = NewObject<URockItemDefinition>(GetTransientPackage(), TEXT("UnknownItemDefinition"), RF_Transient);
		Default->ItemId = TEXT("Unknown");
		Default->Name = NSLOCTEXT("RockInventory", "UnknownItemName", "Unknown item");
		Default->DisplayName = Default->Name;
		Default->Description = NSLOCTEXT("RockInventory", "UnknownItemDescription", "This item is not known to this version of the game.");
		Default->MaxStackCount = 1;
		Default->GridSize = FIntPoint(1, 1);
		TransientUnknownDefinition = Default;
	}
	return TransientUnknownDefinition;
}

bool URockItemRegistrySubsystem::IsUnknownItemDefinition(const URockItemDefinition* Definition) const
{
	return Definition && Definition == GetUnknownItemDefinition();
}

void URockItemRegistrySubsystem::GetAllDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const
{
	OutDefinitions.Reset();
	if (!EnsureUpToDate())
	{
		return;
	}
	for (const TPair<FName, TObjectPtr<URockItemDefinition>>& Pair : ItemDefinitionMap)
	{
		OutDefinitions.Add(Pair.Value);
	}
}
