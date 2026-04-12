// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Transactions/Implementations/RockDropItemTransaction.h"

#include "RockInventoryLogging.h"
#include "Inventory/RockInventory.h"
#include "Kismet/GameplayStatics.h"
#include "Library/RockInventoryLibrary.h"
#include "World/RockInventoryWorldItem.h"

bool FRockDropItemTransaction::CanExecute() const
{
	return true;
}

FRockDropItemUndoTransaction FRockDropItemTransaction::Execute() const
{
	FRockDropItemUndoTransaction UndoTransaction;
	UndoTransaction.bSuccess = false;

	if (!Instigator.IsValid() || !IsValid(SourceInventory))
	{
		UE_LOG(LogRockInventory, Warning, TEXT("DropItemTransaction::Execute - Invalid Instigator"));
		return UndoTransaction;
	}


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
	// We don't want to use the source inventory because what if someone else is instigating it
	// we want to use the instigator not the actor that the pawn is.
	// FTransform transform = SourceInventory->GetOwningActor()->GetActorTransform();
	FTransform transform = pawn->GetActorTransform();
	const FVector throwDirection = FindThrowDirection(DropInstigator);
	const FRotator throwRotation = throwDirection.Rotation();
	const FVector desiredOffset = throwRotation.RotateVector(DropLocationOffset);
	const FVector safeOffset = FindSafeDropLocation(DropInstigator, pawn->GetActorLocation() + desiredOffset);
	transform.SetLocation(safeOffset);

	UClass* WorldItemClass = Item.GetDefinition()->GetWorldItemClass();
	AActor* NewWorldItem = pawn->GetWorld()->SpawnActorDeferred<AActor>(WorldItemClass, transform);
	checkf(NewWorldItem, TEXT("Failed to spawn world item actor of class. Possibly DefaultWorldItemClass is unset in Project Settings"));

	if (!IsValid(NewWorldItem))
	{
		return UndoTransaction;
	}
	IRockLootableInterface* WorldItemInterfaceActor = Cast<IRockLootableInterface>(NewWorldItem);
	if (WorldItemInterfaceActor)
	{
		WorldItemInterfaceActor->SetItemStack(Item);
	}

	UGameplayStatics::FinishSpawningActor(NewWorldItem, transform);

	// Does this work if we do it before FinishSpawningActor?
	if (WorldItemInterfaceActor)
	{
		// TODO: What if we wanted to 'place' an item instead of 'drop/throw' it
		FVector LocalImpulse = throwRotation.RotateVector(Impulse);
		WorldItemInterfaceActor->ApplyThrowImpulse(LocalImpulse);
	}

	UndoTransaction.bSuccess = true;
	UndoTransaction.SpawnedItemStack = NewWorldItem;
	return UndoTransaction;
}

bool FRockDropItemTransaction::AttemptPredict() const
{
	return false;
}

FVector FRockDropItemTransaction::FindThrowDirection(const AController* Controller) const
{
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		return CamRot.Vector();
	}
	// AI fallback
	return Controller->GetControlRotation().Vector();
}

FVector FRockDropItemTransaction::FindSafeDropLocation(const AController* Controller, const FVector& DesiredDropLocation) const
{
	const APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		return DesiredDropLocation;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	// Consider the FVector UFenRangedWeaponInstance::GetCameraViewPoint(FVector& OutCamLoc, FVector& OutAimDir) const
	Pawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);

	const bool bHit = Pawn->GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, DesiredDropLocation, ECC_Visibility, Params);

	// Small pullback so we don't spawn inside the surface
	static constexpr float PullbackDistance = 10.f;
	return bHit
		       ? Hit.Location - (DesiredDropLocation - EyeLocation).GetSafeNormal() * PullbackDistance
		       : DesiredDropLocation;
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
