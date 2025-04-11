// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/RockInventory.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventoryTabInfo.h"
#include "Item/RockItemInstance.h"
#include "Net/UnrealNetwork.h"

void URockInventory::Init(const URockInventoryConfig *config)
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
	InventoryData.Empty();
	Tabs.Empty();
	Tabs.Reserve(config->InventoryTabs.Num()); // Reserve space for the inventory data
	for (const FRockInventoryTabInfo &TabInfo : config->InventoryTabs)
	{
		FRockInventoryTabInfo NewTab = TabInfo;
		NewTab.FirstSlotIndex = totalInventorySlots;
		NewTab.NumSlots = TabInfo.Width * TabInfo.Height;
		Tabs.Add(NewTab);
		totalInventorySlots += NewTab.NumSlots;
	}
	InventoryData.SetNum(totalInventorySlots);
	if (InventoryData.Num() == 0)
	{
		UE_LOG(LogRockInventory, Error, TEXT("URockInventory::Init - No inventory slots defined"));
		return;
	}

	// Initialize each slot's SlotHandle
	for (int32 tabIndex = 0; tabIndex < Tabs.Num(); tabIndex++)
	{
		const FRockInventoryTabInfo &TabInfo = Tabs[tabIndex];
		for (int32 Y = 0; Y < TabInfo.Height; Y++)
		{
			for (int32 X = 0; X < TabInfo.Width; X++)
			{
				const int32 SlotIndex = TabInfo.FirstSlotIndex + (Y * TabInfo.Width + X);
				checkf(0 <= SlotIndex && SlotIndex < InventoryData.Num(),
					   TEXT("Slot index out of range: %d (max: %d)"), SlotIndex, InventoryData.Num() - 1);

				const FRockInventorySlotHandle SlotHandle(tabIndex, X, Y);
				InventoryData[SlotIndex].SlotHandle = SlotHandle;
			}
		}
	}
}

const FRockInventoryTabInfo *URockInventory::GetTabInfo(int32 TabIndex) const
{
	if (TabIndex >= 0 && TabIndex < Tabs.Num())
	{
		return &Tabs[TabIndex];
	}
	return nullptr;
}

int32 URockInventory::FindTabIndex(const FName &TabName) const
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

FRockInventorySlot URockInventory::GetSlotByHandle(const FRockInventorySlotHandle &InSlotHandle) const
{
	if (InSlotHandle.IsValid())
	{
		return GetSlotAt(InSlotHandle.TabIndex, InSlotHandle.X, InSlotHandle.Y);
	}
	return FRockInventorySlot();
}

void URockInventory::SetSlotByHandle(const FRockInventorySlotHandle &InSlotHandle, const FRockInventorySlot &InSlot)
{
	if (!InSlotHandle.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetSlotByHandle - Invalid slot handle"));
		return;
	}

	const int32 TabIndex = InSlotHandle.TabIndex;
	const int32 X = InSlotHandle.X;
	const int32 Y = InSlotHandle.Y;

	const int32 SlotIndex = GetSlotIndex(TabIndex, X, Y);

	if (SlotIndex == INDEX_NONE || SlotIndex < InventoryData.Num())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("SetSlotByHandle - Invalid slot index: %d (Num: %d)"), SlotIndex, InventoryData.Num());
		return;
	}

	// Note: If we were to directly set the slot, it'd break the internal ReplicationID and ReplicationKey
	FRockInventorySlot &Slot = InventoryData[SlotIndex];
	Slot.Item = InSlot.Item;
	Slot.Orientation = InSlot.Orientation;
	Slot.bIsLocked = InSlot.bIsLocked;
	Slot.SlotHandle = InSlotHandle;
	InventoryData.MarkItemDirty(Slot);
}

FRockInventorySlot URockInventory::GetSlotAt(int32 TabIndex, int32 X, int32 Y) const
{
	const int32 SlotIndex = GetSlotIndex(TabIndex, X, Y);
	if (SlotIndex != INDEX_NONE && SlotIndex < InventoryData.Num())
	{
		return InventoryData[SlotIndex];
	}
	return FRockInventorySlot();
}

