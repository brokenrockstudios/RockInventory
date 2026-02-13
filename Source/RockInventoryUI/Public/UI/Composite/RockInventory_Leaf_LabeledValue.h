// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_Leaf.h"
#include "RockInventory_Leaf_LabeledValue.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_Leaf_LabeledValue : public URockInventory_Leaf
{
	GENERATED_BODY()
public:

	void SetText_Label(const FText& Text, bool bCollapse) const;
	void SetText_Value(const FText& Text, bool bCollapse) const;
	virtual void NativePreConstruct() override;
	
private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Label;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Value;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 FontSize_Label = 12;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 FontSize_Value = 16;
};
