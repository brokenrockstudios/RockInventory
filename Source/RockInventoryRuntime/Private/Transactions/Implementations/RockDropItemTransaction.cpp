// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Transactions/Implementations/RockDropItemTransaction.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Kismet/GameplayStatics.h"
#include "Library/RockInventoryLibrary.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "World/RockInventoryWorldItem.h"

bool FRockDropItemTransaction::CanExecute() const
{
	return true;
}

FRockDropItemUndoTransaction FRockDropItemTransaction::Execute() const
{
	FRockDropItemUndoTransaction UndoTransaction;
	UndoTransaction.bSuccess = false;
	const FRockItemStack Item = URockInventoryLibrary::SplitItemStackAtLocation(SourceInventory, SourceSlotHandle);
	if (!Item.IsValid())
	{
		return UndoTransaction;
	}

	const FRockPendingSlotOperation TargetPendingSlot = SourceInventory->GetPendingSlotState(SourceSlotHandle);
	if (TargetPendingSlot.IsClaimedByOther(Instigator.Get()))
	{
		// We can't drop here, someone else is using this slot
		return UndoTransaction;
	}

	UndoTransaction.ExistingOrientation = SourceInventory->GetSlotByHandle(SourceSlotHandle).Orientation;

	FTransform transform = SourceInventory->GetOwningActor()->GetActorTransform();
	// Prefer the instigator's transform if available
	const AController* DropInstigator = Instigator.Get();
	if (!DropInstigator)
	{
		// Where would we drop it if there is no controller
		return UndoTransaction;
	}
	const auto pawn = DropInstigator->GetPawn();
	if (!pawn)
	{
		return UndoTransaction;
	}
	transform = pawn->GetActorTransform();

	transform.AddToTranslation(transform.GetRotation().GetForwardVector() * DropLocationOffset.Size());

	ARockInventoryWorldItemBase* NewWorldItem = SourceInventory->GetOwningActor()->GetWorld()->SpawnActorDeferred<ARockInventoryWorldItemBase>(
		GetDefault<URockInventoryDeveloperSettings>()->DefaultWorldItemClass, transform);

	if (!IsValid(NewWorldItem))
	{
		return UndoTransaction;
	}
	IRockWorldItemInterface* WorldItemInterfaceActor = Cast<IRockWorldItemInterface>(NewWorldItem);
	if (!WorldItemInterfaceActor)
	{
		UE_LOG(LogRockInventory, Error, TEXT("NewWorldItem does not implement URockWorldItemInterface!"));
		return UndoTransaction;
	}
	
	WorldItemInterfaceActor->SetItemStack(Item);
	if (NewWorldItem->StaticMeshComponent->IsSimulatingPhysics())
	{
		NewWorldItem->StaticMeshComponent->AddImpulse(Impulse, NAME_None, true);
	}
	UGameplayStatics::FinishSpawningActor(NewWorldItem, transform);

	UndoTransaction.bSuccess = true;
	UndoTransaction.SpawnedItemStack = NewWorldItem;
	return UndoTransaction;
}

bool FRockDropItemTransaction::AttemptPredict() const
{
	return false;
}

bool FRockDropItemUndoTransaction::CanUndo()
{
	// If the original command wasn't successful, we can't undo it
	if (!bSuccess)
	{
		return false;
	}
	// check if item is still valid, and the target slot is still valid
	// TODO

	return false;
}

bool FRockDropItemUndoTransaction::Undo()
{
	return false;
}
