// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventoryWidgetUtilLibrary.h"
#include "Item/RockItemStack.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RockInventoryUIStaticsLibrary.generated.h"

class URockInventoryUIManagerComponent;
class URockInventory_HoverItem;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventoryUIStaticsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static URockInventoryUIManagerComponent* GetInventoryUIComponent(const APlayerController* PlayerController);

	//UFUNCTION(BlueprintCallable, Category = "Inventory")
	//static EInv_ItemCategory GetItemCategoryFromItemComp(UInv_ItemComponent* ItemComp);

	template<typename T, typename FuncT>
	static void ForEach2D(TArray<T>& Array, int32 Index, const FIntPoint& Range2D, int32 GridColumns, const FuncT& Function);

	// Pass in the Inventory+ItemHandle? and/or slot? Need to get itemdefinition and not sure what else? 
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static void ItemHovered(APlayerController* PC, const FRockItemStack& Item);// UInv_InventoryItem* Item
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static void ShowItemTooltip(UUserWidget* SourceWidget, const FRockItemStack& Item, const FVector2D& ScreenPos);

	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static void ItemUnhovered(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static void UpdateTooltipPosition(APlayerController* PC, const FVector2D& ScreenPos);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static URockInventory_HoverItem* GetHoverItem(APlayerController* PC);
};


template<typename T, typename FuncT>
void URockInventoryUIStaticsLibrary::ForEach2D(TArray<T>& Array, int32 Index, const FIntPoint& Range2D, int32 GridColumns, const FuncT& Function)
{
	for (int32 j = 0; j < Range2D.Y; ++j)
	{
		for (int32 i = 0; i < Range2D.X; ++i)
		{
			const FIntPoint Coordinates = URockInventoryWidgetUtilLibrary::GetPositionFromIndex(Index, GridColumns) + FIntPoint(i, j);
			const int32 TileIndex = URockInventoryWidgetUtilLibrary::GetIndexFromPosition(Coordinates, GridColumns);
			if (Array.IsValidIndex(TileIndex))
			{
				Function(Array[TileIndex]);
			}
		}
	}
}