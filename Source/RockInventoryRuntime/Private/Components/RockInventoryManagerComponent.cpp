// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Components/RockInventoryManagerComponent.h"

#include "Inventory/Transactions/Core/RockInventoryManager.h"


// Sets default values for this component's properties
URockInventoryManagerComponent::URockInventoryManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	TransactionManager = CreateDefaultSubobject<URockInventoryManager>(TEXT("RockInventoryTransactionManager"));
}
