// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventory.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "Net/UnrealNetwork.h"

URockInventory::URockInventory(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
}

void URockInventory::Init(const URockInventoryConfig* config)
{
	if (!config)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - Invalid config object"));
		return;
	}
	if (config->InventoryTabs.Num() == 0)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - No inventory tabs defined"));
		return;
	}

	// Initialize the inventory data
	int32 totalInventorySlots = 0;
	SlotData.Empty();
	ItemData.Empty();

	SlotSections.Empty();
	SlotSections.Reserve(config->InventoryTabs.Num()); // Reserve space for the inventory data
	for (const FRockInventorySectionInfo& TabInfo : config->InventoryTabs)
	{
		FRockInventorySectionInfo NewTab = TabInfo;
		NewTab.FirstSlotIndex = totalInventorySlots;
		SlotSections.Add(NewTab);
		totalInventorySlots += NewTab.GetNumSlots();
	}
	// ItemData will grow dynamically, and not be preallocated
	SlotData.SetNum(totalInventorySlots);
	if (SlotData.Num() == 0)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - No inventory slots defined"));
		return;
	}

	for (int32 SectionIndex = 0; SectionIndex < SlotSections.Num(); ++SectionIndex)
	{
		const FRockInventorySectionInfo& TabInfo = SlotSections[SectionIndex];
		const int32 TabOffset = TabInfo.FirstSlotIndex;

		for (int32 SlotIndex = 0; SlotIndex < TabInfo.GetNumSlots(); ++SlotIndex)
		{
			const int32 AbsoluteSlotIndex = TabOffset + SlotIndex;
			SlotData[AbsoluteSlotIndex].SlotHandle = FRockInventorySlotHandle(SectionIndex, AbsoluteSlotIndex);
			SlotData[AbsoluteSlotIndex].ItemHandle = FRockItemStackHandle::Invalid();
			SlotData[AbsoluteSlotIndex].Orientation = ERockItemOrientation::Horizontal;
			SlotData[AbsoluteSlotIndex].bIsLocked = false;
		}
	}

	SlotData.MarkArrayDirty();
	ItemData.MarkArrayDirty();
}

FRockInventorySectionInfo URockInventory::GetSectionInfo(const FName& SectionName) const
{
	if (SectionName != NAME_None)
	{
		for (const FRockInventorySectionInfo& SectionInfo : SlotSections)
		{
			if (SectionInfo.SectionName == SectionName)
			{
				return SectionInfo;
			}
		}
	}
	return FRockInventorySectionInfo::Invalid();
}

int32 URockInventory::GetSectionIndexById(const FName& SectionName) const
{
	if (SectionName != NAME_None)
	{
		for (int32 SlotIndex = 0; SlotIndex < SlotSections.Num(); SlotIndex++)
		{
			if (SlotSections[SlotIndex].SectionName == SectionName)
			{
				return SlotIndex;
			}
		}
	}
	return INDEX_NONE;
}


FRockInventorySlotEntry URockInventory::GetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	const int32 slotIndex = InSlotHandle.GetIndex();
	if (!SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetSlotByHandle - Invalid slot index"));
		return FRockInventorySlotEntry::Invalid();
	}
	return SlotData[slotIndex];
}

FRockItemStack URockInventory::GetItemBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	const FRockInventorySlotEntry& Slot = GetSlotByHandle(InSlotHandle);
	return GetItemByHandle(Slot.ItemHandle);
}

FRockItemStack URockInventory::GetItemByHandle(const FRockItemStackHandle& InSlotHandle) const
{
	if (!InSlotHandle.IsValid())
	{
		return FRockItemStack::Invalid();
	}
	const int32 index = InSlotHandle.GetIndex();
	if (!ItemData.ContainsIndex(index))
	{
		return FRockItemStack::Invalid();
	}
	FRockItemStack Item = ItemData[index];
	if (Item.Generation == InSlotHandle.GetGeneration())
	{
		return Item;
	}
	return FRockItemStack::Invalid();
}

void URockInventory::SetItemByHandle(const FRockItemStackHandle& InSlotHandle, const FRockItemStack& InItemStack)
{
	const int32 slotIndex = InSlotHandle.GetIndex();
	if (!ItemData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetItemByHandle - Invalid item index"));
		return;
	}

	ItemData[slotIndex] = InItemStack;
	ItemData.MarkItemDirty(ItemData[slotIndex]);
	BroadcastItemChanged(InSlotHandle);
}

void URockInventory::SetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle, const FRockInventorySlotEntry& InSlotEntry)
{
	const int32 slotIndex = InSlotHandle.GetIndex();
	if (!SlotData.ContainsIndex(slotIndex))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetSlotByHandle - Invalid slot index"));
		return;
	}

	SlotData[slotIndex] = InSlotEntry;
	SlotData.MarkItemDirty(SlotData[slotIndex]);
	BroadcastSlotChanged(InSlotHandle);
}

