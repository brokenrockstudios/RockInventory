// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enums/RockEnums.h"
#include "Inventory/RockInventoryQuery.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemStack.h"
#include "Item/RockMoveItemParams.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventoryLibrary.generated.h"

class URockInventoryComponent;
class URockInventory;
struct FRockLootScratch;

/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Core Item
	// Every mutator in this library runs on the authority only: a call on a client logs a warning, changes nothing and fails.
	/**
	 * Add it from anywhere. This merges into existing stacks first, then places what is left as a new stack in the first slot it fits.
	 * We also 'fully initialize' any items not initialized (e.g. create their runtime instances).
	 * @param Params - What the call may do (see FRockLootParams)
	 * @param OutResult - Every placement made and the excess. Always filled; on failure Excess is the whole stack and Placements is empty.
	 * @return True when the whole stack was placed
	 */
	static bool LootItemToInventory(URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params, FRockLootResult& OutResult);

	/**
	 * Read-only: the placements LootItemToInventory would make right now with the same params. Changes nothing and needs no authority,
	 * so a client can ask. The server stays authoritative and may differ after a race. Invalid input returns the whole stack as excess, silently.
	 */
	static FRockLootResult PreviewLoot(const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params = FRockLootParams());

	/**
	 * Read-only: PreviewLoot for several stacks looted one after another with the same params, one result per input stack in input order.
	 * Each stack is simulated against a scratch copy of the occupancy, the partial stacks and the new stacks the earlier ones would create, so a "take all"
	 * accounts for earlier stacks taking space and topping up stacks. Matches calling LootItemToInventory for each stack in order. Invalid stacks get the
	 * whole stack as excess and change nothing. bAllowSwap is ignored here (a swap needs the live occupancy, which the simulation no longer matches).
	 */
	static TArray<FRockLootResult> PreviewLoot(const URockInventory* Inventory, const TArray<FRockItemStack>& ItemStacks, const FRockLootParams& Params = FRockLootParams());

	/**
	 * The sections a loot call would try, in order (read-only, no allocation for up to 16 sections). A section is kept when the call's intent shares a bit with
	 * its AcceptedLootIntents, it has slots, no ExcludeSectionMetaTags entry matches and its SectionFilter accepts the item. Order: sections whose LootPreference
	 * matches the item first, then LootPriority (lower first), then config order. Entries are root-inventory sections only.
	 */
	static void BuildLootPlan(const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params, FRockLootPlan& OutPlan);

	/** Debug text for designers: every section of the inventory, in the order the call would try them, or why it is skipped. */
	UFUNCTION(BlueprintPure, Category = "Rock Inventory")
	static FString DescribeLootPlan(const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params);

	//////////////////////////////////////////////////////////////////////////
	/// Inventory Location Manipulation

	/** Remove an item at a specific location
	 * @param Inventory - The inventory to remove the item from
	 * @param SlotHandle - The handle of the slot to remove the item from
	 * @param Quantity - The quantity of the item to remove. If -1, remove the entire stack
	 * @return The item stack that was removed
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static FRockItemStack SplitItemStackAtLocation(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle, int32 Quantity = -1);

	/**
	 * Move an item from one inventory to another
	 * @param SourceInventory - The source inventory
	 * @param SourceSlotHandle - The handle of the source slot
	 * @param TargetInventory - The target inventory
	 * @param TargetSlotHandle - The handle of the target slot
	 * @param InMoveParams - The move parameters
	 * @return true if the move was successful
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static bool MoveItem(
		URockInventory* SourceInventory, const FRockInventorySlotHandle& SourceSlotHandle,
		URockInventory* TargetInventory, const FRockInventorySlotHandle& TargetSlotHandle,
		const FRockMoveItemParams& InMoveParams = FRockMoveItemParams());

	// Misc helpers

	static bool CanMergeItemAtGridPosition(
		const URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack,
		ERockItemStackMergeCondition MergeCondition = ERockItemStackMergeCondition::Full);
	static int32 MergeItemAtGridPosition(URockInventory* Inventory, FRockInventorySlotHandle SlotHandle, const FRockItemStack& ItemStack);

	/** Get the item by the Slot Handle
	 * @param Inventory - The inventory to get the item from
	 * @param SlotHandle - The handle of the slot to get the item from
	 * @return The item stack at the specified location
	 */
	UFUNCTION(BlueprintCallable)
	static FRockItemStack GetItemBySlotHandle(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle);

	/** Get the item by the Item Handle
	 * @param Inventory - The inventory to get the item from
	 * @param SlotHandle - The handle of the item to get
	 * @return The item stack at the specified location
	 */
	UFUNCTION(BlueprintCallable)
	static FRockItemStack GetItemByItemHandle(URockInventory* Inventory, const FRockItemStackHandle& SlotHandle);

	/** Get the item count in an inventory
	 * @param Inventory - The inventory to get the item from
	 * @param ItemId - The ItemID for which you want to count
	 * @return The item stack at the specified location
	 */
	UFUNCTION(BlueprintCallable)
	static int32 GetItemCount(const URockInventory* Inventory, const FName& ItemId);

	/**
	 * Checks if an item can be placed in a section based on its type restrictions
	 * @param ItemStack - The item stack to check
	 * @param SectionInfo - The section info to check against
	 * @return True if the item can be placed in the section
	 */
	static bool CanItemBePlacedInSection(
		const FRockItemStack& ItemStack,
		const FRockInventorySectionInfo& SectionInfo);

	//////////////////////////////////////////////////////////////////////////
	// Misc Utility functions
	static void PrecomputeOccupancyGrids(
		const URockInventory* Inventory, TArray<bool>& OutOccupancyGrid, FRockItemStackHandle IgnoreItemHandle = FRockItemStackHandle());
	static bool CanItemFitInGridPosition(
		const TArray<bool>& OccupancyGrid, const FRockInventorySectionInfo& TabInfo, int32 X, int32 Y, const FVector2D& ItemSize);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	static TArray<FString> GetInventoryContentsDebug(const URockInventory* Inventory);

	// This needs to point to where the inventory is registered for replication
	static UObject* GetTopLevelOwner(UObject* Instance);

	/** Searches the passed in actor for an Inventory, will use interface or fall back to a component search */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Components")
	static URockInventory* GetInventory(AActor* Actor, bool bFindComponentByClass = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Components")
	static int32 GetSlotIndex(const FRockInventorySlotHandle& SlotHandle);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static void SetCustomValue1(URockInventory* Inventory, const FRockItemStackHandle& ItemHandle, int32 NewValue);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	static void SetCustomValue2(URockInventory* Inventory, const FRockItemStackHandle& ItemHandle, int32 NewValue);

	// The header section tag
	UFUNCTION(BlueprintCallable)
	static FRockInventorySlotHandle FindFirstSlotInSection(URockInventory* Inventory, FGameplayTag SectionTag);
	UFUNCTION(BlueprintCallable)
	static TArray<FRockInventorySlotHandle> FindAllSlotsInSection(URockInventory* Inventory, FGameplayTag SectionTag);

	// The internal meta-tags
	UFUNCTION(BlueprintCallable)
	static FRockInventorySlotHandle FindFirstSlotInSectionWithMetaTag(URockInventory* Inventory, FGameplayTag SectionMetaTag);
	UFUNCTION(BlueprintCallable)
	static TArray<FRockInventorySlotHandle> FindAllSlotsInSectionsWithMetaTag(URockInventory* Inventory, FGameplayTag SectionMetaTag);

private:
	/** Slots another operation holds (first listed operation per slot counts). Empty when nothing is pending. */
	static TBitArray<> BuildPendingSlots(const URockInventory* Inventory);
	/** Pass 1: top up partial stacks across the plan. SkipAbsoluteIndex names a stack that is leaving (INDEX_NONE for none). Scratch (batch preview) adds what earlier stacks would have put there. */
	static void DecideMerges(
		const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootPlan& Plan, const TBitArray<>& PendingSlots,
		int32 SkipAbsoluteIndex, TArray<FRockLootPlacement>& OutPlacements, int32& InOutRemaining, FRockLootScratch* Scratch = nullptr);
	/** Pass 2: the remainder becomes one new stack in the first slot of the plan that fits; marks its footprint in the grid. False when nothing fits. */
	static bool DecideNewStack(
		const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootPlan& Plan, const TBitArray<>& PendingSlots,
		TArray<bool>& InOutGrid, TArray<FRockLootPlacement>& OutPlacements, int32& InOutRemaining, FRockLootScratch* Scratch = nullptr);
	/** Equip with swap (FRockLootParams::CanSwap). Fills OutResult and returns true only when the displaced stack is fully stored too; otherwise OutResult is untouched. */
	static bool DecideSwap(
		const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params, const FRockLootPlan& Plan,
		const TBitArray<>& PendingSlots, FRockLootResult& OutResult);
	/** Applies placements of one stack in order. */
	static void ApplyPlacements(URockInventory* Inventory, const FRockItemStack& ItemStack, const TArray<FRockLootPlacement>& Placements);

	/** Works out where the stack goes without changing the inventory: BuildLootPlan, then merges into partial stacks across the plan, then one new stack in the first slot that fits, then (Equip-only with bAllowSwap, nothing placed) a swap. Fills Placements and Excess. */
	static void DecideLoot(const URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootParams& Params, FRockLootResult& OutResult, FRockLootScratch* Scratch = nullptr);
	/** Applies a decision from DecideLoot, in placement order. A swap removes the displaced stack first, places the new one, then the displaced one. */
	static void CommitLoot(URockInventory* Inventory, const FRockItemStack& ItemStack, const FRockLootResult& Decision);
};
