// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Item/RockItemStack.h"
#include "UI/Composite/RockInventory_Composite.h"
#include "UI/ItemDescription/RockInventoryDescriptor.h"
#include "RockInventory_ItemDescription.generated.h"

class UBorder;
class URockInventory_Leaf_Image;
class URockInventory_Leaf_Text;
class UTextBlock;
// UInterface for ItemDescription widgets.
UINTERFACE(BlueprintType)
class URockInventory_ItemDescriptionInterface : public UInterface
{
	GENERATED_BODY()
};

class IRockInventory_ItemDescriptionInterface
{
	GENERATED_BODY()

public:
	virtual void SetFromItem(const FRockItemStack& item) = 0;
};

// Should this be in the game instead of here?
UCLASS()
class ROCKINVENTORYUI_API URockInventory_ItemDescription : public URockInventory_Composite, public IRockInventory_ItemDescriptionInterface
{
	GENERATED_BODY()

public:
	// Should ItemDescription just be a UUserWidget that has a URockInventory_Composite in it, and is exposed via an interface instead?
	virtual void SetFromItem(const FRockItemStack& Item) override;

	// Give derived classes a chance to apply fields directly
	virtual void ApplyTypedFields(const FRockItemDescriptor& ItemDesc);
	// Blueprint event to apply the descriptor to the widget
	UFUNCTION(BlueprintImplementableEvent)
	void K2_ApplyDescriptor(const FRockItemDescriptor& Item);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class USizeBox> SizeBox;
};



UCLASS()
class ROCKINVENTORYUI_API URockInventory_ItemDescriptionDefault : public URockInventory_ItemDescription
{
	GENERATED_BODY()

public:
	// Should ItemDescription just be a UUserWidget that has a URockInventory_Composite in it, and is exposed via an interface instead?
	virtual void ApplyTypedFields(const FRockItemDescriptor& ItemDesc) override;

private:
	// Optional for 
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URockInventory_Leaf_Text> DisplayNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URockInventory_Leaf_Text> StackCountText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URockInventory_Leaf_Image> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URockInventory_Leaf_Text> DescriptionText;

	

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RarityBorder;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URockInventory_Leaf_Text> RarityText;
	
	// UPROPERTY(meta = (BindWidgetOptional))
	// TObjectPtr<UTextBlock> ItemTagsText;
	// UPROPERTY(meta = (BindWidgetOptional))
	// TObjectPtr<UTextBlock> FloatStatsText;
	// UPROPERTY(meta = (BindWidgetOptional))
	// TObjectPtr<UTextBlock> NotesText;
};
