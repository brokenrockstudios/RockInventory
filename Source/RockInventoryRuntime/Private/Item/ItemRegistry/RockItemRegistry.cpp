// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Item/RockItemDefinition.h"
#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "ProfilingDebugging/ScopedTimers.h"

// Define a log category for easier debugging
DEFINE_LOG_CATEGORY_STATIC(LogRockItemRegistry, Log, All);

URockItemRegistrySubsystem* URockItemRegistrySubsystem::GetInstance()
{
	checkf(GEngine && GEngine->GetWorldContexts().Num() > 0, TEXT("Expected at least one world context"));
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		const UWorld* World = Context.World();
		if (World && World->IsGameWorld())
		{
			if (const UGameInstance* GI = World->GetGameInstance())
			{
				if (URockItemRegistrySubsystem* Subsystem = GI->GetSubsystem<URockItemRegistrySubsystem>())
				{
					return Subsystem;
				}
			}
		}
	}
	checkf(false, TEXT("URockItemRegistrySubsystem::GetInstance() called but no instance found."));
	return nullptr;
}

void URockItemRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogRockItemRegistry, Log, TEXT("Initializing RockItemRegistry..."));
	BuildRegistry();
	bIsInitialized = true;
	UE_LOG(LogRockItemRegistry, Log, TEXT("RockItemRegistry Initialized. Found %d item definitions."), ItemDefinitionMap.Num());
}

void URockItemRegistrySubsystem::Deinitialize()
{
	UE_LOG(LogRockItemRegistry, Log, TEXT("Deinitializing RockItemRegistry..."));
	ItemDefinitionMap.Empty();
	bIsInitialized = false;
	Super::Deinitialize();
}

