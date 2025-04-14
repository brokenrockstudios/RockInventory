// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventory.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventoryTabInfo.h"
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

	Tabs.Empty();
	Tabs.Reserve(config->InventoryTabs.Num()); // Reserve space for the inventory data
	for (const FRockInventoryTabInfo& TabInfo : config->InventoryTabs)
	{
		FRockInventoryTabInfo NewTab = TabInfo;
		NewTab.FirstSlotIndex = totalInventorySlots;
		NewTab.NumSlots = TabInfo.Width * TabInfo.Height;
		Tabs.Add(NewTab);
		totalInventorySlots += NewTab.NumSlots;
	}
	// ItemData will grow dynamically, and not be preallocated
	SlotData.SetNum(totalInventorySlots);
	if (SlotData.Num() == 0)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - No inventory slots defined"));
		return;
	}

	// Initialize each slot's SlotHandle
	for (int32 tabIndex = 0; tabIndex < Tabs.Num(); tabIndex++)
	{
		const FRockInventoryTabInfo& TabInfo = Tabs[tabIndex];
		for (int32 Y = 0; Y < TabInfo.Height; Y++)
		{
			for (int32 X = 0; X < TabInfo.Width; X++)
			{
				const int32 SlotIndex = TabInfo.FirstSlotIndex + (Y * TabInfo.Width + X);
				checkf(0 <= SlotIndex && SlotIndex < SlotData.Num(),
					TEXT("Slot index out of range: %d (max: %d)"), SlotIndex, SlotData.Num() - 1);

				const FRockInventorySlotHandle SlotHandle(tabIndex, X, Y);
				const int32 absoluteIndex = GetAbsoluteSlotIndex(SlotHandle);

				FRockInventorySlotEntry& Slot = SlotData[absoluteIndex];
				Slot.SlotHandle = SlotHandle;
			}
		}
	}
}

const FRockInventoryTabInfo* URockInventory::GetTabInfo(int32 TabIndex) const
{
	if (TabIndex >= 0 && TabIndex < Tabs.Num())
	{
		return &Tabs[TabIndex];
	}
	return nullptr;
}

int32 URockInventory::FindTabIndex(const FName& TabName) const
{
	for (int32 i = 0; i < Tabs.Num(); i++)
	{
		if (Tabs[i].TabID == TabName)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FRockInventorySlotEntry URockInventory::GetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	const int32 absoluteIndex = GetAbsoluteSlotIndex(InSlotHandle);
	// If the handle was invalid, it wouldn't be in range
	if (absoluteIndex < 0 || absoluteIndex >= SlotData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetSlotByHandle - Invalid slot index"));
		return FRockInventorySlotEntry();
	}
	return SlotData[absoluteIndex];
}

FRockInventorySlotEntry& URockInventory::GetSlotRefByHandle(const FRockInventorySlotHandle& InSlotHandle)
{
	const int32 absoluteIndex = GetAbsoluteSlotIndex(InSlotHandle);
	// If the handle was invalid, it wouldn't be in range
	if (absoluteIndex < 0 || absoluteIndex >= SlotData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("GetSlotRefByHandle - Invalid slot index"));
		return FRockInventorySlotEntry::Invalid();
	}
	return SlotData[absoluteIndex];
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
	FRockItemStack Item = ItemData[InSlotHandle.GetIndex()];
	if (Item.Generation == InSlotHandle.GetGeneration())
	{
		return Item;
	}
	return FRockItemStack::Invalid();
}

void URockInventory::SetSlotByHandle(const FRockInventorySlotHandle& InSlotHandle, const FRockInventorySlotEntry& InSlot)
{
	
	const int32 absoluteIndex = GetAbsoluteSlotIndex(InSlotHandle);
	if (absoluteIndex < 0 || absoluteIndex >= SlotData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetSlotByHandle - Invalid slot index"));
		return;
	}

	FRockInventorySlotEntry& Slot = SlotData[absoluteIndex];
	if (Slot.IsValid())
	{
		Slot = InSlot;
		SlotData.MarkItemDirty(Slot);
	}
}

int32 URockInventory::GetAbsoluteSlotIndex(int32 TabIndex, int32 X, int32 Y) const
{
	const FRockInventoryTabInfo* TabInfo = GetTabInfo(TabIndex);
	if (TabInfo && X >= 0 && X < TabInfo->Width && Y >= 0 && Y < TabInfo->Height)
	{
		return TabInfo->FirstSlotIndex + (Y * TabInfo->Width + X);
	}
	return INDEX_NONE;
}

int32 URockInventory::GetAbsoluteSlotIndex(const FRockInventorySlotHandle& InSlotHandle) const
{
	return GetAbsoluteSlotIndex(InSlotHandle.GetTabIndex(), InSlotHandle.GetX(), InSlotHandle.GetY());
}

int32 URockInventory::AddTab(FName TabID, int32 Width, int32 Height)
{
	FRockInventoryTabInfo NewTab;
	NewTab.TabID = TabID;
	NewTab.Width = Width;
	NewTab.Height = Height;
	NewTab.NumSlots = Width * Height;
	NewTab.FirstSlotIndex = SlotData.Num(); // Current size is the first index

	const int32 TabIndex = Tabs.Add(NewTab);

	// Reserve space for the new tab's slots
	const int32 OldSize = SlotData.Num();
	SlotData.AddUninitialized(NewTab.NumSlots);

	// Initialize each slot's SlotHandle
	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			const int32 SlotIndex = NewTab.FirstSlotIndex + (Y * Width + X);
			checkf(0 <= SlotIndex && SlotIndex < SlotData.Num(),
				TEXT("Slot index out of range: %d (max: %d)"), SlotIndex, SlotData.Num() - 1);

			const FRockInventorySlotHandle SlotHandle(TabIndex, X, Y);
			const int32 absoluteIndex = GetAbsoluteSlotIndex(SlotHandle);
			FRockInventorySlotEntry& Slot = SlotData[absoluteIndex];
			Slot.SlotHandle = SlotHandle;
		}
	}

	return TabIndex;
}

// TArrayView<FRockInventorySlotEntry> URockInventory::GetTabSlots(int32 TabIndex)
// {
	// const FRockInventoryTabInfo* TabInfo = GetTabInfo(TabIndex);
	// if (TabInfo && TabInfo->NumSlots > 0)
	// {
	// 	return TArrayView<FRockInventorySlotEntry>(&SlotData.GetSlot(Tabs, TabInfo->FirstSlotIndex), TabInfo->NumSlots);
	// }
	// return TArrayView<FRockInventorySlotEntry>();
// }

int32 URockInventory::GetTabIndexByID(FName TabID) const
{
	for (int32 i = 0; i < Tabs.Num(); i++)
	{
		if (Tabs[i].TabID == TabID)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void URockInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockInventory, ItemData);
	DOREPLIFETIME(URockInventory, SlotData);
	DOREPLIFETIME(URockInventory, Tabs);
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
