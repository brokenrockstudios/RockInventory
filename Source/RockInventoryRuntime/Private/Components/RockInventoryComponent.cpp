// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Components/RockInventoryComponent.h"

#include "RockInventoryLogging.h"
#include "Engine/ActorChannel.h"
#include "Library/RockInventoryLibrary.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "RockInventoryComponent"

// Sets default values for this component's properties
URockInventoryComponent::URockInventoryComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicateUsingRegisteredSubObjectList = true;
	SetIsReplicatedByDefault(true);
	// Set this component to be initialized when the game starts, and to be ticked every frame.
	PrimaryComponentTick.bCanEverTick = true;
	// You can turn off ticking to improve performance if not needed
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URockInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize inventory from config
	if (InventoryConfig)
	{
		Inventory = NewObject<URockInventory>(this, TEXT("Inventory"), RF_Transient);
		// Should the owner be the component or the actor?
		Inventory->Owner = GetOwner();
		Inventory->Init(InventoryConfig);
	}
}

void URockInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (Inventory)
	{
		RemoveReplicatedSubObject(Inventory);
		Inventory = nullptr;
	}
}

bool URockInventoryComponent::K2_AddItem(const FRockItemStack& InItemStack, FRockInventorySlotHandle& outHandle, int32& OutExcess)
{
	return URockInventoryLibrary::LootItemToInventory(Inventory, InItemStack, outHandle, OutExcess);
}

bool URockInventoryComponent::K2_LootItem(const FRockItemStack& InItemStack, FRockInventorySlotHandle& outHandle, int32& OutExcess)
{
	return URockInventoryLibrary::LootItemToInventory(Inventory, InItemStack, outHandle, OutExcess);
}

FRockItemStack URockInventoryComponent::K2_DropItem(const FRockInventorySlotHandle& SlotHandle)
{
	FRockItemStack Item = URockInventoryLibrary::SplitItemStackAtLocation(Inventory, SlotHandle);
	if (!Item.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("DropItem: Failed to remove item at location"));
	}
	return Item;
}

FRockItemStack URockInventoryComponent::K2_RemoveItem(const FRockInventorySlotHandle& InHandle)
{
	FRockItemStack Item = URockInventoryLibrary::SplitItemStackAtLocation(Inventory, InHandle);
	if (!Item.IsValid())
	{
		UE_LOG(LogRockInventory, Warning, TEXT("RemoveItem: Failed to remove item at location"));
	}
	return Item;
}

bool URockInventoryComponent::K2_HasItem(const FName ItemId, int32 MinQuantity)
{
	if (MinQuantity <= 0)
	{
		return false;
	}
	int32 TotalQuantity = K2_GetItemCount(ItemId);
	return TotalQuantity >= MinQuantity;
}

int32 URockInventoryComponent::K2_GetItemCount(const FName ItemId)
{
	if (!Inventory || ItemId != NAME_None)
	{
		return 0;
	}
	return URockInventoryLibrary::GetItemCount(Inventory, ItemId);
}

void URockInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URockInventoryComponent, Inventory);
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