void URockItemRegistrySubsystem::BuildRegistry()
{
	double TimeBuildingRegistry = 0.0;
	{
		FScopedDurationTimer Timer(TimeBuildingRegistry);

		//
		UE_LOG(LogRockItemRegistry, Log, TEXT("Building 1RockItemRegistry... %d %d"), UAssetManager::Get().IsInitialized(), UAssetManager::Get().HasInitialScanCompleted());
		ItemDefinitionMap.Empty(); // Start fresh
		UAssetManager::Get().ScanPathsSynchronous({TEXT("/Game/RockInventory/Items")});

		FPrimaryAssetType AssetType("RockItemDefinition");
		TArray<FDirectoryPath> Directories;
		FDirectoryPath ItemsDir;
		ItemsDir.Path = TEXT("/Game/RockInventory/Items");
		Directories.Add(ItemsDir);
		//
		// UAssetManager::Get().AddPrimaryAssetTypeInfo(
		// 	FPrimaryAssetTypeInfo(
		// 		AssetType,
		// 		UObject::StaticClass(),  // Use StaticClass first to test if it works
		// 		UObject::StaticClass(),
		// 		true,   // bHasBlueprintClasses
		// 		false,  // bIsEditorOnly
		// 		Directories,
		// 		TArray<FSoftObjectPath>(), // SpecificAssets
		// 		FStreamableDelegate(),
		// 		FPrimaryAssetRules()
		// 	)
		// );

		UE_LOG(LogRockItemRegistry, Log, TEXT("Building 2RockItemRegistry... %d %d"), UAssetManager::Get().IsInitialized(), UAssetManager::Get().HasInitialScanCompleted());
		UE_LOG(LogTemp, Warning, TEXT("Looking for Primary Asset Type: %s"), *FPrimaryAssetType("RockItemDefinition").ToString());
		UE_LOG(LogTemp, Warning, TEXT("Available Types:"));

		TArray<FPrimaryAssetTypeInfo> TypeInfos;
		UAssetManager::Get().GetPrimaryAssetTypeInfoList(TypeInfos);
		FPrimaryAssetTypeInfo TypeInfoRock;
		for (const FPrimaryAssetTypeInfo& TypeInfo : TypeInfos)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - %s"), *TypeInfo.PrimaryAssetType.ToString());
			if (TypeInfo.PrimaryAssetType == FPrimaryAssetType("RockItemDefinition"))
			{
				TypeInfoRock = TypeInfo;
			}
		}

		UAssetManager& Manager = UAssetManager::Get();


		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		TArray<FAssetData> AssetDataList;
		AssetRegistry.GetAssetsByPath(FName("/Game/RockInventory/Items"), AssetDataList, true);
		UE_LOG(LogTemp, Warning, TEXT("Found %d assets in path"), AssetDataList.Num());

		for (const FAssetData& AssetData : AssetDataList)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - %s (Class: %s)"), *AssetData.AssetName.ToString(), *AssetData.AssetClassPath.ToString());
		}
		TArray<FPrimaryAssetId> AssetIds;
		UAssetManager::Get().GetPrimaryAssetIdList(FPrimaryAssetType("RockItemDefinition"), AssetIds);
		UE_LOG(LogTemp, Warning, TEXT("Found %d registered RockItemDefinition assets"), AssetIds.Num());
		for (const FPrimaryAssetId& AssetId : AssetIds)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - %s"), *AssetId.ToString());
		}

		FPrimaryAssetTypeInfo Info3 = TypeInfoRock;
		bool bFound = true;
		// bool bFound = UAssetManager::Get().GetPrimaryAssetTypeInfo(FPrimaryAssetType("RockItemDefinition"), Info3);
		UE_LOG(LogTemp, Warning, TEXT("GetPrimaryAssetTypeInfo returned: %s"), bFound ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Warning, TEXT("Info.PrimaryAssetType: %s"), *Info3.PrimaryAssetType.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Info.AssetBaseClassLoaded: %s"), Info3.AssetBaseClassLoaded ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Warning, TEXT("Info.AssetBaseClassName: %s"), *Info3.GetAssetBaseClass().GetAssetName());
		const FPrimaryAssetTypeInfo& Info4 = Info3;
		const TArray<FDirectoryPath>& thing = Info4.GetDirectories();

		UE_LOG(LogTemp, Warning, TEXT("Info.Directories: %d %d"), bFound, thing.Num());
		for (const FDirectoryPath& Dir : thing)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Directory: %s"), *Dir.Path);
		}
		UE_LOG(LogTemp, Warning, TEXT("Info.bHasBlueprintClasses: %s"), Info3.bHasBlueprintClasses ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Warning, TEXT("Info.bIsEditorOnly: %s"), Info3.bIsEditorOnly ? TEXT("true") : TEXT("false"));


		FString AssetPath = "/Game/RockInventory/Items/DA_Sniper1.DA_Sniper1";
		URockItemDefinition* ItemDef2 = LoadObject<URockItemDefinition>(nullptr, *AssetPath);
		if (ItemDef2)
		{
			UE_LOG(LogTemp, Warning, TEXT("Successfully loaded item definition: %s"), *ItemDef2->GetName());
			UE_LOG(LogTemp, Warning, TEXT("Primary Asset ID: %s"), *ItemDef2->GetPrimaryAssetId().ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load item definition: %s"), *AssetPath);
		}

		// Ensure the configured Primary Asset Type exists
		FPrimaryAssetTypeInfo Info;
		UE_LOG(LogTemp, Warning, TEXT("Attempting to find Primary Asset Type: %s"), *ItemDefinitionAssetType.ToString());
		if (!Manager.GetPrimaryAssetTypeInfo(ItemDefinitionAssetType, Info))
		{
			UE_LOG(LogRockItemRegistry, Error, TEXT("Primary Asset Type '%s' not found in Asset Manager settings. Cannot load item definitions."),
				*ItemDefinitionAssetType.ToString());
			return;
		}

		// Get all Primary Asset IDs for our item type
		TArray<FPrimaryAssetId> PrimaryAssetIds;
		Manager.GetPrimaryAssetIdList(ItemDefinitionAssetType, PrimaryAssetIds);

		UE_LOG(LogRockItemRegistry, Warning, TEXT("Scanning for Primary Assets of type '%s'. Found %d potential assets."),
			*ItemDefinitionAssetType.ToString(), PrimaryAssetIds.Num());

		for (const FPrimaryAssetId& AssetId : PrimaryAssetIds)
		{
			// Attempt to synchronously load the asset (since this is initialization)
			// Note: For very large registries, consider asynchronous loading later if startup time becomes an issue.
			TSharedPtr<FStreamableHandle> Handle = Manager.LoadPrimaryAsset(AssetId, TArray<FName>(), FStreamableDelegate());
			//, FStreamableManager::DefaultAsyncLoadPriority);

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
							UE_LOG(LogRockItemRegistry, Error,
								TEXT("Duplicate ItemId '%s' found! Asset '%s' conflicts with existing asset '%s'. Ignoring the new one."),
								*ItemDef->ItemId.ToString(),
								*GetPathNameSafe(ItemDef),
								*GetPathNameSafe(ExistingDef));
						}
						else
						{
							// Add the valid definition to the map
							ItemDefinitionMap.Add(ItemDef->ItemId, ItemDef);
							UE_LOG(LogRockItemRegistry, Warning, TEXT("Added Item Definition: ID '%s', Asset '%s'"), *ItemDef->ItemId.ToString(),
								*GetPathNameSafe(ItemDef));
						}
					}
					else
					{
						UE_LOG(LogRockItemRegistry, Warning, TEXT("Item Definition asset '%s' has a None ItemId. Skipping."),
							*GetPathNameSafe(ItemDef));
					}
				}
				else if (LoadedAsset) // Asset loaded but failed to cast
				{
					UE_LOG(LogRockItemRegistry, Warning,
						TEXT("Asset '%s' associated with PrimaryAssetId '%s' is not a URockItemDefinition. Skipping."),
						*GetPathNameSafe(LoadedAsset), *AssetId.ToString());
				}
				// else: AssetManager->GetPrimaryAssetObject(AssetId) returned null, might indicate loading failed or asset doesn't exist (Asset Manager should handle internal errors)
			}
			else
			{
				UE_LOG(LogRockItemRegistry, Warning, TEXT("Failed to initiate load for PrimaryAssetId '%s'."), *AssetId.ToString());
			}
		}
	}

	UE_LOG(LogRockItemRegistry, Warning, TEXT("BuildRegistry() took %.3f seconds."), TimeBuildingRegistry);
}

URockItemDefinition* URockItemRegistrySubsystem::FindDefinition(FName ItemID) const
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

void URockItemRegistrySubsystem::GetAllDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const
{
	if (!bIsInitialized)
	{
		UE_LOG(LogRockItemRegistry, Warning, TEXT("Attempted to GetAllDefinitions before registry was initialized."));
		OutDefinitions.Empty();
		return;
	}
	// ItemDefinitionMap.GenerateValueArray(OutDefinitions); // Efficiently get all values from the map
}
