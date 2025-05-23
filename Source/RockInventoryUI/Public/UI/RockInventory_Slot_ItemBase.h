// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/RockSlotHandle.h"
#include "Item/RockItemDefinition.h"
#include "RockInventory_Slot_ItemBase.generated.h"

struct FRockItemUIData;
struct FStreamableHandle;
class UImage;
class UTextBlock;
class URockInventory;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_Slot_ItemBase : public UUserWidget
{
	GENERATED_BODY()

public:
	// The main widget for an item that will exist in the canvas panel of a Container
	// This needs to adjust it's size based upon the size of the item
	// We might not need this, but it is here for now. This is the inventory that this slot belongs to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	TObjectPtr<URockInventory> Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FRockInventorySlotHandle SlotHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
	FRockItemStackHandle ItemHandle;
	
	// Bind Image Widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	// Used when this 'slot' or 'item' is locked. Such as if the item is in use, or currently being 'dragged' by a player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<UImage> ItemLockIcon;
	
	// Text widget for displaying item count
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta = (BindWidget))
	TObjectPtr<UImage> LoadingIndicator;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	void SetupBindings();
	
	// Updates the ItemCount text based on current stack size
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UpdateItemCount();

	// Callback for when the inventory changes
	UFUNCTION()
	void OnInventorySlotChanged(URockInventory* ChangedInventory, const FRockInventorySlotHandle& ChangedSlotHandle);
	UFUNCTION()
	void OnInventoryItemChanged(URockInventory* ChangedInventory, const FRockItemStackHandle& ChangedItemHandle);


	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter( const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation ) override;
	virtual void NativeOnDragLeave( const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation )override;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetIconData(const FRockItemUIData& InIconData);

	void SetIsLoading(bool bIsLoading);
	void OnIconLoaded();

	TSharedPtr<FStreamableHandle> StreamHandle;
	bool bIsCurrentlyLoading = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slot")
	TObjectPtr<UTexture2D> FallbackIcon;
	UPROPERTY()
	FRockItemUIData IconData;
};
