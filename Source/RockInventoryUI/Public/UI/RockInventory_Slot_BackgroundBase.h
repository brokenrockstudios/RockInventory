// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RockInventory_ContainerBase.h"
#include "Inventory/RockSlotHandle.h"
#include "Shared/RockGridSlotEventData.h"
#include "RockInventory_Slot_BackgroundBase.generated.h"

class USizeBox;
class UImage;
class URockInventory;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridSlotEvent, const FRockGridSlotEventData&, EventData);

/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_Slot_BackgroundBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	URockInventory_Slot_BackgroundBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void NativeDestruct() override;

	FGridSlotEvent GridSlotClicked;
	FGridSlotEvent GridSlotReleased;
	FGridSlotEvent GridSlotHovered;
	FGridSlotEvent GridSlotUnhovered;
	FGridSlotEvent GridSlotMouseMoved;

private:
	//─────────────────────────────────────────────────────────────────────────
	// We might not need this, but it is here for now. This is the inventory that this slot is representing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URockInventory> Inventory;
	// Information about this slot, the TabIndex, X, and Y position. So that drag/drop and other things can know what we are hovering over
	// or dropping on.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot", meta = (AllowPrivateAccess = "true"))
	FRockInventorySlotHandle SlotHandle;

	// If this slot is occupied by an item that is larger than 1x1, this will be the handle of the 'upper left' corner of that item.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot", meta = (AllowPrivateAccess = "true"))
	FRockInventorySlotHandle AnchorHandle;

	//─ Widgets ───────────────────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_GridSlot;

	UPROPERTY( meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox;

#pragma region "Widget Bindings"
	//─────────────────────────────────────────────────────────────────────────
	// Empty slot, default state.
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Unoccupied;

	// During Hover and always on Item exists background? 
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Occupied;

	// Seemingly never used?
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_Selected;

	// Used when hovering over an item that already has an item? 
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FSlateBrush Brush_GrayedOut;
#pragma endregion

	// In the event we had some information like if this slot is 'locked' or 'expecting a specific item
	// we might bind that here
	// We might bind some 'color highlight' and other functionality here.
	// The item itself will exist on the canvas panel in front of this background widget.

	// Used when this 'slot' or 'item' is locked
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	// TObjectPtr<UImage> ItemLockIcon;

	//─────────────────────────────────────────────────────────────────────────
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	//─ V2 ────────────────────────────────────────────────────────────────────
public:
	void SetOccupiedTexture();
	void SetUnoccupiedTexture();
	void SetSelectedTexture();
	void SetGrayedOutTexture();

	void SetSlot(URockInventory* InInventory, FRockInventorySlotHandle InSlotHandle);
	void SetSize(float InTileSize);

	
	// Sets the handle of the item that is occupying this slot. This is used for drag/drop and other functionality.
	void SetAnchorItemSlotHandle(const FRockInventorySlotHandle& InHandle);
	void ResetAnchorItemSlotHandle();
	
	FRockInventorySlotHandle GetSlotHandle() const;
	FRockInventorySlotHandle GetAnchorItemSlotHandle() const;

private:
	// ── Utility Functions ──
	FRockGridSlotEventData CreateEventData(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
	ERockInventory_TileQuadrant CalculateTileQuadrant(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) const;
};
