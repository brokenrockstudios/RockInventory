// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "UI/RockInventory_Slot_BackgroundBase.h"

#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Inventory/RockInventory.h"
#include "UI/Shared/RockGridSlotEventData.h"

URockInventory_Slot_BackgroundBase::URockInventory_Slot_BackgroundBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set default values for properties
	SlotHandle = FRockInventorySlotHandle();
	Inventory = nullptr;
}

void URockInventory_Slot_BackgroundBase::NativeDestruct()
{
	Super::NativeDestruct();
	// Clean up any resources or references here
	Inventory = nullptr;
	SlotHandle = FRockInventorySlotHandle();
	GridSlotClicked.Clear();
	GridSlotReleased.Clear();
	GridSlotHovered.Clear();
	GridSlotUnhovered.Clear();
}

FReply URockInventory_Slot_BackgroundBase::NativeOnMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	GridSlotClicked.Broadcast(CreateEventData(InGeometry, InMouseEvent));
	return FReply::Handled();
}

FReply URockInventory_Slot_BackgroundBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	GridSlotReleased.Broadcast(CreateEventData(InGeometry, InMouseEvent));
	return FReply::Handled();
}


FReply URockInventory_Slot_BackgroundBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	GridSlotMouseMoved.Broadcast(CreateEventData(InGeometry, InMouseEvent));
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void URockInventory_Slot_BackgroundBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	GridSlotHovered.Broadcast(CreateEventData(InGeometry, InMouseEvent));
}

void URockInventory_Slot_BackgroundBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	GridSlotUnhovered.Broadcast(CreateEventData(FGeometry(), InMouseEvent));
}


void URockInventory_Slot_BackgroundBase::SetOccupiedTexture()
{
	Image_GridSlot->SetBrush(Brush_Occupied);
}

void URockInventory_Slot_BackgroundBase::SetUnoccupiedTexture()
{
	Image_GridSlot->SetBrush(Brush_Unoccupied);
}

void URockInventory_Slot_BackgroundBase::SetSelectedTexture()
{
	Image_GridSlot->SetBrush(Brush_Selected);
}

void URockInventory_Slot_BackgroundBase::SetGrayedOutTexture()
{
	Image_GridSlot->SetBrush(Brush_GrayedOut);
}

void URockInventory_Slot_BackgroundBase::SetSlot(URockInventory* InInventory, FRockInventorySlotHandle InSlotHandle)
{
	Inventory = InInventory;
	SlotHandle = InSlotHandle;
	AnchorHandle = InSlotHandle;
}

void URockInventory_Slot_BackgroundBase::SetSize(float InTileSize)
{
	if (SizeBox)
	{
		SizeBox->SetHeightOverride(InTileSize);
		SizeBox->SetWidthOverride(InTileSize);
	}
}

FRockInventorySlotHandle URockInventory_Slot_BackgroundBase::GetSlotHandle() const
{
	return SlotHandle;
}

void URockInventory_Slot_BackgroundBase::SetAnchorItemSlotHandle(const FRockInventorySlotHandle& InHandle)
{
	AnchorHandle = InHandle;
}

FRockInventorySlotHandle URockInventory_Slot_BackgroundBase::GetAnchorItemSlotHandle() const
{
	return AnchorHandle;
}

void URockInventory_Slot_BackgroundBase::ResetAnchorItemSlotHandle()
{
	AnchorHandle = SlotHandle;
}

FRockGridSlotEventData URockInventory_Slot_BackgroundBase::CreateEventData(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	ERockInventory_TileQuadrant Quadrant = ERockInventory_TileQuadrant::None;
	static const FGeometry DefaultGeometry;
	if (InGeometry != DefaultGeometry)
	{
		Quadrant = CalculateTileQuadrant(InGeometry, InMouseEvent);
	}
	const FRockGridSlotEventData EventData(this, InGeometry, InMouseEvent, Inventory, SlotHandle, AnchorHandle, Quadrant);
	return EventData;
}


ERockInventory_TileQuadrant URockInventory_Slot_BackgroundBase::CalculateTileQuadrant(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) const
{
	const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D Size = InGeometry.GetLocalSize();

	const bool bLeft = LocalPos.X <= Size.X * 0.5f;
	const bool bTop = LocalPos.Y <= Size.Y * 0.5f;

	if (bTop && bLeft)
	{
		return ERockInventory_TileQuadrant::TopLeft;
	}
	else if (bTop)
	{
		return ERockInventory_TileQuadrant::TopRight;
	}
	else if (bLeft)
	{
		return ERockInventory_TileQuadrant::BottomLeft;
	}
	else
	{
		return ERockInventory_TileQuadrant::BottomRight;
	}
}
