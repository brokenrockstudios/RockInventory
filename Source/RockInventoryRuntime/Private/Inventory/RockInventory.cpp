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

	return TabIndex;
}


void URockInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockInventory, ItemData);
	DOREPLIFETIME(URockInventory, SlotData);
	DOREPLIFETIME(URockInventory, SlotSections);
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
	if (!InItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Error, TEXT("Attempted to add an invalid item"));
		return FRockItemStackHandle::Invalid();
	}
	FRockItemStack ItemStack = InItemStack;

	uint32 Index = INDEX_NONE;

	if (FreeIndices.Num() > 0)
	{
		Index = FreeIndices.Pop(EAllowShrinking::No);
		// Should we increase the generation here?
	}
	else if (ItemData.Num() <= SlotData.Num())
	{
		Index = ItemData.AddDefaulted();
		ItemStack.Generation = 0;
	}
	else
	{
		UE_LOG(LogRockInventory, Error, TEXT("Inventory is full. Something went wrong!"));
		return FRockItemStackHandle::Invalid();
	}


	// Initialize the item stack
	if (ItemStack.Definition->bRequiresRuntimeInstance)
	{
		// The outer should be the inventory that owns this item stack?
		ItemStack.RuntimeInstance = NewObject<URockItemInstance>(this);
		if (ItemStack.RuntimeInstance)
		{
			ItemStack.RuntimeInstance->OwningInventory = this;
		}
		else
		{
			UE_LOG(LogRockInventory, Error, TEXT("Failed to create runtime instance for item stack %s"),
				*ItemStack.GetDebugString());
		}
	}

	// Set up the item
	ItemStack.bIsOccupied = true;
	ItemData[Index] = ItemStack;
	ItemData.MarkItemDirty(ItemStack);

	// Return handle with current index and generation
	return FRockItemStackHandle::Create(Index, ItemStack.Generation);
}
