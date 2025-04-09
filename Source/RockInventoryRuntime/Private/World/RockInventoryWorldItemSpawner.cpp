// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "World/RockInventoryWorldItemSpawner.h"

#include "IDetailTreeNode.h"
#include "Item/RockItemDefinition.h"
#include "Settings/RockInventoryDeveloperSettings.h"
#include "World/RockInventoryWorldItem.h"


// Sets default values
ARockInventoryWorldItemSpawner::ARockInventoryWorldItemSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ARockInventoryWorldItemSpawner::SpawnWorldItem()
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (ItemDefinition && ItemStack.StackSize > 0)
	{
		ItemStack.ItemId = ItemDefinition->ItemId;
		const TSubclassOf<ARockInventoryWorldItem> ClassToSpawn = GetDefault<URockInventoryDeveloperSettings>()->DefaultWorldItemClass;

		ARockInventoryWorldItem* SpawnedActor = GetWorld()->SpawnActorDeferred<ARockInventoryWorldItem>(ClassToSpawn, GetActorTransform());
		SpawnedActor->SetItemStack(ItemStack);
		SpawnedActor->FinishSpawning(GetActorTransform());
	}
	else
	{
		// no valid item definition
	}
	
}

