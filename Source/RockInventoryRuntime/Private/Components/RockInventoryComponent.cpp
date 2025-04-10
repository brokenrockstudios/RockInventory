// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Components/RockInventoryComponent.h"

#include "Library/RockInventoryLibrary.h"
#include "Library/RockItemStackLibrary.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "RockInventoryComponent"

// Sets default values for this component's properties
URockInventoryComponent::URockInventoryComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	//Inventory->Tabs
	// Set up initial inventory stuff?
}

void URockInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryConfig)
	{
		Inventory = NewObject<URockInventory>(this, TEXT("Inventory"), RF_Transient);
		Inventory->Init(InventoryConfig);
	}
}


bool URockInventoryComponent::K2_AddItem(const FRockItemStack& InItemStack, FRockInventorySlotHandle& outHandle, int32& OutExcess)
{
	// Sometimes when we add items, they might be 'incomplete'. Such as needing their instances created.
	FRockItemStack ItemStack = URockItemStackLibrary::CreateItemStack(InItemStack);
	
	return URockInventoryLibrary::AddItemToInventory(Inventory, ItemStack, outHandle, OutExcess);
}

bool URockInventoryComponent::K2_RemoveItem(const FRockItemStack& ItemStack, FRockInventorySlotHandle& outHandle, int32& OutExcess)
{
	return false;
}

#if WITH_EDITOR
EDataValidationResult URockInventoryComponent::IsDataValid(class FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (!InventoryConfig)
	{
		Context.AddError(FText::Format(
			LOCTEXT("InvalidInventoryConfig", "InventoryConfig is invalid for {0}"),
			FText::FromString(GetName())));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
