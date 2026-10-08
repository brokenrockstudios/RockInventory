// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "RockInventorySlotViewModel.h"

#include "Inventory/RockInventory.h"
#include "UI/Shared/RockInventoryModelAccess.h"

void URockInventorySlotViewModel::Initialize(URockInventory* NewInventory, const FRockInventorySlotHandle& NewSlot)
{
	if (Inventory != NewInventory || SlotHandle != NewSlot)
	{
		if (Model)
		{
			Model->OnChanged.RemoveAll(this);
		}

		Inventory = NewInventory;
		SlotHandle = NewSlot;
		Model = RockInventoryUI::ModelOf(Inventory);
		ItemHandle = Model ? Model->GetSlotByHandle(SlotHandle).ItemHandle : FRockItemStackHandle();

		if (Model)
		{
			Model->OnChanged.AddUObject(this, &ThisClass::OnModelChanged);
		}

		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Inventory);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SlotHandle);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemHandle);
		// Possibly ALL the getters changed. So we need to notify them all
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStackCount);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMaxStackCount);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemDefinition);
	}
}

int32 URockInventorySlotViewModel::GetStackCount() const
{
	return Model ? Model->GetItemBySlotHandle(SlotHandle).GetStackCount() : 0;
}

int32 URockInventorySlotViewModel::GetMaxStackCount() const
{
	return Model ? Model->GetItemBySlotHandle(SlotHandle).GetMaxStackCount() : 0;
}

URockItemDefinition* URockInventorySlotViewModel::GetItemDefinition() const
{
	return Model ? Model->GetItemBySlotHandle(SlotHandle).GetDefinition() : nullptr;
}

void URockInventorySlotViewModel::OnModelChanged(URockInventoryClientModel& Changed, const FRockInventoryPresentationDiff& Diff)
{
	const FRockItemStackHandle NewItemHandle = Changed.GetSlotByHandle(SlotHandle).ItemHandle;
	const bool bSlotChanged = Diff.bLayoutChanged || Diff.ChangedSlots.Contains(SlotHandle);
	const bool bStackChanged = NewItemHandle.IsValid() && Diff.StacksModified.Contains(NewItemHandle);
	if (!bSlotChanged && !bStackChanged)
	{
		return;
	}
	if (ItemHandle != NewItemHandle)
	{
		ItemHandle = NewItemHandle;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemHandle);
	}
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStackCount);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMaxStackCount);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetItemDefinition);
}
