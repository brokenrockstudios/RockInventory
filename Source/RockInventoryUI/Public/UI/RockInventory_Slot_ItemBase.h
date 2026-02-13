// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RockInventory_ContainerBase.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemDefinition.h"
#include "RockInventory_Slot_ItemBase.generated.h"

class USizeBox;
struct FStreamableHandle;
// Forward declaration
class UImage;
class UTextBlock;
class URockInventory;
class URockDragCarryOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridSlotItemEvent, const FRockGridItemEventData&, EventData);

/**
 * The visual widget for an inventory "item slot" that lives in a ContainerBase
 * - Responsible for icon, count text, and lock/loading indicators
 * - Handles hover/tooltip timing and mouse / key input
 * - Subscribes to inventory slot/item change events and updates presentation
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_Slot_ItemBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	// ---------- Data/Identity ----------
	/** The inventory this slot belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Slot")
	TObjectPtr<URockInventory> Inventory;

	/** The logical slot handle within the inventory. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Slot")
	FRockInventorySlotHandle SlotHandle;

	/** The item stack in this slot (if any). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Slot")
	FRockItemStackHandle ItemHandle;

	/** (Optional) fallback icon if the actual icon fails or is missing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Inventory|Presentation")
	TObjectPtr<UTexture2D> FallbackIcon;

	/** UI data (icon / tint / etc.) currently applied to this slot. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory|Presentation")
	FRockItemUIData IconData;

	// ---------- Bound Widgets ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<USizeBox> OverallSize;
	
	/** Main icon image. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	/** Lock overlay when the item is in use / reserved / being dragged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemLockIcon;

	/** Stack count text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCount;

	/** Simple loading / streaming indicator overlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidgetOptional))
	TObjectPtr<UImage> LoadingIndicator;
	FIntPoint parentContainersTileSize;

private:
	// ---------- Lifecycle ----------
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Unbind delegates, clear timers, cancel streaming. */
	void Cleanup();

	// ---------- Inventory Updates ----------
	/** Update the stack count text from the current ItemHandle. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateItemCount();

	/** Callback: a slot changed within the observed inventory. */
	// UFUNCTION()
	// void OnInventorySlotChanged(const FRockSlotDelta& SlotDelta);
	/** Callback: an item stack changed within the observed inventory. */
	// UFUNCTION()
	// void OnInventoryItemChanged(URockInventory* ChangedInventory, const FRockItemStackHandle& ChangedItemHandle);

	// ---------- Icon / Streaming ----------
	/** Apply icon/tint/etc. for this item. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetIconData(const FRockItemUIData& InIconData);

	/** Drive the loading indicator on/off. */
	void SetIsLoading(bool bIsLoading);

	/** Called when async icon streaming completes. */
	void OnIconLoaded();
public:
	void InitializeItem(URockInventory* InInventory, const FRockInventorySlotHandle& InSlotHandle, float tileSize);
	void Update();

protected:
	// ---------- Input (Keyboard/Mouse) ----------
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

protected:
	// ---------- Tooltip / Hover Timing ----------
	/** Arm the hover to show a tooltip after a delay. */
	void ArmHover();

	/** Disarm hover; if bGracefulHide, delay hide slightly to prevent flicker. */
	void DisarmHover(bool bGracefulHide);

	/** Force tooltip to appear immediately. */
	void ShowTooltipNow();

	/** True if a brief post-drag window should suppress tooltip. */
	bool IsDragSuppressed() const;

protected:
	// ---------- Config (Design-time) ----------
	/** Delay before showing tooltip when the pointer settles. */
	UPROPERTY(EditAnywhere, Category="Inventory|Tooltip")
	float HoverDelaySeconds = 0.45f;

	/** Small grace hide delay to avoid flicker on tiny exits/re-entries. */
	UPROPERTY(EditAnywhere, Category="Inventory|Tooltip")
	float HideGraceSeconds = 0.10f;


	/** Suppress tooltip for a short window after a drag begins. */
	UPROPERTY(EditAnywhere, Category="Inventory|Tooltip")
	float DragSuppressWindow = 0.15f;

private:
	// ---------- Runtime State (Transient) ----------
	/** Handle to any pending async icon stream. */
	// Possibly deprecated in favor of CommonLazyImage's own state?
	TSharedPtr<FStreamableHandle> StreamHandle;

	/** True while we are currently streaming an icon. */
	// Possibly deprecated in favor of CommonLazyImage's own state?
	bool bIsCurrentlyLoading = false;

	/** Last known screen position for tooltip placement. */
	FVector2D LastScreenPos = FVector2D::ZeroVector;

	/** Hover state. */
	bool bHoverArmed = false;

	/** When hover began (for suppression / timing). */
	double HoverStartTime = 0.0;


	/** Whether the tooltip is currently visible. */
	bool bTooltipVisible = false;


	/** Timer to show tooltip after delay. */
	FTimerHandle HoverTimerHandle;
	/** Timer to hide tooltip with grace period. */
	FTimerHandle HideTimerHandle;

public:
	// ---------- Slot Events (lightweight delegates for outer UI) ----------
	FGridSlotItemEvent ItemClicked;
	FGridSlotItemEvent ItemReleased;
	FGridSlotItemEvent ItemHovered;
	FGridSlotItemEvent ItemUnhovered;
};
