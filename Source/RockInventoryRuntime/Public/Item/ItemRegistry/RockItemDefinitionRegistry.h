// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "UObject/PrimaryAssetId.h"
#include "RockItemDefinitionRegistry.generated.h"

class URockItemDefinition;
/**
 * A central registry system that manages all available item definitions in the game.
 * This subsystem loads and provides access to all URockItemDefinition assets,
 * allowing for efficient lookup by ItemID throughout the game.
 *
 * It is an engine subsystem: the data is global and read-only, so one copy serves every game instance
 * (PIE clients, test worlds) and it works without a world (editor tools, validators, commandlets).
 *
 * The registry is built lazily on the first lookup. After that, MarkDirty() makes the next lookup do an
 * additive refresh: definitions that are already loaded are kept, only new asset ids are loaded and
 * removed ones are dropped. In the editor the RockInventoryEditor module calls MarkDirty() on PIE start
 * and when item definition assets are added, removed or renamed.
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockItemRegistrySubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	/** The registry, or nullptr when the engine is not available (shutting down). Needs no world. */
	static URockItemRegistrySubsystem* GetInstance();

	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	/**
	 * Finds an item definition by its unique ItemId.
	 *
	 * @param ItemID The FName identifier of the item definition to find.
	 * @return A pointer to the URockItemDefinition if found, otherwise nullptr.
	 */
	UFUNCTION(BlueprintPure, Category = "Item Registry") // Expose to Blueprint if needed
	URockItemDefinition* FindDefinition(FName ItemID) const;

	/**
	 * Gets all loaded item definitions.
	 * Useful for displaying all available items in UI or debug tools.
	 *
	 * @param OutDefinitions Array to populate with all found definitions.
	 */
	UFUNCTION(BlueprintPure, Category = "Item Registry") // Expose to Blueprint if needed
	void GetAllDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const;

	/** The next lookup refreshes the registry against the Asset Manager. Cheap; safe to call from anywhere. */
	void MarkDirty();

private:
	/** The loaded definition for every item definition primary asset, kept alive by this map. */
	UPROPERTY(Transient)
	TMap<FPrimaryAssetId, TObjectPtr<URockItemDefinition>> LoadedById;

	/** ItemId -> ItemDefinition associations for quick lookup. Derived from LoadedById on every refresh. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<URockItemDefinition>> ItemDefinitionMap;

	/** Primary Asset Type for URockItemDefinition as configured in Project Settings. */
	UPROPERTY()
	FPrimaryAssetType ItemDefinitionAssetType = FPrimaryAssetType(TEXT("RockItemDefinition"));

	/** True until a refresh has completed against an initialized Asset Manager. */
	bool bDirty = true;

	/** Refreshes if dirty. Returns false when the Asset Manager is not ready yet (the registry stays dirty). */
	bool EnsureUpToDate() const;

	/** Diffs the Asset Manager's item definition ids against LoadedById: loads new ones, drops removed ones, rebuilds the lookup map. */
	void Refresh();
};
