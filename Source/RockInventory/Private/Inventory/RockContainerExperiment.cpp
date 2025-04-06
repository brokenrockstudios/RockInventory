// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Inventory/RockContainerExperiment.h"

#include "Inventory/RockInventorySlot.h"
#include "Net/UnrealNetwork.h"

const FRockInventoryTabInfo* URockInventory::GetTabInfo(int32 TabIndex) const
{
	if (TabIndex >= 0 && TabIndex < Tabs.Num())
	{
		return &Tabs[TabIndex];
	}
	return nullptr;
}

int32 URockInventory::GetSlotIndex(int32 TabIndex, int32 X, int32 Y) const
{
	const FRockInventoryTabInfo* TabInfo = GetTabInfo(TabIndex);
	if (TabInfo && X >= 0 && X < TabInfo->Width && Y >= 0 && Y < TabInfo->Height)
	{
		return TabInfo->FirstSlotIndex + (Y * TabInfo->Width + X);
	}
	return INDEX_NONE;
}

FRockInventorySlot* URockInventory::GetSlotAt(int32 TabIndex, int32 X, int32 Y)
{
	TArray<FRockInventorySlot>& AllSlots = InventoryData.AllSlots;

	int32 SlotIndex = GetSlotIndex(TabIndex, X, Y);
	if (SlotIndex != INDEX_NONE && SlotIndex < AllSlots.Num())
	{
		return &AllSlots[SlotIndex];
	}
	return nullptr;
}

int32 URockInventory::AddTab(FName TabID, int32 Width, int32 Height)
{
	TArray<FRockInventorySlot>& AllSlots = InventoryData.AllSlots;

	FRockInventoryTabInfo NewTab;
	NewTab.TabID = TabID;
	NewTab.Width = Width;
	NewTab.Height = Height;
	NewTab.NumSlots = Width * Height;
	NewTab.FirstSlotIndex = AllSlots.Num(); // Current size is the first index

	int32 TabIndex = Tabs.Add(NewTab);

	// Reserve space for the new tab's slots
	int32 OldSize = AllSlots.Num();
	AllSlots.AddUninitialized(NewTab.NumSlots);

	// Initialize each slot
	for (int32 i = 0; i < NewTab.NumSlots; i++)
	{
		int32 SlotArrayIndex = OldSize + i;
		int32 X = i % Width;
		int32 Y = i / Width;

		FRockInventorySlot& Slot = AllSlots[SlotArrayIndex];
		Slot.TabIndex = TabIndex;
		Slot.GridX = X;
		Slot.GridY = Y;
	}

	return TabIndex;
}

TArrayView<FRockInventorySlot> URockInventory::GetTabSlots(int32 TabIndex)
{
	const FRockInventoryTabInfo* TabInfo = GetTabInfo(TabIndex);
	if (TabInfo && TabInfo->NumSlots > 0)
	{
		return TArrayView<FRockInventorySlot>(&InventoryData.AllSlots[TabInfo->FirstSlotIndex], TabInfo->NumSlots);
	}
	return TArrayView<FRockInventorySlot>();
}

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
	DOREPLIFETIME(URockInventory, InventoryData);
	DOREPLIFETIME(URockInventory, Tabs);
}
