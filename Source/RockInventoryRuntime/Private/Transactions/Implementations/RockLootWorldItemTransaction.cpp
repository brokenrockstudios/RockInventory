// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Transactions/Implementations/RockLootWorldItemTransaction.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Library/RockInventoryLibrary.h"
#include "World/RockWorldItemInterface.h"


bool FRockLootWorldItemUndoTransaction::CanUndo()
{
	// no undo of looting world items.
	return false;
}

bool FRockLootWorldItemUndoTransaction::Undo()
{
	if (!bSuccess)
	{
		return false;
	}

	// We can't a 'world loot' at this time.
	return false;
}

bool FRockLootWorldItemTransaction::CanExecute() const
{
	if (!Instigator.IsValid())
	{
		return false;
	}
	if (!TargetInventory)
	{
		return false;
	}
	if (!IsValid(SourceWorldItemActor))
	{
		return false;
	}
	if (!SourceWorldItemActor)
	{
		return false;
	}
	if (!SourceWorldItemActor->GetClass()->ImplementsInterface(URockWorldItemInterface::StaticClass()))
	{
		return false;
	}
	const IRockWorldItemInterface* WorldItemInterfaceActor = Cast<IRockWorldItemInterface>(SourceWorldItemActor);
	if (!WorldItemInterfaceActor)
	{
		UE_LOG(LogRockInventory, Error, TEXT("SourceWorldItemActor %s does not implement URockWorldItemInterface!"), *SourceWorldItemActor->GetName());
		return false;
	}

	// What Item are we looting?
	const FRockItemStack ItemStack = WorldItemInterfaceActor->GetItemStack(Instigator.Get());
	if (!ItemStack.IsValid())
	{
		return false;
	}

	return true;
}

FRockLootWorldItemUndoTransaction FRockLootWorldItemTransaction::Execute()
{
	FRockLootWorldItemUndoTransaction UndoData;
	UndoData.bSuccess = false;
	// Should have been checked by CanExecute
	checkf(Instigator.IsValid(), TEXT("Instigator is not valid"));
	checkf(TargetInventory, TEXT("TargetInventory is not valid"));
	checkf(TargetInventory->GetOwningActor(), TEXT("TargetInventory OwningActor is not valid"));
	checkf(SourceWorldItemActor, TEXT("SourceWorldItemActor is not valid"));
	checkf(SourceWorldItemActor->GetClass()->ImplementsInterface(URockWorldItemInterface::StaticClass()), TEXT("SourceWorldItemActor does not implement URockLootableWorldItem"));

	// Anticheat: Check distance to inventory.
	constexpr float MaxAddDistance = 1000.0f;
	AActor * InventoryActor = TargetInventory->GetOwningActor();

	if (SourceWorldItemActor->GetDistanceTo(InventoryActor) > MaxAddDistance)
	{
		return UndoData;
	}
	IRockWorldItemInterface* WorldItemInterfaceActor = Cast<IRockWorldItemInterface>(SourceWorldItemActor);
	if (!WorldItemInterfaceActor)
	{
		UE_LOG(LogRockInventory, Error, TEXT("SourceWorldItemActor %s does not implement URockWorldItemInterface!"), *SourceWorldItemActor->GetName());
		return UndoData;
	}
	
	// What Item are we looting?
	const FRockItemStack ItemStack = WorldItemInterfaceActor->GetItemStack(TargetInventory->GetOwningActor());
	
	UndoData.bSuccess = URockInventoryLibrary::LootItemToInventory(TargetInventory, ItemStack, UndoData.TargetSlotHandle, UndoData.Excess);

	// Let the actor know how much we took, so it can reconcile if it should delete itself or whatever.
	WorldItemInterfaceActor->OnLooted(TargetInventory->GetOwningActor(), ItemStack, UndoData.Excess);

	return UndoData;
}

bool FRockLootWorldItemTransaction::AttemptPredict() const
{
	return false;
}
