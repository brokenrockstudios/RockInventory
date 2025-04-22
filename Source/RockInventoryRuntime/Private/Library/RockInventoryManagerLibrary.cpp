// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryManagerLibrary.h"

#include "RockInventoryLogging.h"
#include "Components/RockInventoryManagerComponent.h"
#include "Transactions/Core/RockInventoryManager.h"

bool URockInventoryManagerLibrary::EnqueueTransaction(UObject* Object, URockInventoryTransaction* Transaction)
{
	if (!Transaction)
	{
		UE_LOG(LogRockInventory, Error, TEXT("EnqueueTransaction: Invalid transaction parameter"));
		return false;
	}

	URockInventoryManager* Manager = GetInventoryManager(Object);
	if (Manager)
	{
		return Manager->EnqueueTransaction(Transaction);
	}

	// If no manager found, execute the transaction directly
	Transaction->Execute();
	return true;
}

URockInventoryManager* URockInventoryManagerLibrary::GetInventoryManager(UObject* Object)
{
	if (!Object)
	{
		return nullptr;
	}

	// TODO: Check if object implements IInventoryManagerProvider
	// if (const IInventoryManagerProvider* ManagerProvider = Cast<IInventoryManagerProvider>(Object))
	// {
	// 	return ManagerProvider->GetInventoryManager();
	// }

	// Check if the object is directly a manager
	if (URockInventoryManager* Manager = Cast<URockInventoryManager>(Object))
	{
		return Manager;
	}

	// Check if the object is a manager component
	if (const URockInventoryManagerComponent* ManagerComponent = Cast<URockInventoryManagerComponent>(Object))
	{
		return ManagerComponent->TransactionManager;
	}

	// Check if the object is an actor with a manager component
	if (const AActor* Actor = Cast<AActor>(Object))
	{
		if (const URockInventoryManagerComponent* ManagerComponent = Actor->GetComponentByClass<URockInventoryManagerComponent>())
		{
			return ManagerComponent->TransactionManager;
		}
		// if actor is a pawn, get its controller
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (const AController* Controller = Pawn->GetController())
			{
				// It's controller might have the manager component
				if (const URockInventoryManagerComponent* ManagerComponent = Controller->GetComponentByClass<URockInventoryManagerComponent>())
				{
					return ManagerComponent->TransactionManager;
				}
			}
		}
	}

	return nullptr;
}

bool URockInventoryManagerLibrary::HasInventoryManager(UObject* Object)
{
	return GetInventoryManager(Object) != nullptr;
}

TArray<FString> URockInventoryManagerLibrary::GetTransactionHistory(UObject* Object)
{
	if (URockInventoryManager* Manager = GetInventoryManager(Object))
	{
		return Manager->GetTransactionDescriptions();
	}
	return TArray<FString>();
}

bool URockInventoryManagerLibrary::UndoLastTransaction(UObject* Object)
{
	if (URockInventoryManager* Manager = GetInventoryManager(Object))
	{
		return Manager->UndoLastTransaction();
	}
	return false;
}

bool URockInventoryManagerLibrary::RedoTransaction(UObject* Object)
{
	if (URockInventoryManager* Manager = GetInventoryManager(Object))
	{
		return Manager->RedoTransaction();
	}
	return false;
}

void URockInventoryManagerLibrary::ClearTransactionHistory(UObject* Object)
{
	if (URockInventoryManager* Manager = GetInventoryManager(Object))
	{
		Manager->ClearHistory();
	}
}
