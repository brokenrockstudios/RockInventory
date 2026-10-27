// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "UI/RockItemDragDropOperation.h"

#include "Components/RockInventoryManagerComponent.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Library/RockInventoryManagerLibrary.h"
#include "Transactions/Implementations/RockDropItemTransaction.h"

void URockItemDragDropOperation::OnBeginCarry_Implementation()
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
					UGameplayStatics::PlaySound2D(Instigator, soundOverride.LoadSynchronous());
				}
				else if (DefaultDragSound)
				{
					UGameplayStatics::PlaySound2D(Instigator, DefaultDragSound);
				}
			}

			const FRockInventorySlotEntry SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
			URockInventoryManagerComponent* manager = URockInventoryManagerLibrary::GetInventoryManager(Instigator);
			if (SourceSlot.IsValid() && IsValid(manager))
			{
				manager->Server_RegisterSlotStatus(SourceInventory, Instigator, SourceSlotHandle, ERockSlotStatus::Pending);
			}
		}
	}
}

void URockItemDragDropOperation::OnCancelCarry_Implementation()
{
	// Release the lock on the slot
	if (SourceInventory && SourceSlotHandle.IsValid())
	{
		const FRockInventorySlotEntry SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
		URockInventoryManagerComponent* Manager = URockInventoryManagerLibrary::GetInventoryManager(Instigator);
		if (SourceSlot.IsValid() && Manager)
		{
			Manager->Server_ReleaseSlotStatus(SourceInventory, Instigator, SourceSlotHandle);
		}
	}
}

// OnFinishedCarry is called after a successful drop
void URockItemDragDropOperation::OnFinishedCarry_Implementation()
{
	// Release the lock on the slot
	if (SourceInventory && SourceSlotHandle.IsValid())
	{
		const FRockInventorySlotEntry SourceSlot = SourceInventory->GetSlotByHandle(SourceSlotHandle);
		URockInventoryManagerComponent* Manager = URockInventoryManagerLibrary::GetInventoryManager(Instigator);
		if (SourceSlot.IsValid() && Manager)
		{
			Manager->Server_ReleaseSlotStatus(SourceInventory, Instigator, SourceSlotHandle);
		}
	}
}

FRockDropOutcome URockItemDragDropOperation::OnUnhandledDrop_Implementation()
{
	FRockDropOutcome Outcome;
	if (SourceInventory && SourceSlotHandle.IsValid())
	{
		// NOTE: This is kind of 'game specific' choice, other people likely want to override this  and do what is appropriate for their game.

		const FRockDropItemTransaction& DropTransaction = FRockDropItemTransaction(
			Instigator,
			SourceInventory,
			SourceSlotHandle,
			DropLocationOffset,
			DropImpulse);

		URockInventoryManagerComponent* const Manager = URockInventoryManagerLibrary::GetInventoryManager(Instigator);
		if (Manager)
		{
			Manager->DropItem(DropTransaction);
		}
		Outcome.Reason = "item_dropped_world";
	}
	return Outcome;
}

void URockItemDragDropOperation::PlayFeedbackForOutcome_Implementation(const FRockDropOutcome& Outcome)
{
	Super::PlayFeedbackForOutcome_Implementation(Outcome);

	switch (Outcome.Result)
	{
	case ERockDropResult::Success:
		{
			if (Outcome.Reason == "item_moved_widget")
			{
				// Successful drop sound?
				const FRockItemStack& item = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
				if (item.GetDefinition())
				{
					const TSoftObjectPtr<USoundBase> soundOverride = item.GetDefinition()->DropSoundOverride;

					// Consider a some SoundRegistry based upon some 'traits' (e.g. wood, metal) that we could use
					// based upon some gameplay tags or something on the item definition.

					// else use the following
					if (soundOverride.IsValid())
					{
						// TODO: Async Load the sound
						UGameplayStatics::PlaySound2D(Instigator, soundOverride.LoadSynchronous());
					}
					else if (DefaultDropSound)
					{
						UGameplayStatics::PlaySound2D(Instigator, DefaultDropSound);
					}
				}
			}
			else if (Outcome.Reason == "item_moved_world")
			{
				// Dropped into world sound?
				const FRockItemStack& item = SourceInventory->GetItemBySlotHandle(SourceSlotHandle);
				if (item.GetDefinition())
				{
					// Play a different sound if dropped on ground instead of into another inventory?
				}
			}
			break;
		}
	case ERockDropResult::Rejected:
		{
			// Rejected sound?
		}
	case ERockDropResult::Pending:
		{
			// Pending sound?
			// Perhaps this was a 'partial' drop, such as a stack split.
			// Consider a different sound for this? or just reuse above Success?
			break;
		}
	default:
		{
			// No Sound
		}
	}
}
