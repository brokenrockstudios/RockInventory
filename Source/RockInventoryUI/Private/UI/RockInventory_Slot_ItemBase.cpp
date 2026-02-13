// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "UI/RockInventory_Slot_ItemBase.h"

#include "RockInventoryUILogging.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Core/RockInventoryTooltipSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemStack.h"
#include "UI/Shared/RockGridItemEventData.h"
#include "Utils/RockInventoryUIStaticsLibrary.h"

void URockInventory_Slot_ItemBase::NativeConstruct()
{
	Super::NativeConstruct();

	bIsCurrentlyLoading = false;
	// Hide count by default if we haven't yet set it
	if (ItemCount)
	{
		ItemCount->SetVisibility(ESlateVisibility::Hidden);
	}

	// We don't want to show the loading indicator by default
	if (ItemLockIcon)
	{
		ItemLockIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URockInventory_Slot_ItemBase::NativeDestruct()
{
	Super::NativeDestruct();
	DisarmHover(false);

	// Clean up any pending loads when widget is destroyed
	if (StreamHandle.IsValid() && !StreamHandle->HasLoadCompleted())
	{
		StreamHandle->CancelHandle();
		StreamHandle.Reset();
	}
	Cleanup();

}

void URockInventory_Slot_ItemBase::Cleanup()
{
	// Unregister from inventory change events
	if (Inventory)
	{
		//Inventory->OnSlotChanged.RemoveDynamic(this, &ThisClass::OnInventorySlotChanged);
		//Inventory->OnItemChanged.RemoveDynamic(this, &ThisClass::OnInventoryItemChanged);
	}

	Inventory = nullptr;

	ItemClicked.Clear();
	ItemReleased.Clear();
	ItemHovered.Clear();
	ItemUnhovered.Clear();

	FallbackIcon = nullptr;
}

void URockInventory_Slot_ItemBase::UpdateItemCount()
{
	checkfSlow(ItemCount, TEXT("URockInventory_Slot_ItemBase::UpdateItemCount: ItemCount is null!"));

	if (!Inventory || !ItemHandle.IsValid())
	{
		return;
	}

	// Get the item at this slot
	const FRockItemStack& ItemStack = Inventory->GetItemBySlotHandle(SlotHandle);
	if (ItemStack.IsValid())
	{
		// Update the count text if stack size is greater than 1
		if (ItemStack.GetStackCount() > 1)
		{
			ItemCount->SetText(FText::AsNumber(ItemStack.GetStackCount()));
			ItemCount->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// Why hidden vs collapsed?
			// Hide count for single items
			ItemCount->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		// No item in this slot
		ItemCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FReply URockInventory_Slot_ItemBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Switch to gamepad button handling? If we wanted to support gamepad hover/selection. Get EnhancedInput or something?
	// if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom) { return FReply::Handled(); }
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply URockInventory_Slot_ItemBase::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply URockInventory_Slot_ItemBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	constexpr ERockInventory_TileQuadrant Quadrant = ERockInventory_TileQuadrant::None;
	const FRockGridItemEventData EventData(this, InGeometry, InMouseEvent, Inventory, SlotHandle, Quadrant);
	ItemClicked.Broadcast(EventData);
	return FReply::Handled();
}

FReply URockInventory_Slot_ItemBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	constexpr ERockInventory_TileQuadrant Quadrant = ERockInventory_TileQuadrant::None;
	const FRockGridItemEventData EventData(this, InGeometry, InMouseEvent, Inventory, SlotHandle, Quadrant);
	ItemReleased.Broadcast(EventData);
	return FReply::Handled();
}

static FVector2D GetViewportPos(UWorld* World, const FVector2D& AbsolutePos)
{
	FVector2D PixelPos, ViewportPos;
	// Convert ABSOLUTE → VIEWPORT (returns both)
	USlateBlueprintLibrary::AbsoluteToViewport(World, AbsolutePos, PixelPos, ViewportPos);
	return ViewportPos;
}

void URockInventory_Slot_ItemBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FRockGridItemEventData EventData(this, InGeometry, InMouseEvent, Inventory, SlotHandle, ERockInventory_TileQuadrant::None);
	ItemHovered.Broadcast(EventData);


	const FRockItemStack& ItemStack = Inventory ? Inventory->GetItemBySlotHandle(SlotHandle) : FRockItemStack();
	URockInventoryUIStaticsLibrary::ItemHovered(GetOwningPlayer(), ItemStack);

	// 1. Get mouse position in absolute screen space
	LastScreenPos = GetViewportPos(GetWorld(), InMouseEvent.GetScreenSpacePosition());
	ArmHover();
}

void URockInventory_Slot_ItemBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	const FRockGridItemEventData EventData(this, FGeometry(), InMouseEvent, Inventory, SlotHandle, ERockInventory_TileQuadrant::None);
	ItemUnhovered.Broadcast(EventData);
	URockInventoryUIStaticsLibrary::ItemUnhovered(GetOwningPlayer());

	DisarmHover(true);
}

static void GetCenterTopBottomViewport(const FGeometry& Geo, UWorld* World, FVector2D& OutTop, FVector2D& OutBottom)
{
	const FVector2D size = Geo.GetLocalSize();
	const FVector2D lTop = FVector2D(size.X * 0.5f, 0.f);
	const FVector2D lBot = FVector2D(size.X * 0.5f, size.Y);

	// Local → Absolute (desktop pixels)
	const FVector2D absTop = Geo.LocalToAbsolute(lTop);
	const FVector2D absBot = Geo.LocalToAbsolute(lBot);

	// Absolute → Viewport (DPI aware)
	FVector2D dummyPixels;
	USlateBlueprintLibrary::AbsoluteToViewport(World, absTop, dummyPixels, OutTop);
	USlateBlueprintLibrary::AbsoluteToViewport(World, absBot, dummyPixels, OutBottom);
}

FReply URockInventory_Slot_ItemBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	LastScreenPos = GetViewportPos(GetWorld(), InMouseEvent.GetScreenSpacePosition());

	FVector2D LastTopScreenPos;
	FVector2D LastBottomScreenPos;
	GetCenterTopBottomViewport(InGeometry, GetWorld(), LastTopScreenPos, LastBottomScreenPos);

	// Armed means' we are about to show the tooltip, but we aren't showing it yet. In the few milliseconds before it shows
	if (bHoverArmed == false && bTooltipVisible)
	{
		URockInventoryUIStaticsLibrary::UpdateTooltipPosition(GetOwningPlayer(), LastBottomScreenPos);
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void URockInventory_Slot_ItemBase::SetIconData(const FRockItemUIData& InIconData)
{
	IconData = InIconData;
	// TODO: Use the IconData to set hover/tooltip?

	SetIsLoading(true);

	// Handle null case
	if (IconData.Icon.IsNull())
	{
		// Set fallback texture
		if (FallbackIcon)
		{
			ItemIcon->SetBrushFromTexture(FallbackIcon);
		}
		else
		{
			ItemIcon->SetBrushResourceObject(nullptr);
		}
		SetIsLoading(false);
		return;
	}

	// If already loaded, use it immediately
	if (IconData.Icon.IsValid())
	{
		ItemIcon->SetBrushFromTexture(IconData.Icon.Get());
		SetIsLoading(false);
		return;
	}

	// Cancel any existing stream handle
	if (StreamHandle.IsValid() && !StreamHandle->HasLoadCompleted())
	{
		StreamHandle->CancelHandle();
	}

	// Start async load
	StreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		IconData.Icon.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &URockInventory_Slot_ItemBase::OnIconLoaded)
	);
}