int32 URockInventory::GetSlotIndex(int32 TabIndex, int32 X, int32 Y) const
{
	const FRockInventoryTabInfo *TabInfo = GetTabInfo(TabIndex);
	if (TabInfo && X >= 0 && X < TabInfo->Width && Y >= 0 && Y < TabInfo->Height)
	{
		return TabInfo->FirstSlotIndex + (Y * TabInfo->Width + X);
	}
	return INDEX_NONE;
}

int32 URockInventory::AddTab(FName TabID, int32 Width, int32 Height)
{
	FRockInventoryTabInfo NewTab;
	NewTab.TabID = TabID;
	NewTab.Width = Width;
	NewTab.Height = Height;
	NewTab.NumSlots = Width * Height;
	NewTab.FirstSlotIndex = InventoryData.Num(); // Current size is the first index

	const int32 TabIndex = Tabs.Add(NewTab);

	// Reserve space for the new tab's slots
	const int32 OldSize = InventoryData.Num();
	InventoryData.AddUninitialized(NewTab.NumSlots);

	// Initialize each slot's SlotHandle
	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			const int32 SlotIndex = NewTab.FirstSlotIndex + (Y * Width + X);
			checkf(0 <= SlotIndex && SlotIndex < InventoryData.Num(),
				   TEXT("Slot index out of range: %d (max: %d)"), SlotIndex, InventoryData.Num() - 1);

			const FRockInventorySlotHandle SlotHandle(TabIndex, X, Y);
			InventoryData[SlotIndex].SlotHandle = SlotHandle;
		}
	}

	return TabIndex;
}

TArrayView<FRockInventorySlot> URockInventory::GetTabSlots(int32 TabIndex)
{
	const FRockInventoryTabInfo *TabInfo = GetTabInfo(TabIndex);
	if (TabInfo && TabInfo->NumSlots > 0)
	{
		return TArrayView<FRockInventorySlot>(&InventoryData[TabInfo->FirstSlotIndex], TabInfo->NumSlots);
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

void URockInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockInventory, InventoryData);
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

bool URockInventory::MoveItem(
	URockInventory *SourceInventory, FRockInventorySlotHandle SourceSlotHandle, URockInventory *TargetInventory,
	FRockInventorySlotHandle TargetSlotHandle)
{
	if (false // GetOwnerRole() != ROLE_Authority
	)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveItem called on non-authoritative client"));
		// TODO: Maybe we can support 'predictive moves' later on.
		return false;
	}

	// Make sure both inventories are valid.
	// If we were trying to 'drop' the item. Use a different mechanism?
	// Or perhaps does a WorldItem just have it's own inventory?
	if (!SourceInventory || !TargetInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveItem: Invalid inventory handle"));
		return false;
	}
	// Make sure both slot handles are valid.
	if (!SourceSlotHandle.IsValid() || !TargetSlotHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveItem: Invalid slot handle"));
		return false;
	}

	// check source slot
	FRockInventorySlot SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
	if (!SourceSlot.IsValid() || !SourceSlot.Item.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveItem: No Item to move from Source Slot"));
		return false;
	}

	// check target slot
	FRockInventorySlot TargetSlot = TargetInventory->GetSlotByHandle(TargetSlotHandle);
	if (TargetSlot.Item.IsValid())
	{
		// TODO: We don't support swapping items yet
		// Item in targetslot, so let's just not allow that for now.
		return false;
	}

	// update runtime instance reference if needed
	if (SourceSlot.Item.RuntimeInstance)
	{
		SourceSlot.Item.RuntimeInstance->SetOwningInventory(TargetInventory);
		SourceSlot.Item.RuntimeInstance->SetSlotHandle(TargetSlotHandle);
	}

	// perform the move
	TargetSlot.Item = SourceSlot.Item;
	SourceSlot.Item.Reset();

	// update both inventories
	SourceInventory->SetSlotByHandle(SourceSlotHandle, SourceSlot);
	TargetInventory->SetSlotByHandle(TargetSlotHandle, TargetSlot);

	// broadcast changes
	TargetInventory->OnInventoryChanged.Broadcast(TargetInventory, TargetSlotHandle);
	SourceInventory->OnInventoryChanged.Broadcast(SourceInventory, SourceSlotHandle);

	return true;
}
