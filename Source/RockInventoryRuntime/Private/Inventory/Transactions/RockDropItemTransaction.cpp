// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Inventory/Transactions/Implementations/RockDropItemTransaction.h"

#include "Inventory/RockInventory.h"
#include "Kismet/GameplayStatics.h"
#include "Library/RockInventoryLibrary.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "World/RockInventoryWorldItem.h"

void URockDropItemTransaction::Initialize(
	AController* InInstigator, URockInventory* InSourceInventory, const FRockInventorySlotHandle& InSourceSlotHandle)
{
	Instigator = InInstigator;
	SourceInventory = InSourceInventory;
	SourceSlotHandle = InSourceSlotHandle;
}

bool URockDropItemTransaction::Execute_Implementation()
{
	const FRockItemStack Item = URockInventoryLibrary::RemoveItemAtLocation(SourceInventory, SourceSlotHandle);
	if (!Item.IsValid())
	{
		return false;
	}

	ExistingOrientation = SourceInventory->GetSlotByHandle(SourceSlotHandle).Orientation;
	
	FTransform transform = SourceInventory->OwningActor->GetActorTransform();
	// Prefer the instigator's transform if available
	if (const AController* DropInstigator = Instigator.Get())
	{
		if (const auto pawn = DropInstigator->GetPawn())
		{
			transform = pawn->GetActorTransform();
		}
	}

	transform.AddToTranslation(transform.GetRotation().GetForwardVector() * DropLocationOffset.Size());
	
	ARockInventoryWorldItem* NewWorldItem = SourceInventory->OwningActor->GetWorld()->SpawnActorDeferred<ARockInventoryWorldItem>(
		GetDefault<URockInventoryDeveloperSettings>()->DefaultWorldItemClass, transform);

	if (IsValid(NewWorldItem))
	{
		NewWorldItem->Execute_SetItemStack(NewWorldItem, Item);
		if (NewWorldItem->StaticMeshComponent->IsSimulatingPhysics())
		{
			NewWorldItem->StaticMeshComponent->AddImpulse(Impulse, NAME_None, true);
		}
		UGameplayStatics::FinishSpawningActor(NewWorldItem, transform);
		SpawnedItemStack = NewWorldItem;
		return true;
	}

	return false;
}

bool URockDropItemTransaction::Undo_Implementation()
{
	if (!SpawnedItemStack.IsValid())
	{
		return false;
	}
	// TODO: Do a range check to see if we are 'close enough' still to the item.

	// Get the item stack from the world item before destroying it
	const FRockItemStack Item = SpawnedItemStack->GetItemStack();

	// Destroy the world item
	SpawnedItemStack->Destroy();
	SpawnedItemStack = nullptr;

	// Return the item to the source inventory
	return URockInventoryLibrary::PlaceItemAtSlot_Internal(SourceInventory, SourceSlotHandle, Item, ExistingOrientation);
}

bool URockDropItemTransaction::CanUndo() const
{
	return SpawnedItemStack.IsValid();
}

bool URockDropItemTransaction::CanApply(URockInventoryComponent* OwnerInventory) const
{
	if (!SourceInventory || !SourceSlotHandle.IsValid())
	{
		return false;
	}
	// Check if we can remove the item from the source slot
	return true; // URockInventoryLibrary::CanRemoveItemAtLocation(SourceInventory, SourceSlotHandle);
}

FString URockDropItemTransaction::GetDescription() const
{
	if (!SourceInventory)
	{
		return TEXT("Drop Item (Invalid Source)");
	}

	FRockItemStack Item = URockInventoryLibrary::GetItemAtLocation(SourceInventory, SourceSlotHandle);
	return FString::Printf(TEXT("Drop %s from %s"),
		*Item.GetDebugString(),
		*SourceInventory->GetDebugString());
}
