// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "UI/RockItemDragDropOperation.h"

#include "RockInventoryUILogging.h"
#include "Components/RockInventoryManagerComponent.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Transactions/Implementations/RockDropItemTransaction.h"
#include "Library/RockInventoryManagerLibrary.h"

void URockItemDragDropOperation::Dragged_Implementation(const FPointerEvent& PointerEvent)
{
	// Super::Dragged_Implementation(PointerEvent);
	// Fires on every frame while dragging
	// SetSlotLocked? So that no one else can interact with it?

	// If we are 'dragging' an item, trigger the runonce on start drag.
	// Otherwise if we didn't have this here, we'd need to have this code in the 'dragged' function
	if (!bRunOnce)
	{
		bRunOnce = true;
		// We create a new drag drop operation per drag, so this should be a new instance each time.

		// Play Sound
		if (SourceInventory && SourceSlotHandle.IsValid())
		{
			const FRockItemStack& item = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
			if (item.GetDefinition())
			{
				const TSoftObjectPtr<USoundBase> soundOverride = item.GetDefinition()->PickupSoundOverride;
				if (soundOverride.IsValid())
				{
					// TODO: Async Load the sound
					UGameplayStatics::PlaySound2D(this, soundOverride.LoadSynchronous());
				}
				else if (DefaultDragSound)
				{
					UGameplayStatics::PlaySound2D(this, DefaultDragSound);
				}
			}

			const FRockInventorySlotEntry SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
			URockInventoryManagerComponent* manager = URockInventoryManagerLibrary::GetInventoryManager(Instigator);
			if (SourceSlot.IsValid() && manager)
			{
				manager->Server_RegisterSlotStatus(SourceInventory, Instigator, SourceSlotHandle, ERockSlotStatus::Pending);
			}
		}
	}
}

void URockItemDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	if (SourceInventory && SourceSlotHandle.IsValid())
	{
		const FRockDropItemTransaction& DropTransaction = FRockDropItemTransaction(
			Instigator, SourceInventory, SourceSlotHandle, DropLocationOffset, FVector::ZeroVector);

		URockInventoryManagerComponent* const Manager = URockInventoryManagerLibrary::GetInventoryManager(Instigator);
		if (Manager)
		{
			Manager->DropItem(DropTransaction);

			// Release the lock on the slot
			const FRockInventorySlotEntry SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
			if (SourceSlot.IsValid())
			{
				Manager->Server_ReleaseSlotStatus(SourceInventory, Instigator, SourceSlotHandle);
			}
		}

		// Should we have a 'cancel' sound?
		// Broadcasts the event that the drag operation was cancelled
		Super::DragCancelled_Implementation(PointerEvent);
	}
}

void URockItemDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	Super::Drop_Implementation(PointerEvent);

	if (SourceInventory && SourceSlotHandle.IsValid())
	{
		const FRockItemStack& item = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
		if (item.GetDefinition())
		{
			const TSoftObjectPtr<USoundBase> soundOverride = item.GetDefinition()->DropSoundOverride;

			if (soundOverride.IsValid())
			{
				// TODO: Async Load the sound
				UGameplayStatics::PlaySound2D(this, soundOverride.LoadSynchronous());
			}
			else if (DefaultDropSound)
			{
				UGameplayStatics::PlaySound2D(this, DefaultDropSound);
			}
		}

		const FRockInventorySlotEntry SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
		URockInventoryManagerComponent* const Manager = URockInventoryManagerLibrary::GetInventoryManager(Instigator);
		if (SourceSlot.IsValid() && Manager)
		{
			Manager->Server_ReleaseSlotStatus(SourceInventory, Instigator, SourceSlotHandle);
		}
	}
}
