// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Library/RockInventoryManagerLibrary.h"

#include "Components/RockInventoryManagerComponent.h"

URockInventoryManagerComponent* URockInventoryManagerLibrary::GetInventoryManager(UObject* Object)
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

	// Check if the object is a manager component
	if (URockInventoryManagerComponent* ManagerComponent = Cast<URockInventoryManagerComponent>(Object))
	{
		return ManagerComponent;
	}

	// Check if the object is an actor with a manager component
	if (const AActor* Actor = Cast<AActor>(Object))
	{
		if (URockInventoryManagerComponent* ManagerComponent = Actor->GetComponentByClass<URockInventoryManagerComponent>())
		{
			return ManagerComponent;
		}
		// if actor is a pawn, get its controller
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (const AController* Controller = Pawn->GetController())
			{
				// It's controller might have the manager component
				if (URockInventoryManagerComponent* ManagerComponent = Controller->GetComponentByClass<URockInventoryManagerComponent>())
				{
					return ManagerComponent;
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

void URockInventoryManagerLibrary::ClearTransactionHistory(UObject* Object)
{
	if (URockInventoryManagerComponent* Manager = GetInventoryManager(Object))
	{
		Manager->ClearHistory();
	}
}
