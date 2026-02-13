// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "UI/RockInventory_ContainerBase.h"

#include "RockInventoryUILogging.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/RockInventoryManagerComponent.h"
#include "Core/RockItemDragCarrySubsystem.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockInventoryManagerLibrary.h"
#include "Library/RockItemStackLibrary.h"
#include "UI/RockInventory_HoverItem.h"
#include "UI/RockInventory_Slot_BackgroundBase.h"
#include "UI/RockInventory_Slot_ItemBase.h"
#include "UI/RockItemDragDropOperation.h"


void URockInventory_ContainerBase::OnCarryEnded(const URockDragCarryOperation* Operation)
{
	if (const URockItemDragDropOperation* RockOp = Cast<URockItemDragDropOperation>(Operation))
	{
		if (RockOp->SourceInventory == Inventory) // Only handle our inventory
		{
			if (URockInventory_Slot_ItemBase* SlotWidget = FindItemSlotWidgetBySlotHandle(RockOp->SourceSlotHandle))
			{
				SlotWidget->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}

	UnHighlightLast();
}

URockInventory_Slot_ItemBase* URockInventory_ContainerBase::FindItemSlotWidgetBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const
{
	// TODO Replace this whole function with a TMap or TArray in the container that maps slot handles to widgets
	// TMap<FRockItemStackHandle, TWeakObjectPtr<URockInventory_Slot_ItemBase>> ItemToWidget;

	if (ItemsCanvasPanel)
	{
		for (UWidget* child : ItemsCanvasPanel->GetAllChildren())
		{
			if (URockInventory_Slot_ItemBase* itemSlot = Cast<URockInventory_Slot_ItemBase>(child))
			{
				if (itemSlot->SlotHandle == InSlotHandle)
					return itemSlot;
			}
		}
	}
	else if (ItemsOverlayPanel)
	{
		// should only be 1
		for (UWidget* child : ItemsOverlayPanel->GetAllChildren())
		{
			if (URockInventory_Slot_ItemBase* itemSlot = Cast<URockInventory_Slot_ItemBase>(child))
			{
				if (itemSlot->SlotHandle == InSlotHandle)
					return itemSlot;
			}
		}
	}
	return nullptr;
}

URockInventory_Slot_ItemBase* URockInventory_ContainerBase::FindItemSlotWidgetByItemHandle(const FRockItemStackHandle& InItemHandle) const
{
	if (ItemsCanvasPanel)
	{
		for (UWidget* child : ItemsCanvasPanel->GetAllChildren())
		{
			if (URockInventory_Slot_ItemBase* itemSlot = Cast<URockInventory_Slot_ItemBase>(child))
			{
				if (itemSlot->ItemHandle == InItemHandle)
					return itemSlot;
			}
		}
	}
	else if (ItemsOverlayPanel)
	{
		// should only be 1
		for (UWidget* child : ItemsOverlayPanel->GetAllChildren())
		{
			if (URockInventory_Slot_ItemBase* itemSlot = Cast<URockInventory_Slot_ItemBase>(child))
			{
				if (itemSlot->ItemHandle == InItemHandle)
					return itemSlot;
			}
		}
	}
	return nullptr;
}

void URockInventory_ContainerBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (URockItemDragCarrySubsystem* subsystem = URockItemDragCarrySubsystem::Get(GetWorld()))
	{
		subsystem->OnCarryEnded.AddDynamic(this, &ThisClass::OnCarryEnded);
	}
}


void URockInventory_ContainerBase::NativeDestruct()
{
	// If this container is closed, let's cancel any drags originating from it.
	// We don't want to leave the user in a state where they are dragging an item from a hidden inventory.
	// This can happen if the user starts dragging an item, then closes the inventory before dropping.
	// Or do we want to allow this?
	if (URockItemDragCarrySubsystem* subsystem = URockItemDragCarrySubsystem::Get(GetWorld()))
	{
		subsystem->OnCarryEnded.RemoveDynamic(this, &ThisClass::OnCarryEnded);
		if (const URockItemDragDropOperation* drag = Cast<URockItemDragDropOperation>(subsystem->GetDragOperation()))
		{
			if (drag->SourceInventory == Inventory)
			{
				FRockDropOutcome outcome;
				outcome.Result = ERockDropResult::Canceled;
				subsystem->CancelCarry(outcome);
			}
		}
	}

	if (GetWorld())
	{
		// clear all pending destroy timers
		FTimerManager& timerManager = GetWorld()->GetTimerManager();
		for (auto& pair : PendingDestroyTimers)
		{
			timerManager.ClearTimer(pair.Value);
		}
	}
	PendingDestroyTimers.Empty();
	ItemViews.Empty();
	SlotToItem.Empty();
	BackgroundGridSlots.Empty();

	Inventory = nullptr;
	if (ItemsCanvasPanel)
	{
		ItemsCanvasPanel->ClearChildren();
		ItemsCanvasPanel = nullptr;
	}
	else if (ItemsOverlayPanel)
	{
		ItemsOverlayPanel->ClearChildren();
		ItemsOverlayPanel = nullptr;
	}

	Super::NativeDestruct();
}

void URockInventory_ContainerBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bMouseWithinCanvas = true;
	
	if (const URockItemDragCarrySubsystem* subsystem = URockItemDragCarrySubsystem::Get(GetWorld()))
	{
		if (subsystem->IsCarrying())
		{
			URockInventory_HoverItem* HoverItemWidget = Cast<URockInventory_HoverItem>(subsystem->GetActiveVisual());
			if (HoverItemWidget)
			{
				HoverItemWidget->SetTargetSize(TileSize, TabInfo.GetSlotSizePolicy());
			}
		}
	}
}

void URockInventory_ContainerBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bMouseWithinCanvas = false;

	UnHighlightLast();
}

void URockInventory_ContainerBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bMouseWithinCanvas)
	{
		return;
	}
}

