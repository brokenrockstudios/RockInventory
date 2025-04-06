// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Item/ItemRegistry/RockItemRegistry.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Item/RockItemDefinition.h"

// Define a log category for easier debugging
DEFINE_LOG_CATEGORY_STATIC(LogRockItemRegistry, Log, All);

void URockItemRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogRockItemRegistry, Log, TEXT("Initializing RockItemRegistry..."));
    BuildRegistry();
    bIsInitialized = true;
    UE_LOG(LogRockItemRegistry, Log, TEXT("RockItemRegistry Initialized. Found %d item definitions."), ItemDefinitionMap.Num());
}

void URockItemRegistry::Deinitialize()
{
    UE_LOG(LogRockItemRegistry, Log, TEXT("Deinitializing RockItemRegistry..."));
    ItemDefinitionMap.Empty();
    bIsInitialized = false;
    Super::Deinitialize();
}

void URockItemRegistry::BuildRegistry()
{
    ItemDefinitionMap.Empty(); // Start fresh

    UAssetManager& Manager = UAssetManager::Get();

    // Ensure the configured Primary Asset Type exists
    FPrimaryAssetTypeInfo Info;
    if (!Manager.GetPrimaryAssetTypeInfo(ItemDefinitionAssetType, Info))
    {
         UE_LOG(LogRockItemRegistry, Error, TEXT("Primary Asset Type '%s' not found in Asset Manager settings. Cannot load item definitions."), *ItemDefinitionAssetType.ToString());
         return;
    }

    // Get all Primary Asset IDs for our item type
    TArray<FPrimaryAssetId> PrimaryAssetIds;
    Manager.GetPrimaryAssetIdList(ItemDefinitionAssetType, PrimaryAssetIds);

    UE_LOG(LogRockItemRegistry, Log, TEXT("Scanning for Primary Assets of type '%s'. Found %d potential assets."), *ItemDefinitionAssetType.ToString(), PrimaryAssetIds.Num());

    for (const FPrimaryAssetId& AssetId : PrimaryAssetIds)
    {
        // Attempt to synchronously load the asset (since this is initialization)
        // Note: For very large registries, consider asynchronous loading later if startup time becomes an issue.
        TSharedPtr<FStreamableHandle> Handle = Manager.LoadPrimaryAsset(AssetId, TArray<FName>(), FStreamableDelegate());//, FStreamableManager::DefaultAsyncLoadPriority);

        if (Handle.IsValid())
        {
             // Block until loading is complete for initialization.
             // Handle->WaitUntilComplete(); // Alternatively, just get the object directly if already loaded or load sync

             // Get the loaded asset object
             // Using GetPrimaryAssetObject is safer as it handles assets potentially not loaded yet.
             UObject* LoadedAsset = Manager.GetPrimaryAssetObject(AssetId);

             if (URockItemDefinition* ItemDef = Cast<URockItemDefinition>(LoadedAsset))
             {
                 if (!ItemDef->ItemId.IsNone())
                 {
                     if (ItemDefinitionMap.Contains(ItemDef->ItemId))
                     {
                         // Duplicate ItemId found! This is usually an error in data setup.
                         URockItemDefinition* ExistingDef = ItemDefinitionMap[ItemDef->ItemId];
                         UE_LOG(LogRockItemRegistry, Error, TEXT("Duplicate ItemId '%s' found! Asset '%s' conflicts with existing asset '%s'. Ignoring the new one."),
                                *ItemDef->ItemId.ToString(),
                                *GetPathNameSafe(ItemDef),
                                *GetPathNameSafe(ExistingDef));
                     }
                     else
                     {
                         // Add the valid definition to the map
                         ItemDefinitionMap.Add(ItemDef->ItemId, ItemDef);
                         UE_LOG(LogRockItemRegistry, Verbose, TEXT("Added Item Definition: ID '%s', Asset '%s'"), *ItemDef->ItemId.ToString(), *GetPathNameSafe(ItemDef));
                     }
                 }
                 else
                 {
                     UE_LOG(LogRockItemRegistry, Warning, TEXT("Item Definition asset '%s' has a None ItemId. Skipping."), *GetPathNameSafe(ItemDef));
                 }
             }
             else if(LoadedAsset) // Asset loaded but failed to cast
             {
                  UE_LOG(LogRockItemRegistry, Warning, TEXT("Asset '%s' associated with PrimaryAssetId '%s' is not a URockItemDefinition. Skipping."), *GetPathNameSafe(LoadedAsset), *AssetId.ToString());
             }
             // else: AssetManager->GetPrimaryAssetObject(AssetId) returned null, might indicate loading failed or asset doesn't exist (Asset Manager should handle internal errors)
        }
        else
        {
            UE_LOG(LogRockItemRegistry, Warning, TEXT("Failed to initiate load for PrimaryAssetId '%s'."), *AssetId.ToString());
        }
    }
}

URockItemDefinition* URockItemRegistry::FindDefinition(FName ItemID) const
{
    if (!bIsInitialized)
    {
        UE_LOG(LogRockItemRegistry, Warning, TEXT("Attempted to FindDefinition before registry was initialized."));
        return nullptr;
    }
    if (ItemID.IsNone())
    {
        UE_LOG(LogRockItemRegistry, Warning, TEXT("Attempted to FindDefinition with None ItemID."));
        return nullptr;
    }

    const TObjectPtr<URockItemDefinition>* FoundDefPtr = ItemDefinitionMap.Find(ItemID);
    if (FoundDefPtr)
    {
        return *FoundDefPtr; // Dereference the TObjectPtr pointer to get the URockItemDefinition*
    }

    UE_LOG(LogRockItemRegistry, Warning, TEXT("Could not find Item Definition with ID '%s'."), *ItemID.ToString());
    return nullptr;
}

void URockItemRegistry::GetAllDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const
{
     if (!bIsInitialized)
    {
        UE_LOG(LogRockItemRegistry, Warning, TEXT("Attempted to GetAllDefinitions before registry was initialized."));
        OutDefinitions.Empty();
        return;
    }
    ItemDefinitionMap.GenerateValueArray(OutDefinitions); // Efficiently get all values from the map
}