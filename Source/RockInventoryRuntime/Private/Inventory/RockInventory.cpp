// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Inventory/RockInventory.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventoryTabInfo.h"
#include "Item/RockItemInstance.h"
#include "Net/UnrealNetwork.h"


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
	InventoryData.Empty();
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
	InventoryData.SetNum(totalInventorySlots);
	if (InventoryData.Num() == 0)
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
				checkf(0 <= SlotIndex && SlotIndex < InventoryData.Num(),
					TEXT("Slot index out of range: %d (max: %d)"), SlotIndex, InventoryData.Num() - 1);
				
				const FRockInventorySlotHandle SlotHandle(tabIndex, X, Y);
				InventoryData[SlotIndex].SlotHandle = SlotHandle;
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

FRockInventorySlot* URockInventory::GetSlotByHandle(FRockInventorySlotHandle SlotHandle)
{
	if (SlotHandle.IsValid())
	{
		const int32 TabIndex = SlotHandle.TabIndex;
		const int32 X = SlotHandle.X;
		const int32 Y = SlotHandle.Y;

		return GetSlotAt(TabIndex, X, Y);
	}
	return nullptr;
}

FRockInventorySlot URockInventory::K2_GetSlotByHandle(FRockInventorySlotHandle SlotHandle)
{
	FRockInventorySlot* Slot = GetSlotByHandle(SlotHandle);
	if (Slot)
	{
		return *Slot;
	}
	return FRockInventorySlot();
}

FRockInventorySlot* URockInventory::GetSlotAt(int32 TabIndex, int32 X, int32 Y)
{
	const int32 SlotIndex = GetSlotIndex(TabIndex, X, Y);
	if (SlotIndex != INDEX_NONE && SlotIndex < InventoryData.Num())
	{
		return &InventoryData[SlotIndex];
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

	// We don't have anything to initialize yet, but we could do it here if needed.
	// Initialize each slot
	// for (int32 i = 0; i < NewTab.NumSlots; i++)
	// {
	// 	const int32 SlotArrayIndex = OldSize + i;
	// 	const int32 X = i % Width;
	// 	const int32 Y = i / Width;
	//
	// 	FRockInventorySlotHandle SlotHandle = FRockInventorySlotHandle(TabIndex, X, Y);
	// 	//FRockInventorySlot& Slot =
	// 	//InventoryData[SlotArrayIndex] = FRockInventorySlot(SlotHandle);
	// }

	return TabIndex;
}

TArrayView<FRockInventorySlot> URockInventory::GetTabSlots(int32 TabIndex)
{
	const FRockInventoryTabInfo* TabInfo = GetTabInfo(TabIndex);
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


void URockInventory::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	UObject::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockInventory, InventoryData);
	DOREPLIFETIME(URockInventory, Tabs);
}

void URockInventory::BroadcastInventoryChanged()
{
	// TODO: This is just placeholder, will need to be updated to use the actual slot handle
	OnInventoryChanged.Broadcast(this, FRockInventorySlotHandle::Invalid());
}

FString URockInventory::GetDebugString() const
{
	// Is there a better 'name' for this inventory?
	return GetName();
}

bool URockInventory::MoveItem(
	URockInventory* SourceInventory, FRockInventorySlotHandle SourceSlotHandle, URockInventory* TargetInventory,
	FRockInventorySlotHandle TargetSlotHandle)
{
	if (false //GetOwnerRole() != ROLE_Authority
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
		UE_LOG(LogTemp, Warning, TEXT("Invalid inventory handle"));
		return false;
	}
	// Make sure both slot handles are valid.
	if (!SourceSlotHandle.IsValid() || !TargetSlotHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid slot handle"));
		return false;
	}

	FRockInventorySlot* SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
	if (!SourceSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Item to move from Source Slot"));
		return false;
	}
	if (!SourceSlot->Item.IsValid())
	{
		// source slot isn't valid, so we can't move anything
		return false;
	}
	FRockInventorySlot* TargetSlot = TargetInventory->GetSlotByHandle(TargetSlotHandle);
	if (TargetSlot->Item.IsValid())
	{
		// TODO: We don't support swapping items yet
		// Item in targetslot, so let's just not allow that for now. 
		return false;
	}
	// Can TargetInventory accept this item at the TargetSlot?
	// TODO: If we wanted to merge with an existing item, we could do that here, but how to reconcile runtime instances?
	// if (SourceSlot->Item.RuntimeInstance)
	// {
	// TODO: Add support for dealing with  
	//	return false;
	// }


	if (SourceSlot->Item.RuntimeInstance)
	{
		SourceSlot->Item.RuntimeInstance->SetOwningInventory(TargetInventory);
		SourceSlot->Item.RuntimeInstance->SetSlotHandle(TargetSlotHandle);
	}
	TargetSlot->Item = SourceSlot->Item;
	SourceSlot->Item.Reset();
	TargetInventory->InventoryData.MarkItemDirty(*TargetSlot);
	SourceInventory->InventoryData.MarkItemDirty(*SourceSlot);

	TargetInventory->OnInventoryChanged.Broadcast(TargetInventory, TargetSlotHandle);
	SourceInventory->OnInventoryChanged.Broadcast(SourceInventory, SourceSlotHandle);

	// TargetSlot->Item.StackSize = SourceSlot->Item.StackSize;
	// SourceSlot->Item.StackSize = 0;
	// TargetSlot->Item.RuntimeInstance = SourceSlot->Item.RuntimeInstance;
	// SourceSlot->Item.RuntimeInstance = nullptr;
	// TargetSlot->Item.Definition = SourceSlot->Item.Definition;
	// SourceSlot->Item.Definition = nullptr;
	//
	//SourceSlot->Item->SetSlotHandle(TargetSlotHandle);

	// Assume TargetSlot requires to be empty at this time.
	// if (TargetSlot && TargetSlot->Item == nullptr)
	// {
	// 	// Move the item from Source to Target
	// 	TargetSlot->Item = SourceSlot->Item;
	// 	SourceSlot->Item = nullptr;
	//
	// 	// Update the slot handles
	// 	TargetSlot->SlotHandle = TargetSlotHandle;
	// 	SourceSlot->SlotHandle = SourceSlotHandle;
	//
	// 	return true;

	return true;
}