FIntPoint URockInventory_ContainerBase::CalculateStartingCoordinate(
	const FIntPoint& Center, const FIntPoint& Size, const ERockInventory_TileQuadrant Quadrant) const
{
	// Pre-compute “half size” using plain integer division.
	const int32 halfWidth = Size.X / 2;
	const int32 halfHeight = Size.Y / 2;

	const bool bHasEvenWidth = (Size.X % 2) == 0;
	const bool bHasEvenHeight = (Size.Y % 2) == 0;

	// Decode which side we’re on.
	const bool bRight = (Quadrant == ERockInventory_TileQuadrant::TopRight) || (Quadrant == ERockInventory_TileQuadrant::BottomRight);
	const bool bBottom = (Quadrant == ERockInventory_TileQuadrant::BottomLeft) || (Quadrant == ERockInventory_TileQuadrant::BottomRight);

	// Extra tile only matters when the size along that axis is even.
	const int32 xOffset = (bRight && bHasEvenWidth) ? 1 : 0;
	const int32 yOffset = (bBottom && bHasEvenHeight) ? 1 : 0;

	FIntPoint retVal(
		Center.X - halfWidth + xOffset,
		Center.Y - halfHeight + yOffset
	);

	// Clamp to grid bounds
	retVal.X = FMath::Max(0, FMath::Min(retVal.X, TabInfo.GetColumns() - Size.X));
	retVal.Y = FMath::Max(0, FMath::Min(retVal.Y, TabInfo.GetRows() - Size.Y));

	return retVal;
}