int32 URockInventory::AddSection(const FName& SectionName, int32 Width, int32 Height)
{
	FRockInventorySectionInfo NewTab;
	NewTab.SectionName = SectionName;
	NewTab.Width = Width;
	NewTab.Height = Height;
	NewTab.FirstSlotIndex = SlotData.Num(); // Current size is the first index

	const int32 TabIndex = SlotSections.Add(NewTab);

	// Reserve space for the new tab's slots
	SlotData.AddUninitialized(NewTab.GetNumSlots());

	// Initialize each slot's SlotHandle
	for (int32 SlotIndex = 0; SlotIndex < NewTab.GetNumSlots(); ++SlotIndex)
	{
		const int32 AbsoluteSlotIndex = NewTab.FirstSlotIndex + SlotIndex;
		checkf(0 <= AbsoluteSlotIndex && AbsoluteSlotIndex < SlotData.Num(),
			TEXT("Absolute slot index out of range:"));

		const FRockInventorySlotHandle SlotHandle(TabIndex, AbsoluteSlotIndex);
		SlotData[AbsoluteSlotIndex].SlotHandle = SlotHandle;
	}
	SlotData.MarkArrayDirty();

	return TabIndex;
}


void URockInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockInventory, ItemData);
	DOREPLIFETIME(URockInventory, SlotData);
	DOREPLIFETIME(URockInventory, SlotSections);
}

bool URockInventory::IsSupportedForNetworking() const
{
	return true;
}

void URockInventory::BroadcastSlotChanged(const FRockInventorySlotHandle& SlotHandle)
{
	// TODO: This is just placeholder, will need to be updated to use the actual slot handle
	OnInventoryChanged.Broadcast(this, SlotHandle);
}

void URockInventory::BroadcastItemChanged(const FRockItemStackHandle& RockInventorySlotHandle)
{
	// TODO: This is just placeholder, will need to be updated to use the actual item handle
}

FString URockInventory::GetDebugString() const
{
	// Is there a better 'name' for this inventory?
	return GetName();
}

FRockItemStackHandle URockInventory::AddItemToInventory(const FRockItemStack& InItemStack)
{
	// We shouldn't have items without a definition
	checkf(InItemStack.IsValid(), TEXT("AddItemToInventory - Invalid item stack"));
	checkf(InItemStack.Definition, TEXT("AddItemToInventory - Invalid item definition"));

	const uint32 Index = AcquireAvailableItemIndex();
	checkf(Index != INDEX_NONE, TEXT("AddItemToInventory - Failed to acquire item index"));
	
	// Modify a reference
	FRockItemStack& NewItemStack = ItemData[Index];
	// Generation is reconciled in the AcquireAvailableItemData
	NewItemStack.Definition = InItemStack.Definition;
	NewItemStack.StackSize = InItemStack.StackSize;
	NewItemStack.CustomValue1 = InItemStack.CustomValue1;
	NewItemStack.CustomValue2 = InItemStack.CustomValue2;
	NewItemStack.bIsOccupied = true;

	// Initialize the item stack
	if (InItemStack.Definition->bRequiresRuntimeInstance)
	{
		// The outer should be the inventory that owns this item stack?
		NewItemStack.RuntimeInstance = NewObject<URockItemInstance>(this);
		checkf(NewItemStack.RuntimeInstance, TEXT("Failed to create item instance"));
		NewItemStack.RuntimeInstance->OwningInventory = this;
	}
	// Set up the item
	ItemData.MarkItemDirty(ItemData[Index]);
	BroadcastItemChanged(ItemData[Index].ItemHandle);
	
	// Return handle with current index and generation
	return NewItemStack.ItemHandle;
}

uint32 URockInventory::AcquireAvailableItemIndex()
{
	if (FreeIndices.Num() > 0)
	{
		// The item should already have its handle and generation set
		return FreeIndices.Pop(EAllowShrinking::No);
	}
	else if (ItemData.Num() <= SlotData.Num())
	{
		// Generate a new item index
		// Generation gets incremented during the 'release' of an item.
		// Should we grab in 'chunks' like perhaps 5 slots at a time? Though that could cause extra premature replications?
		// though we won't have to trigger array dirty as much?
		const uint32 Index = ItemData.AddDefaulted();
		ItemData[Index].Generation = 0;
		ItemData[Index].ItemHandle = FRockItemStackHandle::Create(Index, 0);

		// Since we modified the array, we need to mark it dirty? Or can we just mark the item only?
		ItemData.MarkItemDirty(ItemData[Index]);
		return Index;
	}
	checkf(false, TEXT("AcquireAvailableItemData - No space left. Inventory is full or not initialized properly."));
	return INDEX_NONE;
}

void URockInventory::ReleaseItemIndex(uint32 InIndex)
{
	if (InIndex != INDEX_NONE && ItemData.ContainsIndex(InIndex))
	{
		FreeIndices.Add(InIndex);
		// Invalidate the item stack
		ItemData[InIndex].Generation++;
		// Update the ItemHandle with new Generation
		ItemData[InIndex].ItemHandle = FRockItemStackHandle::Create(InIndex, ItemData[InIndex].Generation);
		ItemData.MarkItemDirty(ItemData[InIndex]);
		BroadcastItemChanged(ItemData[InIndex].ItemHandle);
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("ReleaseItemIndex - Invalid index"));
	}
}
