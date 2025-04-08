// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "World/RockInventoryWorldItem.h"

ARockInventoryWorldItem::ARockInventoryWorldItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ARockInventoryWorldItem::SetItemStack(const FRockItemStack& InItemStack)
{
	if (InItemStack.IsValid())
	{
		ItemStack = InItemStack;
		// Update the static mesh?
	}
	else
	{
		ItemStack = FRockItemStack();
	}
}

FRockItemStack& ARockInventoryWorldItem::GetItemStack()
{
	return ItemStack;
}