ERockInventory_TileQuadrant URockInventory_ContainerBase::CalculateTileQuadrant(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const
{
	checkf(TileSize > 0.0f, TEXT("TileSize must be greater than zero."));

	// Fractional position inside the tile: 0–1 range
	const FVector2D Local = (MousePosition - CanvasPosition) / TileSize;
	const float FracX = FMath::Frac(Local.X);
	const float FracY = FMath::Frac(Local.Y);

	// Pack the two compares into two bits: (Y ≥ 0.5) << 1 | (X ≥ 0.5)
	const uint8 QuadrantIndex = (FracY >= 0.5f) * 2 | (FracX >= 0.5f);

	return static_cast<ERockInventory_TileQuadrant>(QuadrantIndex);
}

FIntPoint URockInventory_ContainerBase::CalculateHoveredCoordinates(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const
{
	checkf(TileSize > 0.0f, TEXT("TileSize must be greater than zero."));

	return FIntPoint{
		static_cast<int32>(FMath::FloorToInt((MousePosition.X - CanvasPosition.X) / TileSize)),
		static_cast<int32>(FMath::FloorToInt((MousePosition.Y - CanvasPosition.Y) / TileSize))
	};
}

FIntPoint URockInventory_ContainerBase::GetCoordinatesFromIndex(int32 Index) const
{
	return FIntPoint{
		Index % TabInfo.GetColumns(),
		Index / TabInfo.GetColumns()
	};
}

int32 URockInventory_ContainerBase::GetIndexFromPosition(const FIntPoint& Position) const
{
	return Position.X + Position.Y * TabInfo.GetColumns();
}

FRockInventory_SpaceQueryResult URockInventory_ContainerBase::QueryHoverSpace(FIntPoint Position, FIntPoint Dimensions) const
{
	FRockInventory_SpaceQueryResult result;
	// in the grid bounds?
	if (!IsInGridBounds(GetIndexFromPosition(Position), Dimensions))
	{
		return result;
	}

	result.bHasSpace = true;

	// If more than one of the indices is occupied with the same item, we need to see if they all have the same upper left index.
	TSet<int32> occupiedUpperLeftIndices;

	if (!Inventory)
	{
		return result;
	}

	const URockItemDragCarrySubsystem* carrySubsystem = URockItemDragCarrySubsystem::Get(GetOwningPlayer());
	ensureMsgf(carrySubsystem, TEXT("Could not get CarrySubsystem in QueryHoverSpace!"));
	const URockItemDragDropOperation* dragDrop = Cast<URockItemDragDropOperation>(carrySubsystem->GetDragOperation());
	ensureMsgf(dragDrop, TEXT("Could not get correct DragDropOperation in QueryHoverSpace!"));
	ensureMsgf(dragDrop->SourceInventory, TEXT("DragDrop SourceInventory is null!"));
	const FRockItemStack& copyOfSourceItem = dragDrop->SourceInventory->GetItemBySlotHandle(dragDrop->SourceSlotHandle);

	for (int32 column = 0; column < Dimensions.Y; ++column)
	{
		for (int32 row = 0; row < Dimensions.X; ++row)
		{
			const int32 checkRow = Position.X + row;
			const int32 checkCol = Position.Y + column;
			const int32 checkIndex = GetIndexFromPosition(FIntPoint(checkRow, checkCol));

			const URockInventory_Slot_BackgroundBase* gridSlot = BackgroundGridSlots[checkIndex];
			if (!gridSlot)
			{
				continue;
			}

			const FRockInventorySlotHandle anchorSlotHandle = gridSlot->GetAnchorItemSlotHandle();
			const FRockItemStack item = Inventory->GetItemBySlotHandle(anchorSlotHandle);
			if (item.IsValid() && item != copyOfSourceItem)
			{
				occupiedUpperLeftIndices.Add(anchorSlotHandle.GetAbsoluteIndex());
				result.bHasSpace = false;
			}

			// const FRockInventorySlotEntry SlotEntry = Inventory->GetSlotByAbsoluteIndex(checkIndex);
			// const FRockItemStack Item = Inventory->GetItemByHandle(SlotEntry.ItemHandle);
			// if (Item.IsValid())
			// {
			// 	OccupiedUpperLeftIndices.Add(SlotEntry.SlotHandle.GetIndex());
			// 	Result.bHasSpace = false;
			// }
		}
	}
	if (occupiedUpperLeftIndices.Num() == 1) // single item at position - it's valid for swapping/combining
	{
		const int32 Index = *occupiedUpperLeftIndices.CreateConstIterator(); // Is there a better way to get the singular element?
		const FRockInventorySlotEntry SlotEntry = Inventory->GetSlotByAbsoluteIndex(Index);
		result.UpperLeftIndex = SlotEntry.SlotHandle.GetAbsoluteIndex();
	}


	// UInv_InventoryStatics::ForEach2D(GridSlots,
	// 	UInv_WidgetUtils::GetIndexFromPosition(Position, Columns),
	// 	Dimensions,
	// 	Columns,
	// 	[&](const UInv_GridSlot* GridSlot)
	// 	{
	// 		if (GridSlot->GetInventoryItem().IsValid())
	// 		{
	// 			OccupiedUpperLeftIndices.Add(GridSlot->GetUpperLeftIndex());
	// 			Result.bHasSpace = false;
	// 		}
	// 	});

	// if so, is there only one item in the way? (can we swap?)
	// if (OccupiedUpperLeftIndices.Num() == 1) // single item at position - it's valid for swapping/combining
	// {
	// 	const int32 Index = *OccupiedUpperLeftIndices.CreateConstIterator();
	// 	//Result.ValidItem = GridSlots[Index]->GetInventoryItem();
	// 	Result.UpperLeftIndex = GridSlots[Index]->GetUpperLeftIndex();
	// }

	return result;
}


bool URockInventory_ContainerBase::IsInGridBounds(int32 StartIndex, FIntPoint ItemGridSize) const
{
	// Early validation
	if (StartIndex < 0 || StartIndex >= TabInfo.GetNumSlots() ||
		ItemGridSize.X <= 0 || ItemGridSize.Y <= 0)
	{
		return false;
	}

	const int32 columns = TabInfo.GetColumns();
	const int32 rows = TabInfo.GetRows();

	const int32 endColumn = (StartIndex % columns) + ItemGridSize.X;
	const int32 endRow = (StartIndex / columns) + ItemGridSize.Y;
	return endColumn <= columns && endRow <= rows;
}

void URockInventory_ContainerBase::OnTileParametersUpdated(const FRockInventory_TileParameters& Parameters)
{
	const URockItemDragCarrySubsystem* carrySubsystem = URockItemDragCarrySubsystem::Get(GetOwningPlayer());
	if (!carrySubsystem)
	{
		return;
	}
	const URockItemDragDropOperation* dragDrop = Cast<URockItemDragDropOperation>(carrySubsystem->GetDragOperation());
	if (!dragDrop)
	{
		return;
	}

	const FRockItemStack& copyOfItem = dragDrop->SourceInventory->GetItemBySlotHandle(dragDrop->SourceSlotHandle);

	FIntPoint dimensions = FIntPoint(1, 1);
	if (copyOfItem.IsValid())
	{
		dimensions = copyOfItem.GetDefinition()->GridSize;
	}

	// Calculate the starting coordinate (Top Left) for highlighting.
	const int32 tileIndex = GetLocalIndexFromSlotHandle(Parameters.SlotHandle);
	const FIntPoint tileCoordinates = GetCoordinatesFromIndex(tileIndex);
	const FIntPoint startingCoordinate = CalculateStartingCoordinate(tileCoordinates, dimensions, Parameters.TileQuadrant);

	ItemDropIndex = GetIndexFromPosition(startingCoordinate, TabInfo.GetColumns());
	CurrentQueryResult = QueryHoverSpace(startingCoordinate, dimensions);

	if (CurrentQueryResult.bHasSpace)
	{
		HighlightSlots(ItemDropIndex, dimensions);
		return;
	}
	UnHighlightLast();
}

void URockInventory_ContainerBase::HighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	if (!bMouseWithinCanvas)
	{
		return;
	}
	UnHighlightLast();

	ForEachSlot(Index,
		Dimensions,
		[&](int32 SlotIndex, int32 Column, int32 Row, int32 AbsoluteIndex)
		{
			if (!BackgroundGridSlots.IsValidIndex(SlotIndex))
			{
				return;
			}
			URockInventory_Slot_BackgroundBase* BackgroundSlot = BackgroundGridSlots[SlotIndex];
			if (BackgroundSlot)
			{
				// TODO: Set the background slot to highlighted texture
				BackgroundSlot->SetGrayedOutTexture();
			}
		});
	LastHighlightedDimensions = Dimensions;
	LastHighlightedIndex = Index;
}

void URockInventory_ContainerBase::UnHighlightLast()
{
	UnHighlightSlots(LastHighlightedIndex, LastHighlightedDimensions);
}

void URockInventory_ContainerBase::UnHighlightSlots(const int32 Index, const FIntPoint& Dimensions)
{
	ForEachSlot(Index,
		Dimensions,
		[&](int32 SlotIndex, int32 Column, int32 Row, int32 AbsoluteIndex)
		{
			if (!BackgroundGridSlots.IsValidIndex(SlotIndex))
			{
				return;
			}
			URockInventory_Slot_BackgroundBase* BackgroundSlot = BackgroundGridSlots[SlotIndex];
			if (!BackgroundSlot)
			{
				return;
			}
			if (HasItemAtAbsoluteIndex(AbsoluteIndex))
			{
				//BackgroundSlot->SetOccupiedTexture();
				BackgroundSlot->SetUnoccupiedTexture();
			}
			else
			{
				BackgroundSlot->SetUnoccupiedTexture();
			}
		});
}

int32 URockInventory_ContainerBase::GetIndexFromPosition(const FIntPoint& Position, const int32 Columns) const
{
	return Position.Y * Columns + Position.X;
}

bool URockInventory_ContainerBase::HasItemAtAbsoluteIndex(int32 AbsoluteIndex) const
{
	if (!Inventory)
	{
		return false;
	}

	const FRockInventorySlotEntry slotEntry = Inventory->GetSlotByAbsoluteIndex(AbsoluteIndex);
	const FRockItemStack itemStack = Inventory->GetItemByHandle(slotEntry.ItemHandle);
	return itemStack.IsValid();
}


bool URockInventory_ContainerBase::PickUp(URockInventory* InInventory, const FRockInventorySlotHandle& InSlotHandle)
{
	URockItemDragCarrySubsystem* carrySubsystem = URockItemDragCarrySubsystem::Get(GetOwningPlayer());
	if (!carrySubsystem || !InInventory || !InSlotHandle.IsValid())
	{
		UE_LOG(LogRockInventoryUI, Warning, TEXT("ContainerBase::PickUp: Invalid parameters"));
		return false;
	}

	if (!ensure(DragCarryOperationClass))
	{
		UE_LOG(LogRockInventoryUI, Error, TEXT("PickUp: DragCarryOperationClass is not set in Inventory Container!"));
		return false;
	}
	URockDragCarryOperation* dragDrop = NewObject<URockDragCarryOperation>(GetOwningPlayer(), DragCarryOperationClass);
	URockItemDragDropOperation* rockDragDrop = Cast<URockItemDragDropOperation>(dragDrop);
	if (!rockDragDrop)
	{
		UE_LOG(LogRockInventoryUI, Error, TEXT("OnItemClicked: Failed to create RockDragDropOperation"));
		return false;
	}

	rockDragDrop->Instigator = GetOwningPlayer();
	rockDragDrop->SourceInventory = InInventory;
	rockDragDrop->SourceSlotHandle = InSlotHandle;
	rockDragDrop->Offset = FVector2D(150.0f, 0.0f);
	rockDragDrop->MoveItemParams = FRockMoveItemParams();
	rockDragDrop->DropImpulse = FVector::ZeroVector;
	rockDragDrop->Orientation = ERockItemOrientation::Horizontal;
	rockDragDrop->MoveMode = ERockItemMoveMode::FullStack;

	{
		const FRockInventorySlotEntry SlotEntry = Inventory->GetSlotByHandle(InSlotHandle);
		rockDragDrop->MoveCount = SlotEntry.ItemHandle.IsValid() ? Inventory->GetItemByHandle(SlotEntry.ItemHandle).GetStackCount() : 0;
	}

	URockInventory_HoverItem* HoverItemWidget = CreateWidget<URockInventory_HoverItem>(GetWorld(), HoverItemClass);
	HoverItemWidget->SetItemSource(Inventory, InSlotHandle);
	HoverItemWidget->SetTargetSize(TileSize, TabInfo.GetSlotSizePolicy());

	rockDragDrop->HoverDragVisual = HoverItemWidget;

	carrySubsystem->BeginCarry(rockDragDrop);


	// bind lambda to DragCancelled and Drop to unhighlight slots
	//rockDragDrop->OnDragCancelled.AddDynamic(this, &ThisClass::UnHighlightSlots);
	//rockDragDrop->OnDrop.AddDynamic(this, &ThisClass::UnHighlightSlots);

	// TODO: DONT DELETE ME UNTIL THIS IS IMPLEMENTED PROPERLY
	// InitializeDrag(); // This function will set the icon as 'locked' or 'transparent' until the drag is complete
	// rockDragDrop->OnDragCancelled.AddDynamic(this, &ThisClass::SetOverallSizeAsVisible);
	// rockDragDrop->OnDrop.AddDynamic(this, &ThisClass::SetOverallSizeAsVisible);
	// Update the SetOverallSize widget as Visible


	// Make current 'widget' hit test invisible. So that we could move slightly and not have it interfere with the dragged item widget.
	// We need to 'undo' this sometime later. But for now, refresh likely is remaking all the widgets
	for (UWidget* child : GridPanel->GetAllChildren())
	{
		if (URockInventory_Slot_ItemBase* itemSlot = Cast<URockInventory_Slot_ItemBase>(child))
		{
			if (itemSlot->SlotHandle == InSlotHandle)
			{
				// TODO: Add back later
				// itemSlot->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	}
	return true;
}

bool URockInventory_ContainerBase::CanAcceptCarryDrop(const URockDragCarryOperation* Operation) const
{
	// Check if we can carry the item being dragged over this container
	if (!Operation)
	{
		return false;
	}
	return true;
}

FRockDropOutcome URockInventory_ContainerBase::OnCarryDrop(const URockDragCarryOperation* Operation, const FRockCarryDropContext& DropContext)
{
	FRockDropOutcome Outcome;
	const URockItemDragCarrySubsystem* carrySubsystem = URockItemDragCarrySubsystem::Get(GetOwningPlayer());

	if (!carrySubsystem || !carrySubsystem->IsCarrying())
	{
		Outcome.Result = ERockDropResult::Rejected;
		return Outcome;
	}

	if (const URockItemDragDropOperation* dragOperation = Cast<URockItemDragDropOperation>(carrySubsystem->GetDragOperation()))
	{
		if (const UInventoryCarryContextData* itemContext = Cast<UInventoryCarryContextData>(DropContext.ContextData))
		{
			const FRockMoveItemTransaction moveTransaction(GetOwningPlayer(),
				dragOperation->SourceInventory,
				dragOperation->SourceSlotHandle,
				itemContext->TargetInventory,
				itemContext->TargetSlotHandle,
				dragOperation->MoveItemParams);

			if (URockInventoryManagerComponent* manager = URockInventoryManagerLibrary::GetInventoryManager(GetOwningPlayer()))
			{
				manager->MoveItem(moveTransaction);
			}
			Outcome.SFX = dragOperation->DefaultDropSound;
		}
		Outcome.Result = ERockDropResult::Success;
		return Outcome;
	}

	Outcome.Result = ERockDropResult::Rejected;
	return Outcome;
}

int32 URockInventory_ContainerBase::GetLocalIndexFromSlotHandle(const FRockInventorySlotHandle& SlotHandle) const
{
	// Get the local index from the slot handle
	if (!Inventory)
	{
		return INDEX_NONE;
	}
	//const FRockInventorySectionInfo section = Inventory->GetSectionInfoBySlotHandle(SlotHandle);
	//section.GetLocalIndex(SlotHandle.GetAbsoluteIndex());
	const int32 localIndex = TabInfo.GetLocalIndex(SlotHandle.GetAbsoluteIndex());
	return localIndex;
}


void URockInventory_ContainerBase::OnGridSlotClicked(const FRockGridSlotEventData& EventData)
{
	TileParameters.TileQuadrant = EventData.TileQuadrant;
	TileParameters.SlotHandle = EventData.AnchorSlotHandle;
	OnTileParametersUpdated(TileParameters);

	URockItemDragCarrySubsystem* carrySubsystem = URockItemDragCarrySubsystem::Get(GetOwningPlayer());
	if (!ensure(carrySubsystem))
	{
		return;
	}
	if (!carrySubsystem->IsCarrying())
	{
		return;
	}

	const URockItemDragDropOperation* dragDrop = Cast<URockItemDragDropOperation>(carrySubsystem->GetDragOperation());
	if (!dragDrop)
	{
		UE_LOG(LogRockInventoryUI, Warning, TEXT("OnGridSlotClicked: No appropriate drag operation"));
		return;
	}
	carrySubsystem->BumpDropEpoch();

	if (IsLeftClick(EventData.MouseEvent))
	{
		// Context
		FRockCarryDropContext dropContext;
		dropContext.MouseEvent = EventData.MouseEvent;
		dropContext.TargetObject = this;

		// Context Data
		UInventoryCarryContextData* contextData = NewObject<UInventoryCarryContextData>(GetOwningPlayer());
		contextData->TargetInventory = EventData.Inventory;
		contextData->TargetSlotHandle = EventData.SlotHandle;
		dropContext.ContextData = contextData;

		// ItemDropIndex is updated in the OnTileParametersUpdated
		contextData->TargetSlotHandle = BackgroundGridSlots[ItemDropIndex]->GetSlotHandle();

		carrySubsystem->RequestTargetDrop(dropContext);
	}
}

void URockInventory_ContainerBase::OnGridSlotReleased(const FRockGridSlotEventData& EventData)
{
	//if (false)
	{
		// // Disabling the 'drag hold drop' functionality for now
		//
		// TileParameters.TileQuadrant = EventData.TileQuadrant;
		// TileParameters.SlotHandle = EventData.AnchorSlotHandle;
		// OnTileParametersUpdated(TileParameters);
		// //UE_LOG(LogRockInventoryUI, Warning, TEXT(" GridSlotReleased: %s"), *InSlotHandle.ToString());
		// URockItemDragCarrySubsystem* carrySubsystem = URockItemDragCarrySubsystem::Get(GetOwningPlayer());
		// if (!carrySubsystem)
		// {
		// 	UE_LOG(LogRockInventoryUI, Warning, TEXT("OnGridSlotReleased: No carry subsystem"));
		// }
		// if (!carrySubsystem->IsCarrying())
		// {
		// 	return;
		// }
		//
		// // Something is wrong here
		// const URockItemDragDropOperation* dragDrop = Cast<URockItemDragDropOperation>(carrySubsystem->GetDragOperation());
		// if (dragDrop->SourceInventory == EventData.Inventory &&
		// 	(dragDrop->SourceSlotHandle == EventData.SlotHandle || dragDrop->SourceSlotHandle == EventData.AnchorSlotHandle))
		// {
		// 	// If the dragged slot is this one, ignore it as 'released'
		// 	//UE_LOG(LogRockInventoryUI, Warning, TEXT("OnGridSlotReleased: Ignoring release on same slot we are dragging from %s"), *InSlotHandle.ToString());
		// 	return;
		// }
		//
		// // If the dragged slot is this one, ignore it as 'released'
		//
		// carrySubsystem->BumpDropEpoch();
		// {
		// 	FRockCarryDropContext dropContext;
		// 	dropContext.MouseEvent = EventData.MouseEvent;
		// 	dropContext.TargetObject = this;
		//
		// 	{
		// 		UInventoryCarryContextData* contextData = NewObject<UInventoryCarryContextData>(GetOwningPlayer());
		// 		contextData->TargetInventory = EventData.Inventory;
		// 		contextData->TargetSlotHandle = EventData.SlotHandle;
		// 		dropContext.ContextData = contextData;
		// 	}
		//
		// 	carrySubsystem->RequestTargetDrop(dropContext);
		// }
	}
}

void URockInventory_ContainerBase::OnGridSlotHovered(const FRockGridSlotEventData& EventData)
{
	// UE_LOG(LogRockInventoryUI, Warning, TEXT("GridSlotHovered: %s"), *InSlotHandle.ToString());
}

void URockInventory_ContainerBase::OnGridSlotUnhovered(const FRockGridSlotEventData& EventData)
{
	// UE_LOG(LogRockInventoryUI, Warning, TEXT("GridSlotUnhovered: %s"), *InSlotHandle.ToString());
}

void URockInventory_ContainerBase::OnGridSlotMouseMoved(const FRockGridSlotEventData& EventData)
{
	// UE_LOG(LogRockInventoryUI, Warning, TEXT("Mouse Move on Slot %s, LocalTile %d, Quadrant: %d"), *EventData.SlotHandle.ToString(), EventData.LocalSectionIndex, (int32)EventData.TileQuadrant);
	//TileParameters.TileQuadrant = ERockInventory_TileQuadrant::None;
	//TileParameters.TileIndex = EventData.LocalSectionIndex;
	TileParameters.TileQuadrant = EventData.TileQuadrant;
	TileParameters.SlotHandle = EventData.AnchorSlotHandle;
	OnTileParametersUpdated(TileParameters);
	//TileParameters.TileIndex = INDEX_NONE;
	//TileParameters.TileCoordinates = FIntPoint::ZeroValue;
}

void URockInventory_ContainerBase::OnItemClicked(const FRockGridItemEventData& EventData)
{
	if (IsLeftClick(EventData.PointerEvent))
	{
		// UE_LOG(LogRockInventoryUI, Warning, TEXT("OnItemClicked: %s"), *InSlotHandle.ToString());
		if (auto* carrySubsystem = URockItemDragCarrySubsystem::Get(GetOwningPlayer()))
		{
			carrySubsystem->BumpDropEpoch();

			if (carrySubsystem->IsCarrying())
			{
				const URockItemDragDropOperation* drag = Cast<URockItemDragDropOperation>(carrySubsystem->GetDragOperation());
				if (drag && drag->SourceInventory == EventData.Inventory && drag->SourceSlotHandle == EventData.SlotHandle)
				{
					// Clicking on the same item we are carrying - cancel the carry
					FRockDropOutcome Outcome;
					Outcome.Result = ERockDropResult::Canceled;
					carrySubsystem->CancelCarry(Outcome);
					return;
				}
				return;
			}
			else
			{
				if (PickUp(EventData.Inventory, EventData.SlotHandle))
				{
					// TODO: do later
					//EventData.SlotWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
			}
		}
	}
}

void URockInventory_ContainerBase::OnItemReleased(const FRockGridItemEventData& EventData)
{
	//UE_LOG(LogRockInventoryUI, Warning, TEXT("OnItemReleased: %s"), *InSlotHandle.ToString());
}

void URockInventory_ContainerBase::OnItemHovered(const FRockGridItemEventData& EventData)
{
	//UE_LOG(LogRockInventoryUI, Warning, TEXT("OnItemHovered: %s"), *InSlotHandle.ToString());
}

void URockInventory_ContainerBase::OnItemUnhovered(const FRockGridItemEventData& EventData)
{
	//UE_LOG(LogRockInventoryUI, Warning, TEXT("OnItemUnhovered: %s"), *InSlotHandle.ToString());
}


void URockInventory_ContainerBase::OnItemChanged(URockInventory* InInventory, const FRockItemStackHandle& InItemHandle)
{
	// Update the relevant ItemSlot widgets
}


void URockInventory_ContainerBase::OnSlotChanged(const FRockSlotDelta& SlotDelta)
{
	// Check if this slot belongs to this section
	if (!Inventory || SlotDelta.Inventory != Inventory || SlotDelta.SlotHandle.GetSectionIndex() != TabInfo.GetSectionIndex())
	{
		return;
	}

	if (SlotDelta.ChangeType == ERockSlotChangeType::PendingChange)
	{
		UE_LOG(LogRockInventoryUI,
			Warning,
			TEXT("!!!! URockInventory_Slot_ItemBase::OnInventorySlotChanged: %s %d"),
			*SlotDelta.SlotHandle.ToString(),
			SlotDelta.ChangeType);
		// Ignore Pending for now?
		// return;
	}

	// 2) Current state of the slot
	const FRockInventorySlotHandle slotHandle = SlotDelta.SlotHandle;
	const FRockItemStack currentItem = Inventory->GetItemBySlotHandle(slotHandle);
	const FRockItemStackHandle prevItemHandle = SlotToItem.FindRef(slotHandle);

	//  
	if (currentItem.IsValid())
	{
		// Create/Update/Move
		EnsureWidgetForItem(currentItem, slotHandle);
		return;
	}

	// Nothing to really do, other than possibly clean up stale mappings
	SlotToItem.Remove(slotHandle);
	if (!prevItemHandle.IsValid())
	{
		return;
	}

	// If another slot has this item, it means it was a move, skip destroying
	if (const FRockContainerItemView* known = ItemViews.Find(prevItemHandle))
	{
		if (known->AnchorSlot.IsValid() && known->AnchorSlot != slotHandle)
		{
			// It moved, don't destroy
			return;
		}
	}

	// prevent double-scheduling
	// Not needed because we already 'early out of schedule' CancelPendingDestroy(PrevItemHandle);

	// otherwise, schedule a deferred destroy on the next tick. If it reappears later in this tick, it should cancel the destroy request
	ScheduleDeferredDestroy(prevItemHandle);
}


void URockInventory_ContainerBase::CancelPendingDestroy(const FRockItemStackHandle& ItemHandle)
{
	FTimerHandle timerHandle;
	// Atomically finds the timer, copies it, and removes the entry from the map.
	if (PendingDestroyTimers.RemoveAndCopyValue(ItemHandle, timerHandle))
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(timerHandle);
		}
	}
}

void URockInventory_ContainerBase::ScheduleDeferredDestroy(const FRockItemStackHandle& ItemHandle)
{
	if (!ItemHandle.IsValid() || PendingDestroyTimers.Contains(ItemHandle) || !GetWorld())
	{
		return;
	}

	const FTimerDelegate timerDelegate = FTimerDelegate::CreateWeakLambda(this,
		[this, ItemHandle]()
		{
			PendingDestroyTimers.Remove(ItemHandle);
			DestroyWidgetForItem(ItemHandle);
		});

	const FTimerHandle Handle = GetWorld()->GetTimerManager().SetTimerForNextTick(timerDelegate);
	PendingDestroyTimers.Add(ItemHandle, Handle);
}

void URockInventory_ContainerBase::DestroyWidgetForItem(const FRockItemStackHandle& ItemHandle)
{
	if (!ItemHandle.IsValid())
	{
		return;
	}

	// Ensure we are not double-destroying
	CancelPendingDestroy(ItemHandle);

	// final check to see if the item still exists in the inventory
	if (Inventory)
	{
		const FRockItemStack itemStack = Inventory->GetItemByHandle(ItemHandle);
		if (itemStack.IsValid())
		{
			// This triggers when moving item from one container to another
			// So it might still exist in the inventory, but not in this 'section'.
			// Compare slot entry section to this container section
			const FRockInventorySlotEntry slotEntry = Inventory->GetSlotByItemHandle(ItemHandle);
			if (TabInfo.GetSectionIndex() == slotEntry.SlotHandle.GetSectionIndex())
			{
				ensureMsgf(false, TEXT("DestroyWidgetForItem: Item %s still exists in inventory and this section, not destroying"), *ItemHandle.ToString());
				UE_LOG(LogRockInventoryUI,
					Warning,
					TEXT("DestroyWidgetForItem: Item %s still exists in inventory, not destroying"),
					*ItemHandle.ToString());
				// Item still exists, do not destroy
				return;
			}
			else
			{
				// It moved to another section, we can destroy the widget
				// Since it will be recreated in the other section.
				// Later on, we can perhaps attempt to pool it, or move the ownership of ItemWidgets to some parent item 'manager' widget?
				
			}
		}
	}

	FRockContainerItemView itemView;
	if (!ItemViews.RemoveAndCopyValue(ItemHandle, itemView))
	{
		return; // Nothing to destroy
	}

	const FRockInventorySlotHandle lastAnchor = itemView.AnchorSlot;
	const FIntPoint size = itemView.Size;

	// Clear all mappings
	ItemViews.Remove(ItemHandle);
	SlotToItem.Remove(lastAnchor);

	// Clear anchors if we still know the last slot
	if (lastAnchor.IsValid() && Inventory)
	{
		const int32 absIndex = lastAnchor.GetAbsoluteIndex();

		const int32 localIndex = TabInfo.GetLocalIndex(absIndex);

		ForEachSlot(localIndex,
			size,
			[&](int32 SlotIndex, int32 C, int32 R, int32 AbsIdx)
			{
				if (URockInventory_Slot_BackgroundBase* gridSlot = BackgroundGridSlots[SlotIndex])
				{
					// Clear anchor if it was set to this slot
					if (gridSlot->GetAnchorItemSlotHandle() == lastAnchor)
					{
						gridSlot->ResetAnchorItemSlotHandle();
					}
				}
			});
	}

	// Destroy the widget
	if (URockInventory_Slot_ItemBase* itemWidget = itemView.Widget.Get())
	{
		itemWidget->RemoveFromParent();
	}
}

void URockInventory_ContainerBase::BindToInventorySection(URockInventory* NewInventory, FName InSectionName)
{
	// Use the current section name if we have one
	if (InSectionName.IsNone())
	{
		InSectionName = TabInfo.GetSectionName();
	}
	if (InSectionName.IsNone())
	{
		// Is there another kind of fallback that we should/could support?
		// e.g. the first section in the inventory?
		UE_LOG(LogRockInventoryUI, Warning, TEXT("BindToInventorySection: No section name provided or cached"));
		return;
	}

	if (NewInventory)
	{
		// 1) Unbind from the current inventory (if any)
		if (Inventory)
		{
			Inventory->OnItemChanged.RemoveDynamic(this, &ThisClass::OnItemChanged);
			Inventory->OnSlotChanged.RemoveDynamic(this, &ThisClass::OnSlotChanged);
		}
		// 2) Ensure we won't double-bind to the incoming inventory
		NewInventory->OnItemChanged.RemoveDynamic(this, &ThisClass::OnItemChanged);
		NewInventory->OnSlotChanged.RemoveDynamic(this, &ThisClass::OnSlotChanged);

		// 3) Swap to the new inventory and cache section info
		Inventory = NewInventory;
		TabInfo = Inventory->GetSectionInfo(InSectionName);
		SizePolicy = TabInfo.GetSlotSizePolicy();

		// 4) Bind
		Inventory->OnItemChanged.AddDynamic(this, &ThisClass::OnItemChanged);
		Inventory->OnSlotChanged.AddDynamic(this, &ThisClass::OnSlotChanged);
	}
	else
	{
		SizePolicy = ERockItemSizePolicy::RespectSize;

		// Leave TabInfo as 'default'. Or should we 'force' a reset or something?
	}
	CreateItemsPanel();
	GenerateGrid();
	GenerateItems();
}

void URockInventory_ContainerBase::CreateItemsPanel()
{
	ItemsOverlayPanel = nullptr;
	ItemsCanvasPanel = nullptr;

	UOverlaySlot* overlaySlot = nullptr;
	if (TabInfo.GetNumSlots() > 1)
	{
		ItemsCanvasPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ItemsCanvasPanel"));
		overlaySlot = OverlayPanelRoot->AddChildToOverlay(ItemsCanvasPanel);
	}
	else
	{
		ItemsOverlayPanel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ItemsOverlayPanel"));
		overlaySlot = OverlayPanelRoot->AddChildToOverlay(ItemsOverlayPanel);
	}
	if (overlaySlot)
	{
		overlaySlot->SetHorizontalAlignment(HAlign_Fill);
		overlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
}


void URockInventory_ContainerBase::ForceRefreshInventory()
{
	GenerateGrid();
	GenerateItems();
}

void URockInventory_ContainerBase::GenerateGrid()
{
	BackgroundGridSlots.Empty();
	if (!ensure(GridPanel))
	{
		return;
	}
	GridPanel->ClearChildren();
	if (!GetWorld())
	{
		// No world, probably means we are in design time
		return;
	}

	int32 SectionIndex = INDEX_NONE;
	if (Inventory)
	{
		SectionIndex = Inventory->GetSectionIndexById(TabInfo.GetSectionName());
	}

	if (!GridSlotWidgetClass)
	{
		UE_LOG(LogRockInventoryUI, Warning, TEXT("GenerateGrid: GridSlotWidgetClass is null"));
		return;
	}

	ForEachSlot([this, SectionIndex](int32 SlotIndex, int32 Column, int32 Row, int32 AbsoluteIndex)
	{
		URockInventory_Slot_BackgroundBase* backgroundSlotWidget = WidgetTree->ConstructWidget<URockInventory_Slot_BackgroundBase>(GridSlotWidgetClass);
		checkf(backgroundSlotWidget, TEXT("Failed to create background slot widget"));

		BackgroundGridSlots.Add(backgroundSlotWidget);

		backgroundSlotWidget->GridSlotClicked.AddDynamic(this, &ThisClass::OnGridSlotClicked);
		backgroundSlotWidget->GridSlotReleased.AddDynamic(this, &ThisClass::OnGridSlotReleased);
		backgroundSlotWidget->GridSlotHovered.AddDynamic(this, &ThisClass::OnGridSlotHovered);
		backgroundSlotWidget->GridSlotUnhovered.AddDynamic(this, &ThisClass::OnGridSlotUnhovered);
		backgroundSlotWidget->GridSlotMouseMoved.AddDynamic(this, &ThisClass::OnGridSlotMouseMoved);

		// Relative slot to the section
		backgroundSlotWidget->SetSlot(Inventory, FRockInventorySlotHandle(SectionIndex, AbsoluteIndex));
		backgroundSlotWidget->SetSize(TileSize);
		
		UGridSlot* gridSlotWidget = GridPanel->AddChildToGrid(backgroundSlotWidget, Row, Column);
		if (gridSlotWidget)
		{
			gridSlotWidget->SetHorizontalAlignment(HAlign_Center);
			gridSlotWidget->SetVerticalAlignment(VAlign_Top);
			gridSlotWidget->SetPadding(0);
			gridSlotWidget->SetColumnSpan(1);
			gridSlotWidget->SetRowSpan(1);
			// Move to Background
			gridSlotWidget->SetLayer(-100);
		}
	});
}


void URockInventory_ContainerBase::ClearItems()
{
	if (ItemsCanvasPanel)
	{
		ItemsCanvasPanel->ClearChildren();
	}
	if (ItemsOverlayPanel)
	{
		ItemsOverlayPanel->ClearChildren();
	}
}

void URockInventory_ContainerBase::UpdateWidgetForItem(
	URockInventory_Slot_ItemBase* WidgetItem, const FRockItemStack& ItemStack, FRockInventorySlotHandle SlotHandle)
{
	if (!Inventory || !WidgetItem || !SlotHandle.IsValid())
	{
		return;
	}

	// Index and Position
	const int32 absIndex = SlotHandle.GetAbsoluteIndex();
	const int32 localIndex = TabInfo.GetLocalIndex(absIndex);
	const FIntPoint tilePosition = GetCoordinatesFromIndex(localIndex);

	// Size in tiles
	const FIntPoint itemGridSize = (SizePolicy == ERockItemSizePolicy::IgnoreSize)
		                               ? FIntPoint(1)
		                               : URockItemStackLibrary::GetItemSize(ItemStack);

	// Pixels/Layout
	const FVector2D pixelPosition = FVector2D(tilePosition.X * TileSize, tilePosition.Y * TileSize);
	const FVector2D pixelSize = itemGridSize * TileSize;

	// Layout into Canvas if present
	if (ItemsCanvasPanel)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(WidgetItem->Slot))
		{
			CanvasSlot->SetPosition(pixelPosition);
			CanvasSlot->SetSize(pixelSize);
			CanvasSlot->SetAutoSize(false);
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		}
	}

	// --- per-item view record ---
	FRockContainerItemView& itemView = ItemViews.FindOrAdd(ItemStack.ItemHandle);

	const FRockInventorySlotHandle oldAnchor = itemView.AnchorSlot;
	const FIntPoint oldSize = itemView.Size;
	const int32 oldLocalIndex = TabInfo.GetLocalIndex(oldAnchor.GetAbsoluteIndex());

	if (oldAnchor.IsValid())
	{
		// Clear OLD anchors
		ForEachSlot(oldLocalIndex,
			oldSize,
			[&](int32 SlotIndex, int32 Column, int32 Row, int32 AbsoluteIndex)
			{
				if (URockInventory_Slot_BackgroundBase* gridSlot = BackgroundGridSlots[SlotIndex])
				{
					// Only clear if it still points to THIS item's old anchor
					if (gridSlot->GetAnchorItemSlotHandle() == oldAnchor)
					{
						gridSlot->ResetAnchorItemSlotHandle();
					}
				}
			});
	}

	// Update view record
	itemView.AnchorSlot = SlotHandle;
	itemView.Widget = WidgetItem;
	itemView.Size = itemGridSize;

	// Update background anchors for all covered tiles
	ForEachSlot(localIndex,
		itemGridSize,
		[&](int32 SlotIndex, int32 Column, int32 Row, int32 AbsoluteIndex)
		{
			if (URockInventory_Slot_BackgroundBase* gridSlot = BackgroundGridSlots[SlotIndex])
			{
				gridSlot->SetAnchorItemSlotHandle(SlotHandle);
				// Optionally set occupancy visuals here
				// BG->SetOccupiedTexture();
			}
		});

	if (WidgetItem && IsValid(WidgetItem))
	{
		WidgetItem->InitializeItem(Inventory, SlotHandle, TileSize);

		// Set the item slot to hit testable again
		//WidgetItem->SetVisibility(ESlateVisibility::Visible);
	}


	// Update reverse index
	SlotToItem.Remove(oldAnchor);
	SlotToItem.FindOrAdd(SlotHandle) = ItemStack.ItemHandle;
}


void URockInventory_ContainerBase::GenerateItems()
{
	if (!Inventory)
	{
		UE_LOG(LogRockInventoryUI, Warning, TEXT( "GenerateItems: Inventory is null"));
		return;
	}

	ClearItems();

	//////////////////////////////////////////////////////////////////////////
	/// Calculate uniform grid cell size to calculate proper positions of canvas slots
	// float CellWidth = 0.0f;
	// float CellHeight = 0.0f;
	//
	// // If we couldn't get cell size from existing children, use default values
	// if (CellWidth <= 0.0f || CellHeight <= 0.0f)
	// {
	// 	CellWidth = 64.0f; // Default cell width, adjust as needed
	// 	CellHeight = 64.0f; // Default cell height, adjust as needed
	// }
	//////////////////////////////////////////////////////////////////////////

	for (int32 slotIndex = 0; slotIndex < TabInfo.GetNumSlots(); ++slotIndex)
	{
		const int32 AbsoluteIndex = TabInfo.GetFirstSlotIndex() + slotIndex;

		FRockInventorySlotEntry slotEntry = Inventory->GetSlotByAbsoluteIndex(AbsoluteIndex);
		if (!slotEntry.SlotHandle.IsValid())
		{
			continue;
		}
		const FRockItemStack newItemStack = Inventory->GetItemByHandle(slotEntry.ItemHandle);
		EnsureWidgetForItem(newItemStack, slotEntry.SlotHandle);
	}
}

void URockInventory_ContainerBase::EnsureWidgetForItem(const FRockItemStack& ItemStack, FRockInventorySlotHandle SlotHandle)
{
	if (!Inventory || !ItemStack.IsValid() || !SlotHandle.IsValid())
	{
		// invalid parameters
		return;
	}

	CancelPendingDestroy(ItemStack.ItemHandle);

	// 1. Reuse existing widget if it exists
	FRockContainerItemView& itemView = ItemViews.FindOrAdd(ItemStack.ItemHandle);
	if (URockInventory_Slot_ItemBase* existingWidget = itemView.Widget.Get())
	{
		UpdateWidgetForItem(existingWidget, ItemStack, SlotHandle);
		SlotToItem.FindOrAdd(SlotHandle) = ItemStack.ItemHandle;
		itemView.AnchorSlot = SlotHandle;
		return;
	}

	// 2. Create new widget
	const int32 absoluteSlotIndex = SlotHandle.GetAbsoluteIndex();
	URockInventory_Slot_ItemBase* widgetItem = WidgetTree->ConstructWidget<URockInventory_Slot_ItemBase>(ItemSlotWidgetClass);
	if (!widgetItem)
	{
		ensureMsgf(false, TEXT("AddItemSlot: Failed to create item slot widget"));
		UE_LOG(LogRockInventoryUI, Error, TEXT("AddItemSlot: Failed to create item slot widget"));
		return;
	}

	// Init
	widgetItem->InitializeItem(Inventory, SlotHandle, TileSize);
	widgetItem->ItemClicked.AddDynamic(this, &ThisClass::OnItemClicked);
	widgetItem->ItemReleased.AddDynamic(this, &ThisClass::OnItemReleased);
	widgetItem->ItemHovered.AddDynamic(this, &ThisClass::OnItemHovered);
	widgetItem->ItemUnhovered.AddDynamic(this, &ThisClass::OnItemUnhovered);


	if (ItemsCanvasPanel)
	{
		UCanvasPanelSlot* canvasSlot = ItemsCanvasPanel->AddChildToCanvas(widgetItem);
		checkfSlow(canvasSlot, TEXT("Failed to add item slot to canvas"));
		canvasSlot->SetAutoSize(false);
		canvasSlot->SetAlignment(FVector2D::ZeroVector);
	}
	else if (ItemsOverlayPanel)
	{
		UOverlaySlot* overlaySlot = ItemsOverlayPanel->AddChildToOverlay(widgetItem);
		checkfSlow(overlaySlot, TEXT("Failed to add item slot to overlay"));
		overlaySlot->SetHorizontalAlignment(HAlign_Fill);
		overlaySlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Update position/Size + Anchor Background tiles
	UpdateWidgetForItem(widgetItem, ItemStack, SlotHandle);

	// Update IndexMaps
	itemView.AnchorSlot = SlotHandle;
	itemView.Widget = widgetItem;
	itemView.Size = URockItemStackLibrary::GetItemSize(ItemStack);
	SlotToItem.FindOrAdd(SlotHandle) = ItemStack.ItemHandle;
	return;
}


bool URockInventory_ContainerBase::IsLeftClick(const FPointerEvent& InPointerEvent) const
{
	return InPointerEvent.GetEffectingButton() == EKeys::LeftMouseButton;
}

// Right Click is 'secondary' button. How to translate to gamepad?
bool URockInventory_ContainerBase::IsRightClick(const FPointerEvent& InPointerEvent) const
{
	return InPointerEvent.GetEffectingButton() == EKeys::RightMouseButton;
}
