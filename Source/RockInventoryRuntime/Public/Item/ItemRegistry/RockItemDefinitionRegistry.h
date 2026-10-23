// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "UObject/PrimaryAssetId.h"
#include "RockItemDefinitionRegistry.generated.h"

class URockItemDefinition;

/**
 * A source of item definitions beside the primary asset scan (for example definitions built from data records).
 * The registry does not own the provider: unregister it before it is destroyed. The provider keeps its definitions
 * alive between refreshes; the registry holds them while they are registered.
 *
 * To change a definition, edit the same object in place (keep the pointer) and call
 * URockItemRegistrySubsystem::NotifyProviderChanged(); the registry rebuilds its ItemId map on the next lookup.
 */
class ROCKINVENTORYRUNTIME_API IRockItemDefinitionProvider
{
public:
	virtual ~IRockItemDefinitionProvider() = default;

	/** Appends every definition this provider currently offers. */
	virtual void EnumerateItemDefinitions(TArray<URockItemDefinition*>& OutDefinitions) const = 0;

	/** Where a definition came from (a record file, for example), used in duplicate ItemId errors. Default is the object path. */
	virtual FString DescribeItemSource(const URockItemDefinition* Definition) const;
};

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

	/** FindDefinition without the "could not find" warning (callers that handle a miss themselves, such as a stack resolving its ItemId). */
	URockItemDefinition* TryFindDefinition(FName ItemID) const;

	/**
	 * The definition a stack falls back to when the registry does not know its ItemId: 1x1, never stacks, no fragments.
	 * Taken from URockInventoryDeveloperSettings::UnknownItemDefinition, or a transient code default when that is unset.
	 */
	URockItemDefinition* GetUnknownItemDefinition() const;
	/** True for the fallback definition (Definition may be null). */
	bool IsUnknownItemDefinition(const URockItemDefinition* Definition) const;

	/** Broadcast after a refresh and after RegisterForTest: stacks showing the unknown-item placeholder look their ItemId up again. */
	FSimpleMulticastDelegate OnRefreshed;

	/** The next lookup refreshes the registry against the Asset Manager. Cheap; safe to call from anywhere. */
	void MarkDirty();

	/**
	 * Adds a definition provider beside the asset scan. Definitions are taken from the scan first, then from providers
	 * in registration order. A duplicate ItemId logs an error naming both sources and the earlier source wins (so an
	 * asset beats a provider until the asset is deleted). Registering again is a no-op.
	 */
	void RegisterProvider(IRockItemDefinitionProvider* Provider);
	/** Removes a provider; its definitions leave the registry on the next lookup. */
	void UnregisterProvider(IRockItemDefinitionProvider* Provider);
	/** A provider added, removed or edited definitions (including in place). The next lookup rebuilds the ItemId map. */
	void NotifyProviderChanged() { MarkDirty(); }

	/**
	 * Test seam: makes FindDefinition return this definition for its ItemId even though no asset backs it (a NewObject test
	 * definition), so a stack that replicates only its ItemId resolves again. Takes priority over assets. Unregister when done.
	 */
	void RegisterForTest(URockItemDefinition* Definition);
	/** Removes what RegisterForTest added, if that exact definition is still the one registered for its ItemId. */
	void UnregisterForTest(const URockItemDefinition* Definition);

private:
	/** The loaded definition for every item definition primary asset, kept alive by this map. */
	UPROPERTY(Transient)
	TMap<FPrimaryAssetId, TObjectPtr<URockItemDefinition>> LoadedById;

	/** ItemId -> ItemDefinition associations for quick lookup. Derived from LoadedById on every refresh. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<URockItemDefinition>> ItemDefinitionMap;

	/** Registered providers, in registration order. Not owned. */
	TArray<IRockItemDefinitionProvider*> Providers;

	/** Keeps provider definitions alive while they are registered. Rebuilt on every refresh. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<URockItemDefinition>> ProviderDefinitions;

	/** The code default for GetUnknownItemDefinition, created on first use when the developer setting is unset. */
	UPROPERTY(Transient)
	mutable TObjectPtr<URockItemDefinition> TransientUnknownDefinition;

	/** Definitions registered by tests (RegisterForTest), consulted before the assets. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<URockItemDefinition>> TestDefinitions;

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
