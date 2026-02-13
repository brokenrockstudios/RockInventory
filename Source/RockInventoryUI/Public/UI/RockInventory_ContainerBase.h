// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Core/RockDropCarryReceiver.h"
#include "Inventory/RockInventorySectionInfo.h"
#include "Inventory/RockInventorySlot.h"
#include "Inventory/Events/RockSlotDelta.h"
#include "Shared/RockContainerItemView.h"
#include "Shared/RockGridItemEventData.h"
#include "Shared/RockGridSlotEventData.h"
#include "Shared/RockInventory_SpaceQueryResult.h"
#include "Shared/RockInventory_TileParameters.h"
#include "Shared/RockInventory_TileQuadrant.h"
#include "RockInventory_ContainerBase.generated.h"


// Forward Declarations
class UOverlay;
class URockDragCarryOperation;
class URockInventory_HoverItem;
class URockInventory_Slot_BackgroundBase;
class URockInventory_Slot_ItemBase;
class UGridPanel;
class UCanvasPanel;
class UUniformGridPanel;
class URockInventory;

namespace RockInventoryUI
{
template <typename FuncT>
concept SlotFunction = requires(FuncT f, int32 a, int32 b, int32 c, int32 d)
{
	f(a, b, c, d);
};
}

/**
 * InventoryGrid equivalent?
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_ContainerBase : public UCommonUserWidget, public IRockDropCarryReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnCarryEnded(const URockDragCarryOperation* Operation);

	URockInventory_Slot_ItemBase* FindItemSlotWidgetBySlotHandle(const FRockInventorySlotHandle& InSlotHandle) const;
	URockInventory_Slot_ItemBase* FindItemSlotWidgetByItemHandle(const FRockItemStackHandle& InItemHandle) const;


	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	// void UpdateTileParameters(const FVector2D& CanvasPosition, const FVector2D& MousePosition);
	ERockInventory_TileQuadrant CalculateTileQuadrant(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const;
	FIntPoint CalculateHoveredCoordinates(const FVector2D& CanvasPosition, const FVector2D& MousePosition) const;

	FIntPoint GetCoordinatesFromIndex(int32 Index) const;
	int32 GetIndexFromPosition(const FIntPoint& Position) const;

	bool IsInGridBounds(int32 StartIndex, FIntPoint ItemGridSize) const;

	void OnTileParametersUpdated(const FRockInventory_TileParameters& Parameters);
	void HighlightSlots(int32 Index, const FIntPoint& Dimensions);
	void UnHighlightLast();
	void UnHighlightSlots(int32 Index, const FIntPoint& Dimensions);
	int32 GetIndexFromPosition(const FIntPoint& Position, int32 Columns) const;
	bool HasItemAtAbsoluteIndex(int32 AbsoluteIndex) const;
	FIntPoint CalculateStartingCoordinate(const FIntPoint& Center, const FIntPoint& Size, const ERockInventory_TileQuadrant Quadrant) const;

private:
	FRockInventory_SpaceQueryResult QueryHoverSpace(FIntPoint Position, FIntPoint Dimensions) const;

protected:
	bool PickUp(URockInventory* InInventory, const FRockInventorySlotHandle& InSlotHandle);

public:
	virtual float GetTileSize() const { return TileSize; }

	bool bMouseWithinCanvas = false;
	int32 ItemDropIndex = INDEX_NONE;
	int32 LastHighlightedIndex;
	FIntPoint LastHighlightedDimensions;
	FRockInventory_SpaceQueryResult CurrentQueryResult;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float TileSize = 64.0f;

	/** Drag/carry operation class to spawn when this slot begins a carry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|DragCarry")
	TSubclassOf<URockDragCarryOperation> DragCarryOperationClass;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<URockInventory_HoverItem> HoverItemClass;

	// The current tile parameters based on mouse position
	// FRockInventory_TileParameters TileParameters;
	// The last tile parameters used for comparison
	FRockInventory_TileParameters TileParameters;


	// Apply to a sub-region of the grid
	template <RockInventoryUI::SlotFunction FuncT>
	void ForEachSlot(int32 StartIndex, const FIntPoint& Size, FuncT&& Function) const;

	// Apply to the entire grid
	template <RockInventoryUI::SlotFunction FuncT>
	void ForEachSlot(FuncT&& Function) const;

	
	template <RockInventoryUI::SlotFunction FuncT>
	void ForEachSlotInternal(int32 StartIndex, int32 RangeWidth, int32 RangeHeight, FuncT&& Function) const;

	// FOOD4THOUGHT:
	// Consider leveraging a Canvas in any container that is larger than a '1x1' (equipment slot) container.
	// This would allow for more flexible layouts, and allow for overlapping slots.
	// It would also allow for more flexible item dragging, and relocation. 
	// UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidget))
	// TObjectPtr<UCanvasPanel> Canvas;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidget))
	TObjectPtr<UGridPanel> GridPanel = nullptr;

	// Used for containers of size 1x1 and stores the CanvasPanel 
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidget))
	TObjectPtr<UOverlay> OverlayPanelRoot = nullptr;

	// Used for slots bigger than 1x1
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UCanvasPanel> ItemsCanvasPanel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UOverlay> ItemsOverlayPanel = nullptr;


	// The actual slots in the grid panel.
	// Should be in the order of the grid (row by row, left to right)
	UPROPERTY()
	TArray<TObjectPtr<URockInventory_Slot_BackgroundBase>> BackgroundGridSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<URockInventory_Slot_ItemBase> ItemSlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<URockInventory_Slot_BackgroundBase> GridSlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<URockInventory> Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FRockInventorySectionInfo TabInfo;

	UFUNCTION()
	void OnItemChanged(URockInventory* InInventory, const FRockItemStackHandle& InItemHandle);

	UFUNCTION()
	void OnSlotChanged(const FRockSlotDelta& SlotDelta);

	void CreateItemsPanel();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void BindToInventorySection(URockInventory* NewInventory, FName InSectionName = NAME_None);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ForceRefreshInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void GenerateGrid();


	// ~Begin IRockDropCarryReceiver interface
	virtual bool CanAcceptCarryDrop(const URockDragCarryOperation* Operation) const override;
	virtual FRockDropOutcome OnCarryDrop(const URockDragCarryOperation* Operation, const FRockCarryDropContext& DropContext) override;
	// ~End IRockDropCarryReceiver interface

	int32 GetLocalIndexFromSlotHandle(const FRockInventorySlotHandle& SlotHandle) const;
	// ~Begin Background slot events
	UFUNCTION()
	void OnGridSlotClicked(const FRockGridSlotEventData& EventData);
	UFUNCTION()
	void OnGridSlotReleased(const FRockGridSlotEventData& EventData);
	UFUNCTION()
	void OnGridSlotHovered(const FRockGridSlotEventData& EventData);
	UFUNCTION()
	void OnGridSlotUnhovered(const FRockGridSlotEventData& EventData);
	UFUNCTION()
	void OnGridSlotMouseMoved(const FRockGridSlotEventData& EventData);
	// ~End Background slot events

	// ~Begin Item slot events
	UFUNCTION()
	void OnItemClicked(const FRockGridItemEventData& EventData);
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnItemReleased(const FRockGridItemEventData& EventData);
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnItemHovered(const FRockGridItemEventData& EventData);
	UFUNCTION()
	void OnItemUnhovered(const FRockGridItemEventData& EventData);
	// ~End Item slot events

	void ClearItems();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void GenerateItems();
	bool IsLeftClick(const FPointerEvent& InPointerEvent) const;
	bool IsRightClick(const FPointerEvent& InPointerEvent) const;

private:
	// Cached Information regarding this container
	// Updates during InitializeInventory
	ERockItemSizePolicy SizePolicy = ERockItemSizePolicy::RespectSize;


	void CancelPendingDestroy(const FRockItemStackHandle& ItemHandle);
	void ScheduleDeferredDestroy(const FRockItemStackHandle& ItemHandle);
	void DestroyWidgetForItem(const FRockItemStackHandle& ItemHandle);

	void UpdateWidgetForItem(URockInventory_Slot_ItemBase* WidgetItem, const FRockItemStack& ItemStack, FRockInventorySlotHandle SlotHandle);
	void EnsureWidgetForItem(const FRockItemStack& ItemStack, FRockInventorySlotHandle SlotHandle);

	
	//void MoveWidgetForItem(URockInventory_Slot_ItemBase* WidgetItem, const FRockInventorySlotHandle& From, const FRockInventorySlotHandle& To);

	TMap<FRockItemStackHandle, FRockContainerItemView> ItemViews;

	// This in theory could be replaced from a TMap to a fixed TArray 1:1
	// TArray<FRockItemStackHandle> SlotOwner;
	// with the slots in the grid. Would be slightly more performant and smaller size
	TMap<FRockInventorySlotHandle, FRockItemStackHandle> SlotToItem;

	TMap<FRockItemStackHandle, FTimerHandle> PendingDestroyTimers;
};


template <RockInventoryUI::SlotFunction FuncT>
void URockInventory_ContainerBase::ForEachSlotInternal(int32 StartIndex, int32 RangeWidth, int32 RangeHeight, FuncT&& Function) const
{
	if (StartIndex < 0)
	{
		// Invalid start index, nothing to do
		return;
	}
	
	const int32 gridWidth = TabInfo.GetColumns();
	const int32 gridHeight = TabInfo.GetRows();
	if (gridWidth <= 0 || gridHeight <= 0)
	{
		// Invalid grid dimensions, nothing to do
		return;
	}

	// ── Convert StartIndex -> (col,row) ─────────────────────────────────────
	const int32 startCol = StartIndex % gridWidth;
	const int32 startRow = StartIndex / gridWidth;

	if (startCol >= gridWidth || startRow >= gridHeight || RangeWidth <= 0 || RangeHeight <= 0)
	{
		// degenerate rectangle – nothing to do
		return;
	}

	// ── Clamp the rectangle so we never read past the tab’s bounds ──────────
	const int32 endCol = FMath::Min(startCol + RangeWidth, gridWidth); // exclusive
	const int32 endRow = FMath::Min(startRow + RangeHeight, gridHeight); // exclusive

	// ────────────────────────────────────────────────────────────────────────
	for (int32 row = startRow; row < endRow; ++row)
	{
		// first local index in this row
		const int32 rowBase = row * gridWidth;

		for (int32 col = startCol; col < endCol; ++col)
		{
			const int32 localIndex = rowBase + col;
			const int32 absoluteIndex = TabInfo.GetFirstSlotIndex() + localIndex;

			std::forward<FuncT>(Function)(localIndex, col, row, absoluteIndex);
		}
	}
}

template <RockInventoryUI::SlotFunction FuncT>
void URockInventory_ContainerBase::ForEachSlot(FuncT&& Function) const
{
	ForEachSlotInternal(0, TabInfo.GetColumns(), TabInfo.GetRows(), Function);
}


template <RockInventoryUI::SlotFunction FuncT>
void URockInventory_ContainerBase::ForEachSlot(int32 StartIndex, const FIntPoint& Size, FuncT&& Function) const
{
	ForEachSlotInternal(StartIndex, Size.X, Size.Y, Function);
}
