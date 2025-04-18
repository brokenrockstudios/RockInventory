// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/RockSlotHandle.h"
#include "Inventory/Transactions/Core/RockInventoryTransaction.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "UObject/Object.h"
#include "World/RockInventoryWorldItem.h"
#include "RockLootWorldItemTransaction.generated.h"

class URockInventory;
/**
 *
 */
UCLASS(BlueprintType, Blueprintable)
class ROCKINVENTORYRUNTIME_API URockAddItemTransaction : public URockInventoryTransaction
{
	GENERATED_BODY()

public:
	///////////////////////////////////////////////////////////////////////////////
	/// Core Transaction Data (Required)
	// For a 'world loot', this should technically be a 'world item' that is being looted

	// TODO replace
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URockInventory> TargetInventory = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockItemStack ItemStack;

protected:
	//////////////////////////////////////////////////////////////////////////
	/// Internal Undo state 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRockInventorySlotHandle TargetSlotHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Excess;

public:
	virtual bool Execute_Implementation() override
	{
		return URockInventoryLibrary::LootItemToInventory(TargetInventory, ItemStack, TargetSlotHandle, Excess);
	}

	virtual bool Undo_Implementation()
	{
		// const FRockItemStack Item = URockInventoryLibrary::RemoveItemAtLocation(TargetInventory, SourceSlotHandle);
		// if (!Item.IsValid())
		// {
		// 	return false;
		// }
		//
		// ExistingOrientation = TargetInventory->GetSlotByHandle(SourceSlotHandle).Orientation;
		//
		// FTransform transform = TargetInventory->OwningActor->GetActorTransform();
		// // Prefer the instigator's transform if available
		// if (const AController* DropInstigator = Instigator.Get())
		// {
		// 	if (const auto pawn = DropInstigator->GetPawn())
		// 	{
		// 		transform = pawn->GetActorTransform();
		// 	}
		// }
		//
		// transform.AddToTranslation(transform.GetRotation().GetForwardVector() * DropLocationOffset.Size());
		//
		// ARockInventoryWorldItem* NewWorldItem = TargetInventory->OwningActor->GetWorld()->SpawnActorDeferred<ARockInventoryWorldItem>(
		// 	GetDefault<URockInventoryDeveloperSettings>()->DefaultWorldItemClass, transform);
		//
		// if (IsValid(NewWorldItem))
		// {
		// 	NewWorldItem->Execute_SetItemStack(NewWorldItem, Item);
		// 	UGameplayStatics::FinishSpawningActor(NewWorldItem, transform);
		// 	SpawnedItemStack = NewWorldItem;
		// 	return true;
		// }

		return false;
	}

	virtual FString GetDescription_Implementation() const
	{
		return FString::Printf(TEXT("Add Item"));
	}
};
