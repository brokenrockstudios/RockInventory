// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "UI/RockInventory_HoverItem.h"

#include "RockInventoryUILogging.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Inventory/RockInventory.h"
#include "Item/RockItemDefinition.h"
#include "UI/Composite/RockInventory_Leaf_Image.h"


void URockInventory_HoverItem::UpdateStackCount(const int32 Count)
{
	StackCount = Count;
	if (StackCount > 1)
	{
		Text_StackCount->SetText(FText::AsNumber(StackCount));
		Text_StackCount->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		// Don't show stack count if it's 1 or 0
		Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
	}
}

int32 URockInventory_HoverItem::GetStackCount() const
{
	return StackCount;
}

void URockInventory_HoverItem::SetItemSource(URockInventory* Inventory, const FRockInventorySlotHandle& SlotHandle)
{
	OwningInventory = Inventory;
	ItemSlotSourceHandle = SlotHandle;
	if (Inventory)
	{
		SetItemStack(Inventory->GetItemBySlotHandle(SlotHandle));
	}
}

void URockInventory_HoverItem::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void URockInventory_HoverItem::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void URockInventory_HoverItem::NativeConstruct()
{
	Super::NativeConstruct();
}

void URockInventory_HoverItem::NativeDestruct()
{
	Super::NativeDestruct();
}

void URockInventory_HoverItem::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Lerp to new size?
	
}

void URockInventory_HoverItem::SetTargetSize(int32 InTileSize, ERockItemSizePolicy InSizePolicy)
{
	FIntPoint GridSize = FIntPoint(1, 1);
	if (InSizePolicy == ERockItemSizePolicy::RespectSize && CopyItemStack.IsValid() && CopyItemStack.GetDefinition())
	{
		GridSize = CopyItemStack.GetDefinition()->GridSize;
	}

	SizeBox->SetWidthOverride(GridSize.X * InTileSize);
	SizeBox->SetHeightOverride(GridSize.Y * InTileSize);
}

void URockInventory_HoverItem::SetItemStack(const FRockItemStack& ItemStack)
{
	CopyItemStack = ItemStack;
	// Update visual state
	const URockItemDefinition* itemDefinition = ItemStack.GetDefinition();

	if (ItemStack.IsValid() && itemDefinition)
	{
		UpdateStackCount(ItemStack.GetStackCount());
		IconImage->SetImage(itemDefinition->IconData.Icon);
	}
	else
	{
		UE_LOG(LogRockInventoryUI, Error, TEXT("HoverItem::SetItemStack: Invalid ItemStack or ItemDefinition"));
		// TODO: Have better 'default fallback' image. Perhaps 'default frog or question mark'?
		IconImage->SetImage(nullptr);
		UpdateStackCount(0);
	}
}
