// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "Transactions/Core/RockSlotExpectation.h"

#include "Inventory/RockInventory.h"
#include "Inventory/RockInventoryData.h"

FRockSlotExpectation FRockSlotExpectation::FromStack(const FRockItemStack* Stack, ERockItemOrientation SlotOrientation)
{
	FRockSlotExpectation Result;
	Result.bCheck = true;
	Result.bEmpty = !Stack || !Stack->IsValid();
	if (!Result.bEmpty)
	{
		Result.Definition = Stack->GetDefinition();
		Result.Count = Stack->GetStackCount();
		Result.CustomValue1 = Stack->GetCustomValue1();
		Result.CustomValue2 = Stack->GetCustomValue2();
		Result.Orientation = SlotOrientation;
	}
	return Result;
}

bool FRockSlotExpectation::MatchesStack(const FRockItemStack* Stack, ERockItemOrientation SlotOrientation) const
{
	if (!bCheck)
	{
		return true;
	}
	const bool bStackEmpty = !Stack || !Stack->IsValid();
	if (bStackEmpty || bEmpty)
	{
		return bStackEmpty && bEmpty;
	}
	return Stack->GetDefinition() == Definition
		&& Stack->GetStackCount() == Count
		&& Stack->GetCustomValue1() == CustomValue1
		&& Stack->GetCustomValue2() == CustomValue2
		&& SlotOrientation == Orientation;
}

FRockSlotExpectation FRockSlotExpectation::Capture(const FRockInventoryData& Data, const FRockInventorySlotHandle& Slot)
{
	const FRockInventorySlotEntry* Entry = Data.GetSlot(Slot);
	return FromStack(Data.GetSlotStack(Slot), Entry ? Entry->Orientation : ERockItemOrientation::Horizontal);
}

FRockSlotExpectation FRockSlotExpectation::Capture(const URockInventory& Inventory, const FRockInventorySlotHandle& Slot)
{
	const FRockItemStack Stack = Inventory.GetItemBySlotHandle(Slot);
	return FromStack(&Stack, Inventory.GetSlotByHandle(Slot).Orientation);
}

bool FRockSlotExpectation::Matches(const FRockInventoryData& Data, const FRockInventorySlotHandle& Slot) const
{
	const FRockInventorySlotEntry* Entry = Data.GetSlot(Slot);
	return MatchesStack(Data.GetSlotStack(Slot), Entry ? Entry->Orientation : ERockItemOrientation::Horizontal);
}

bool FRockSlotExpectation::Matches(const URockInventory& Inventory, const FRockInventorySlotHandle& Slot) const
{
	if (!bCheck)
	{
		return true;
	}
	const FRockItemStack Stack = Inventory.GetItemBySlotHandle(Slot);
	return MatchesStack(&Stack, Inventory.GetSlotByHandle(Slot).Orientation);
}
