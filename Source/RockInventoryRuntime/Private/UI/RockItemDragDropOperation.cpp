// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "UI/RockItemDragDropOperation.h"

#include "Components/RockInventoryManagerComponent.h"
#include "Inventory/Transactions/Core/RockInventoryManager.h"
#include "Inventory/Transactions/Implementations/RockDropItemTransaction.h"

void URockItemDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);
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
}
