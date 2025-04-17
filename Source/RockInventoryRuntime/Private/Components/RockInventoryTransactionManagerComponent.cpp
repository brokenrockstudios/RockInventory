// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Components/RockInventoryTransactionManagerComponent.h"


// Sets default values for this component's properties
URockInventoryTransactionManagerComponent::URockInventoryTransactionManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	RockInventoryTransactionManager = CreateDefaultSubobject<URockInventoryTransactionManager>(TEXT("RockInventoryTransactionManager"));
	
}


