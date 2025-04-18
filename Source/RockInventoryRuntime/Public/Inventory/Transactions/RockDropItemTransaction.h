// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventoryTransaction.h"
#include "Item/World/RockInventoryWorldItem.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "UObject/Object.h"
#include "RockDropItemTransaction.generated.h"

/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockDropItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> SourceInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle SourceSlotHandle;

	// Consider optional parameters like where the player is looking or 'hit tracing' or something?
	// Offset from player center's viewport? 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector DropLocationOffset = FVector::ZeroVector;

	// Used for 'undo' functionality
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<ARockInventoryWorldItem> SpawnedItemStack = nullptr;
	UPROPERTY()
	ERockItemOrientation ExistingOrientation;

	virtual bool Execute_Implementation() 
	{
		const FRockItemStack Item = URockInventoryLibrary::RemoveItemAtLocation(SourceInventory, SourceSlotHandle);
		if (!Item.IsValid())
		{
			return false;
		}

		ExistingOrientation = SourceInventory->GetSlotByHandle(SourceSlotHandle).Orientation;
		FTransform Transform = SourceInventory->OwningActor->GetActorTransform();
		Transform.AddToTranslation(Transform.GetRotation().GetForwardVector() * DropLocationOffset.Size());
		
		ARockInventoryWorldItem* NewWorldItem = SourceInventory->OwningActor->GetWorld()->SpawnActorDeferred<ARockInventoryWorldItem>(
			GetDefault<URockInventoryDeveloperSettings>()->DefaultWorldItemClass, Transform);

		if (IsValid(NewWorldItem))
		{
			NewWorldItem->Execute_SetItemStack(NewWorldItem, Item);
			UGameplayStatics::FinishSpawningActor(NewWorldItem, Transform);
			SpawnedItemStack = NewWorldItem;
			return true;
		}

		return false;
	}

	virtual bool Undo_Implementation() override
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

	virtual bool CanUndo() const override
	{
		return SpawnedItemStack.IsValid();
	}

	virtual bool CanApply(URockInventoryComponent* OwnerInventory) const override
	{
		if (!SourceInventory || !SourceSlotHandle.IsValid())
		{
			return false;
		}

		// Check if we can remove the item from the source slot
		return true; // URockInventoryLibrary::CanRemoveItemAtLocation(SourceInventory, SourceSlotHandle);
	}

	virtual FString GetDescription() const override
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
};
