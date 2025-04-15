// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventory.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventorySectionInfo.h"
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
			SlotData[SlotIndex].SlotHandle = FRockInventorySlotHandle(SectionIndex, AbsoluteSlotIndex);
			SlotData[SlotIndex].ItemHandle = FRockItemStackHandle::Invalid();
			SlotData[SlotIndex].Orientation = ERockItemOrientation::Horizontal;
			SlotData[SlotIndex].bIsLocked = false;
		}
	}
}

const FRockInventorySectionInfo* URockInventory::GetTabInfo(int32 TabIndex) const
{
	if (TabIndex >= 0 && TabIndex < SlotSections.Num())
	{
		return &SlotSections[TabIndex];
	}
	return nullptr;
}

int32 URockInventory::FindSectionIndex(const FName& SectionName) const
{
	for (int32 i = 0; i < SlotSections.Num(); i++)
	{
		if (SlotSections[i].SectionName == SectionName)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FRockInventorySlotEntry URockInventory::GetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	const int32 slotIndex = InSlotHandle.GetIndex();
	if (slotIndex < 0 || slotIndex >= SlotData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetSlotByHandle - Invalid slot index"));
		return FRockInventorySlotEntry();
	}
	return SlotData[slotIndex];
}

FRockInventorySlotEntry& URockInventory::GetSlotRefByHandle(const FRockInventorySlotHandle& InSlotHandle)
{
	const int32 slotIndex = InSlotHandle.GetIndex();
	// If the handle was invalid, it wouldn't be in range
	if (slotIndex < 0 || slotIndex >= SlotData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetSlotRefByHandle - Invalid slot index"));
		return FRockInventorySlotEntry::Invalid();
	}
	return SlotData[slotIndex];
}

FRockInventorySlotEntry URockInventory::GetSlotByAbsoluteIndex(int32 AbsoluteIndex) const
{
	return SlotData[AbsoluteIndex];
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
	FRockItemStack Item = ItemData[InSlotHandle.GetIndex()];
	if (Item.Generation == InSlotHandle.GetGeneration())
	{
		return Item;
	}
	return FRockItemStack::Invalid();
}

void URockInventory::SetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle, const FRockInventorySlotEntry& InSlot)
{
	const int32 SlotIndex = InSlotHandle.GetIndex();
	if (SlotIndex < 0 || SlotIndex >= SlotData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetSlotByHandle - Invalid slot index"));
		return;
	}

	FRockInventorySlotEntry& Slot = SlotData[SlotIndex];
	if (Slot.IsValid())
	{
		Slot = InSlot;
		SlotData.MarkItemDirty(Slot);
	}
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
		checkf(0 <= AbsoluteSlotIndex && AbsoluteSlotIndex < SlotData.Num(), TEXT("Absolute slot index out of range:"));

		const FRockInventorySlotHandle SlotHandle(TabIndex, AbsoluteSlotIndex);
		SlotData[AbsoluteSlotIndex].SlotHandle = SlotHandle;
	}

	return TabIndex;
}

int32 URockInventory::GetSectionIndexByID(const FName& SectionName) const
{
	for (int32 SlotIndex = 0; SlotIndex < SlotSections.Num(); SlotIndex++)
	{
		if (SlotSections[SlotIndex].SectionName == SectionName)
		{
			return SlotIndex;
		}
	}
	return INDEX_NONE;
}

void URockInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockInventory, ItemData);
	DOREPLIFETIME(URockInventory, SlotData);
	DOREPLIFETIME(URockInventory, SlotSections);
}

void URockInventory::BroadcastInventoryChanged(const FRockInventorySlotHandle& SlotHandle)
{
	// TODO: This is just placeholder, will need to be updated to use the actual slot handle
	OnInventoryChanged.Broadcast(this, SlotHandle);
}

FString URockInventory::GetDebugString() const
{
	// Is there a better 'name' for this inventory?
	return GetName();
}

FRockItemStackHandle URockInventory::AddItemToInventory(const FRockItemStack& ItemStack)
{
	if (!ItemStack.IsValid())
	{
		UE_LOG(LogRockInventory, Error, TEXT("Attempted to add an invalid item"));
		return FRockItemStackHandle::Invalid();
	}

	uint32 Index = INDEX_NONE;

	if (FreeIndices.Num() > 0)
	{
		Index = FreeIndices.Pop(EAllowShrinking::No);
		// Should we increase the generation here?
	}
	else if (ItemData.Num() <= SlotData.Num())
	{
		Index = ItemData.AddDefaulted();
		ItemData[Index].Generation = 0;
	}
	else
	{
		UE_LOG(LogRockInventory, Error, TEXT("Inventory is full. Something went wrong!"));
		return FRockItemStackHandle::Invalid();
	}

	// Set up the item
	ItemData[Index] = ItemStack;
	ItemData[Index].bIsOccupied = true;

	ItemData.MarkItemDirty(ItemData[Index]);

	// Return handle with current index and generation
	return FRockItemStackHandle::Create(Index, ItemData[Index].Generation);
}
