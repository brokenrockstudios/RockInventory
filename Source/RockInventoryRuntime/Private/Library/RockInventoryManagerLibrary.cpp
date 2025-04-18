// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Library/RockInventoryManagerLibrary.h"

#include "RockInventoryLogging.h"
#include "Components/RockInventoryManagerComponent.h"
#include "Inventory/Transactions/Core/RockInventoryManager.h"

void URockInventoryManagerLibrary::EnqueueTransaction(UObject* Object, URockInventoryTransaction* transaction)
{
	if (!Object || !transaction)
	{
		UE_LOG(LogRockInventory, Error, TEXT("EnqueueTransaction: Invalid input parameters - Object or Transaction is null"));
		return;
	}

	URockInventoryManager* transactionManager = nullptr;

	if (URockInventoryManager* manager = Cast<URockInventoryManager>(Object))
	{
		transactionManager = manager;
	}
	else if (const URockInventoryManagerComponent* inventoryComponent = Cast<URockInventoryManagerComponent>(Object))
	{
		transactionManager = inventoryComponent->TransactionManager;
	}
	else if (const AActor* actor = Cast<AActor>(Object))
	{
		if (const URockInventoryManagerComponent* managerComponent = actor->GetComponentByClass<URockInventoryManagerComponent>())
		{
			transactionManager = managerComponent->TransactionManager;
		}
	}

	if (!transactionManager)
	{
		UE_LOG(LogRockInventory, Error, TEXT("EnqueueTransaction: Failed to find valid transaction manager for object of type %s"),
			*GetNameSafe(Object->GetClass()));
		return;
	}

	transactionManager->EnqueueTransaction(transaction);
}
