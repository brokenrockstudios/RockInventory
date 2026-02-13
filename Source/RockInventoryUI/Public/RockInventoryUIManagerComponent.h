// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RockInventoryUIManagerComponent.generated.h"


class URockItemDragDropOperation;
class URockInventory_HoverItem;
class URockInventory_InventoryBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ROCKINVENTORYUI_API URockInventoryUIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	URockInventory_InventoryBase* GetInventoryMenu() const { return InventoryMenu; }
	URockInventory_HoverItem* GetHoverItem() const;
	
	// UInv_InventoryBase
	UPROPERTY()
	TObjectPtr<URockInventory_InventoryBase> InventoryMenu;

	//UPROPERTY()
	//TObjectPtr<URockItemDragDropOperation> CurrentDragDropOperation = nullptr;
};
