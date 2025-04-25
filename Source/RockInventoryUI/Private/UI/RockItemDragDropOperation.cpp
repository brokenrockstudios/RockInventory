// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "UI/RockItemDragDropOperation.h"

#include "Item/RockItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Transactions/Implementations/RockDropItemTransaction.h"
#include "Library/RockInventoryManagerLibrary.h"
#include "Misc/RockInventoryDeveloperSettings.h"

void URockItemDragDropOperation::Dragged_Implementation(const FPointerEvent& PointerEvent)
{
	// Super::Dragged_Implementation(PointerEvent);
	// Fires on every frame while dragging
	// UE_LOG(LogRockInventory, Warning, TEXT("Dragged_Implementation"));

	// SetSlotLocked? So that no one else can interact with it?


	// Play Sound
	if (SourceInventory && SourceSlot.IsValid())
	{
		const FRockItemStack& item = SourceInventory->GetItemBySlotHandle(SourceSlot);
		if (item.GetDefinition())
		{
			const TSoftObjectPtr<USoundBase> soundOverride = item.GetDefinition()->PickupSoundOverride;

			if (soundOverride.IsValid())
			{
				// Async Load the sound

				UGameplayStatics::PlaySound2D(this, soundOverride.Get());
			}
			else if (DefaultDragSound)
			{
				UGameplayStatics::PlaySound2D(this, DefaultDragSound);
			}
		}
	}
}

void URockItemDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	if (SourceInventory && SourceSlot.IsValid())
	{
		URockDropItemTransaction* Transaction = URockDropItemTransaction::CreateDropItemTransaction(Instigator, SourceInventory, SourceSlot);
		if (Transaction)
		{
			Transaction->DropLocationOffset = DropLocationOffset;
			URockInventoryManagerLibrary::EnqueueTransaction(Instigator, Transaction);
		}

		// Should we have a 'cancel' sound?

		// Broadcasts the event that the drag operation was cancelled
		Super::DragCancelled_Implementation(PointerEvent);
	}
}

void URockItemDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	Super::Drop_Implementation(PointerEvent);

	if (SourceInventory && SourceSlot.IsValid())
	{
		const FRockItemStack& item = SourceInventory->GetItemBySlotHandle(SourceSlot);
		if (item.GetDefinition())
		{
			const TSoftObjectPtr<USoundBase> soundOverride = item.GetDefinition()->DropSoundOverride;

			if (soundOverride.IsValid())
			{
				UGameplayStatics::PlaySound2D(this, soundOverride.Get());
			}
			else if (DefaultDropSound)
			{
				UGameplayStatics::PlaySound2D(this, DefaultDropSound);
			}
		}
	}
}
