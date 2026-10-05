// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Components/ActorTestSpawner.h"
#include "Inventory/RockInventory.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Item/RockItemDefinition.h"
#include "UObject/StrongObjectPtr.h"

#include "Misc/RockInventoryTags.h"

/**
 * A real URockInventory owned by an actor in a transient game world, plus helpers to lay it out and fill it.
 *
 * AddItemToInventory requires an owning actor with authority, which is why this spawns a world.
 * Everything the fixture creates is kept alive for the lifetime of the fixture, so a GC during a latent test can't pull it out.
 * Declare it as a TEST_CLASS member; CQTest constructs a fresh one per TEST_METHOD.
 */
class FRockInventoryFixture
{
public:
	FActorTestSpawner Spawner;
	AActor* Owner = nullptr;
	URockInventory* Inventory = nullptr;

	/** Lays out the inventory with the given sections, in order. */
	void Init(const TArray<FRockInventorySectionInfo>& Sections);

	/** One section tagged RockInventoryTags::Inventory_Section_Backpack. */
	void InitGrid(int32 Columns, int32 Rows, ERockItemSizePolicy SizePolicy = ERockItemSizePolicy::RespectSize);

	URockItemDefinition* MakeDefinition(FName ItemId, int32 MaxStack = 1, FIntPoint GridSize = FIntPoint(1, 1));

	/** Like MakeDefinition, with ItemTags set and GetAllTags() refreshed (section filters match against those). */
	URockItemDefinition* MakeTaggedDefinition(FName ItemId, const FGameplayTagContainer& ItemTags, int32 MaxStack = 1, FIntPoint GridSize = FIntPoint(1, 1));

	/** A section with the given tag and grid size, a filter that accepts only items carrying at least one of RequiredTags (empty: no filter). */
	static FRockInventorySectionInfo MakeSection(
		const FGameplayTag& SectionTag, int32 Columns, int32 Rows,
		const FGameplayTagContainer& RequiredTags = FGameplayTagContainer(), const FGameplayTagContainer& MetaTags = FGameplayTagContainer());

	/** Absolute slot handle for a cell of the given section. */
	FRockInventorySlotHandle SlotAt(const FGameplayTag& Section, int32 Column, int32 Row) const;
	/** Absolute slot handle for a cell of the Backpack section. */
	FRockInventorySlotHandle SlotAt(int32 Column, int32 Row) const;

	/** Adds an item and assigns it to the given slot, bypassing placement rules. Use for arranging a scenario. */
	FRockItemStackHandle PlaceAt(URockItemDefinition* Definition, int32 Count, FRockInventorySlotHandle Slot);

	/** The real placement path (merge into partial stacks, then first slot that fits). */
	bool Loot(URockItemDefinition* Definition, int32 Count, FRockInventorySlotHandle& OutSlot, int32& OutExcess);

private:
	TArray<TStrongObjectPtr<UObject>> KeepAlive;
};
