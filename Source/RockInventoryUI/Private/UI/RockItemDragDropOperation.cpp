// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "UI/RockItemDragDropOperation.h"

#include "Components/RockInventoryManagerComponent.h"
#include "Inventory/RockInventory.h"
#include "UI/Shared/RockInventoryModelAccess.h"
#include "Item/RockItemDefinition.h"
#include "Item/Fragment/RockItemFragment_Sound.h"
#include "Kismet/GameplayStatics.h"
#include "Library/RockInventoryManagerLibrary.h"
#include "Transactions/Implementations/RockDropItemTransaction.h"
#include "UI/RockInventory_HoverItem.h"

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
			const FRockItemStack& item = RockInventoryUI::ModelOf(SourceInventory)->GetItemBySlotHandle(SourceSlotHandle);
			if (item.GetDefinition())
			{
				TSoftObjectPtr<USoundBase> soundOverride;
				if (auto SoundFragment = item.GetDefinition()->FindFragment<FRockItemFragment_Sound>())
				{
					soundOverride = SoundFragment->InventoryPickup;
				}
				
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

			// No slot lock while carrying (T-78): a move is validated by the preconditions it carries. Shared containers get a light claim in T-109.
		}
	}
}

void URockItemDragDropOperation::OnCancelCarry_Implementation()
{
	// Nothing to release: carrying takes no slot lock (T-78)
}

// OnFinishedCarry is called after a successful drop
void URockItemDragDropOperation::OnFinishedCarry_Implementation()
{
	// Nothing to release: carrying takes no slot lock (T-78)
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

void URockItemDragDropOperation::OnRotateRequested_Implementation()
{
	MoveItemParams.DesiredOrientation = MoveItemParams.DesiredOrientation == ERockItemOrientation::Horizontal
		? ERockItemOrientation::Vertical
		: ERockItemOrientation::Horizontal;

	if (URockInventory_HoverItem* HoverItem = Cast<URockInventory_HoverItem>(HoverDragVisual))
	{
		HoverItem->SetOrientation(MoveItemParams.DesiredOrientation);
	}
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
				const FRockItemStack& item = RockInventoryUI::ModelOf(SourceInventory)->GetItemBySlotHandle(SourceSlotHandle);
				if (item.GetDefinition())
				{
					TSoftObjectPtr<USoundBase> soundOverride;
					if (auto SoundFragment = item.GetDefinition()->FindFragment<FRockItemFragment_Sound>())
					{
						soundOverride = SoundFragment->InventoryDrop;
					}

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
				const FRockItemStack& item = RockInventoryUI::ModelOf(SourceInventory)->GetItemBySlotHandle(SourceSlotHandle);
				if (item.GetDefinition())
				{
					// Play a different sound if dropped on ground instead of into another inventory?
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("URockItemDragDropOperation::PlayFeedbackForOutcome_Implementation: Unhandled success reason '%s'. Consider implementing feedback for this case."), *Outcome.Reason.ToString());
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