void URockInventory_Slot_ItemBase::SetIsLoading(bool bIsLoading)
{
	bIsCurrentlyLoading = bIsLoading;

	// Update loading indicator visibility
	if (LoadingIndicator)
	{
		LoadingIndicator->SetVisibility(bIsLoading ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Optionally apply any visual changes to the item icon during loading
	if (ItemIcon)
	{
		// Could adjust opacity, visibility, etc.
		// For example:
		ItemIcon->SetRenderOpacity(bIsLoading ? 0.3f : 1.0f);
	}
}

void URockInventory_Slot_ItemBase::OnIconLoaded()
{
	// Only proceed if we're still valid
	if (!IsValidLowLevel())
	{
		return;
	}

	if (IconData.Icon.IsValid())
	{
		ItemIcon->SetBrushFromTexture(IconData.Icon.Get());
	}
	else
	{
		// Set fallback texture
		if (FallbackIcon)
		{
			ItemIcon->SetBrushFromTexture(FallbackIcon);
		}
		else
		{
			ItemIcon->SetBrushResourceObject(nullptr);
		}
	}

	SetIsLoading(false);
	StreamHandle.Reset();
}

void URockInventory_Slot_ItemBase::InitializeItem(URockInventory* InInventory, const FRockInventorySlotHandle& InSlotHandle, float tileSize)
{
	if (!ensure(InInventory))
	{
		return;
	}

	Inventory = InInventory;
	SlotHandle = InSlotHandle;
	parentContainersTileSize = tileSize;

	Update();
}

void URockInventory_Slot_ItemBase::Update()
{
	checkfSlow(Inventory, TEXT("URockInventory_Slot_ItemBase::Update: Inventory is null"));
	checkfSlow(OverallSize, TEXT("URockInventory_Slot_ItemBase::Update: Inventory is null"));

	// A lock, orientation, or underlying ItemSlot may have changed
	// Recheck everything
	if (!Inventory || !SlotHandle.IsValid())
	{
		return;
	}

	const FRockInventorySlotEntry slot = Inventory->GetSlotByHandle(SlotHandle);
	ItemHandle = slot.ItemHandle;

	const FRockItemStack LocalCopyOfItem = Inventory->GetItemByHandle(ItemHandle);

	if (!LocalCopyOfItem.IsValid() || !LocalCopyOfItem.GetDefinition())
	{
		UE_LOG(LogRockInventoryUI, Error, TEXT("URockInventory_Slot_Item Base::InitializeItem: Invalid ItemStack"));
		return;
	}
	// Get SectionInfo
	FRockInventorySectionInfo sectionInfo = Inventory->GetSectionInfoBySlotHandle(SlotHandle);
	bool bRespectSize = sectionInfo.GetSlotSizePolicy() == ERockItemSizePolicy::RespectSize;
	auto itemTileSize = bRespectSize ? LocalCopyOfItem.GetDefinition()->GridSize : FIntPoint(1, 1);

	const FIntPoint itemSize = itemTileSize * parentContainersTileSize;
	OverallSize->SetWidthOverride(itemSize.X);
	OverallSize->SetHeightOverride(itemSize.Y);

	SetIconData(LocalCopyOfItem.GetDefinition()->IconData);

	// UpdateBorder(); ? 
	// Update the item count on construction
	UpdateItemCount();
}

void URockInventory_Slot_ItemBase::ArmHover()
{
	GetWorld()->GetTimerManager().ClearTimer(HideTimerHandle);

	HoverStartTime = FPlatformTime::Seconds();
	bHoverArmed = true;

	// Start/Restart delayed show
	FTimerDelegate showToolTipDelegate;
	TWeakObjectPtr<URockInventory_Slot_ItemBase> Self = this;
	showToolTipDelegate.BindLambda([Self]()
	{
		if (!Self.IsValid())
		{
			return;
		}
		Self->ShowTooltipNow();
	});
	GetWorld()->GetTimerManager().SetTimer(HoverTimerHandle, showToolTipDelegate, HoverDelaySeconds, /*bLoop=*/false);
}

void URockInventory_Slot_ItemBase::DisarmHover(bool bGracefulHide)
{
	GetWorld()->GetTimerManager().ClearTimer(HoverTimerHandle);
	bHoverArmed = false;

	if (bTooltipVisible)
	{
		if (bGracefulHide && HideGraceSeconds > 0.f)
		{
			FTimerDelegate Del;
			TWeakObjectPtr<URockInventory_Slot_ItemBase> Self = this;
			Del.BindLambda([Self]()
			{
				if (!Self.IsValid()) return;
				if (URockInventoryTooltipSubsystem* Sub = URockInventoryTooltipSubsystem::GetTooltipSubsystem(Self->GetWorld()))
				{
					Sub->HideIfFrom(Self.Get());
				}
				Self->bTooltipVisible = false;
			});
			GetWorld()->GetTimerManager().SetTimer(HideTimerHandle, Del, HideGraceSeconds, /*bLoop=*/false);
		}
		else
		{
			if (URockInventoryTooltipSubsystem* Sub = URockInventoryTooltipSubsystem::GetTooltipSubsystem(GetWorld()))
			{
				Sub->HideIfFrom(this);
			}
			bTooltipVisible = false;
		}
	}
}

bool URockInventory_Slot_ItemBase::IsDragSuppressed() const
{
	const double Elapsed = FPlatformTime::Seconds() - HoverStartTime;
	return (Elapsed < DragSuppressWindow);
}

void URockInventory_Slot_ItemBase::ShowTooltipNow()
{
	if (!bHoverArmed)
	{
		return;
	}
	if (IsDragSuppressed())
	{
		// e.g., user started dragging immediately
		return;
	}

	const FRockItemStack& ItemStack = Inventory ? Inventory->GetItemBySlotHandle(SlotHandle) : FRockItemStack();

	if (!ItemStack.IsValid())
	{
		// No item, no tooltip
		return;
	}
	bTooltipVisible = true;
	bHoverArmed = false;
	URockInventoryUIStaticsLibrary::ShowItemTooltip(this, ItemStack, LastScreenPos);
}
