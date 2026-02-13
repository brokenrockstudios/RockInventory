// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/RockItemStack.h"
#include "RockInventory_InventoryBase.generated.h"


/**
 * Equivalent to UInv_InventoryBase
 *
 * Base level class for the 'top level' inventory widget
 * This is the base class for the inventory widget that will contain all other widgets
 * We probably want to later change this to an interface or handled some other way
 * Since later on 'hover' tooltips might be more abstracted than 'inventory' specific
 */

class URockInventory_HoverItem;

UCLASS()
class ROCKINVENTORYUI_API URockInventory_InventoryBase : public UUserWidget
{
	GENERATED_BODY()
public:
	
	virtual void OnItemHovered(const FRockItemStack& Item) {} // UInv_InventoryItem* Item
	virtual void OnItemUnhovered() {}
	virtual bool HasHoverItem() const { return false; }
	virtual URockInventory_HoverItem* GetHoverItem() const { return nullptr; }
	virtual float GetTileSize() const { return 0.f; }

};
