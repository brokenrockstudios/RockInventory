// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_ContainerBase.h"
#include "Blueprint/UserWidget.h"
#include "Item/RockItemStack.h"
#include "RockInventory_HoverItem.generated.h"

class UCommonLazyImage;
class URockInventory;
class URockInventory_Leaf_Image;
class USizeBox;
class UTextBlock;
struct FRockItemStackHandle;
struct FRockItemStack;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_HoverItem : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateStackCount(const int32 Count);

	// bool IsStackable() const;
	int32 GetStackCount() const;

	// Should this be a copy of the item or a reference to the underlying item/inventory.
	// We'd need a reference to the inventory to get the item stack if we need to modify it?

	void SetItemSource(URockInventory* Inventory, const FRockInventorySlotHandle& ItemHandle);
	void SetTargetSize(int32 InTileSize, ERockItemSizePolicy InSizePolicy);

	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URockInventory_Leaf_Image> IconImage;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox;


	// Should this be a SlotHandle instead?
	int32 PreviousGridIndex = INDEX_NONE;

	int32 StackCount = 0;
	bool bIsStackable = false;

	// Consider adding back for a Cached version for lerping to new size?
	//UPROPERTY()
	//ERockItemSizePolicy SizePolicy = ERockItemSizePolicy::RespectSize;
	//FIntPoint GridSize = FIntPoint(1, 1);
	// TODO: Do I want the default to be 40 or 48 instead of the 64?
	//float TileSize = 64.f;
	

	void SetItemStack(const FRockItemStack& ItemStack);
	FRockItemStack CopyItemStack; // This is a copy of the item stack, not a reference to the underlying item stack.
	UPROPERTY()
	TObjectPtr<URockInventory> OwningInventory = nullptr;
	FRockInventorySlotHandle ItemSlotSourceHandle;
};
