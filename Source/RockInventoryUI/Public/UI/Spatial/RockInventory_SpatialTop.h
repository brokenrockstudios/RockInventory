// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_InventoryBase.h"
#include "Item/RockItemFragment.h"
#include "UI/Composite/RockInventory_CompositeBase.h"
#include "RockInventory_SpatialTop.generated.h"

/**
 * 
 */

class URockInventory_ContainerBase;
class URockInventory_ItemDescription;
class UCanvasPanel;

// This is the 'main master' widget of all inventory UI widgets.
// A lot of this code will likely be moved to a game specific solution, with possibly just an interface provided here instead
// But for sake of getting something working, this is fine for now.


UCLASS()
class ROCKINVENTORYUI_API URockInventory_SpatialTop : public URockInventory_InventoryBase
{
	GENERATED_BODY()

	virtual void OnItemHovered(const FRockItemStack& ItemStack) override;
	virtual void OnItemUnhovered() override;
	virtual bool HasHoverItem() const override;
	virtual URockInventory_HoverItem* GetHoverItem() const override;
	virtual float GetTileSize() const override;
	void AssimilateInventoryFragments(const FRockItemStack& ItemStack, URockInventory_ItemDescription* DescriptionComposite);
	void Assimilate(URockInventory_CompositeBase* widget, const FRockItemFragmentInstance& fragment);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	// Should this be defaulted somewhere else? and/or overrideable per item?
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<URockInventory_ItemDescription> ItemDescriptionClass;
	
	// Current 'hovered item description tooltip widget'
	UPROPERTY()
	TObjectPtr<URockInventory_ItemDescription> ItemDescription;
	URockInventory_ItemDescription* GetItemDescription();

	FTimerHandle DescriptionTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float DescriptionTimerDelay = 0.5f;
	
	TWeakObjectPtr<URockInventory_ContainerBase> ActiveGrid;
};
