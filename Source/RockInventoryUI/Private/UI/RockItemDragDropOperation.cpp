// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "UI/RockItemDragDropOperation.h"

#include "Components/RockInventoryManagerComponent.h"
#include "Inventory/Transactions/Core/RockInventoryManager.h"
#include "Inventory/Transactions/Implementations/RockDropItemTransaction.h"

void URockItemDragDropOperation::Dragged_Implementation(const FPointerEvent& PointerEvent)
{
	// Super::Dragged_Implementation(PointerEvent);
	// Fires on every frame while dragging
	// UE_LOG(LogRockInventory, Warning, TEXT("Dragged_Implementation"));
}

void URockItemDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	URockDropItemTransaction* Transaction = NewObject<URockDropItemTransaction>(this);

	Transaction->Initialize(Instigator, SourceInventory, SourceSlot);

	Transaction->DropLocationOffset = DropLocationOffset;

	const URockInventoryManagerComponent* TransactionComponent = Instigator->GetComponentByClass<URockInventoryManagerComponent>();
	if (TransactionComponent && TransactionComponent->TransactionManager)
	{
		TransactionComponent->TransactionManager->EnqueueTransaction(Transaction);
	}
	else
	{
		Transaction->Execute();
	}
	
	// Broadcasts the event that the drag operation was cancelled
	Super::DragCancelled_Implementation(PointerEvent);
}

void URockItemDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	Super::Drop_Implementation(PointerEvent);
	
}
