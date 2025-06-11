// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "Components/RockInventoryComponent.h"

#include "RockInventoryLogging.h"
#include "Library/RockInventoryLibrary.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "RockInventoryComponent"

// Sets default values for this component's properties
URockInventoryComponent::URockInventoryComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	// Set this component to be initialized when the game starts, and to be ticked every frame.
	PrimaryComponentTick.bCanEverTick = true;
	// You can turn off ticking to improve performance if not needed
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URockInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only server should instantiate the inventory. The client will receive it via replication
	if (GetOwner()->HasAuthority())
	{
		Inventory = NewObject<URockInventory>(this); // ?? RF_Transient
		Inventory->Owner = this;
		Inventory->Init(InventoryConfig);
	}
}

void URockInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (Inventory)
	{
		Inventory->UnregisterReplicationWithOwner();
		RemoveReplicatedSubObject(Inventory);
		Inventory = nullptr;
	}
}

void URockInventoryComponent::OnRep_Inventory(URockInventory* OldInventory)
{
	UE_LOG(LogRockInventory, Warning, TEXT("OnRep_Inventory called for %s %p %p"), *GetName(), Inventory.Get(), OldInventory);

	// Is there a better place to do this?
	// Ensure the inventory is valid and has been initialized
	if (Inventory)
	{
		UE_LOG(LogRockInventory, Warning, TEXT("OnRep_Inventory - Inventory is valid for %s"), *GetName());
		Inventory->ItemData.SetOwningInventory(Inventory);
		Inventory->SlotData.SetOwningInventory(Inventory);
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("OnRep_Inventory - Inventory is null for %s"), *GetName());
	}
	
	K2_OnInventoryChanged();
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
