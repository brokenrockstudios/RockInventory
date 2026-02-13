// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_Leaf.h"
#include "RockInventory_Leaf_Text.generated.h"

class UTextBlock;

/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_Leaf_Text : public URockInventory_Leaf
{
	GENERATED_BODY()
public:
	void SetText(const FText& Text) const;
	void SetColorAndOpacity(const FSlateColor& Color) const;
	virtual void NativePreConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_LeafText;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	int32 FontSize = 12;
};

