// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "RockInventorySlotViewModel.h"

#include "Inventory/RockInventory.h"

void URockInventorySlotViewModel::Initialize(URockInventory* NewInventory, const FRockInventorySlotHandle& NewSlot)
{
	if (Inventory != NewInventory || SlotHandle != NewSlot)
	{
		if (IsValid(Inventory))
		{
			Inventory->OnSlotChanged.RemoveAll(this);
			Inventory->OnItemChanged.RemoveAll(this);
		}
		
		Inventory = NewInventory;
		SlotHandle = NewSlot;
		
		if (IsValid(Inventory))
		{
			Inventory->OnItemChanged.AddDynamic(this, &ThisClass::OnItemChanged);
			Inventory->OnSlotChanged.AddDynamic(this, &ThisClass::OnSlotChanged);
		}
		
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Inventory);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SlotHandle);
		// Possibly ALL the getters changed. So we need to notify them all
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStackCount);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMaxStackCount);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemDefinition);
		
		
		//ResolveItem();
	}
}

int32 URockInventorySlotViewModel::GetStackCount() const
{
	if (IsValid(Inventory))
	{
		return Inventory->GetItemBySlotHandle(SlotHandle).GetStackCount();
	}
	return 0;
}

int32 URockInventorySlotViewModel::GetMaxStackCount() const
{
	if (IsValid(Inventory))
	{
		return Inventory->GetItemBySlotHandle(SlotHandle).GetMaxStackCount();
	}
	return 0;
}

URockItemDefinition* URockInventorySlotViewModel::GetItemDefinition() const
{
	if (IsValid(Inventory))
	{
		return Inventory->GetItemBySlotHandle(SlotHandle).GetDefinition();
	}
	return nullptr;
}

void URockInventorySlotViewModel::OnItemChanged(const FRockItemDelta& ItemDelta)
{
}

void URockInventorySlotViewModel::OnSlotChanged(const FRockSlotDelta& SlotDelta)
{
}
