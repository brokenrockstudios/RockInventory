// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockInventory_Leaf.h"
#include "RockInventory_Leaf_Image.generated.h"

class UCommonLazyImage;
class USizeBox;
class UImage;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_Leaf_Image : public URockInventory_Leaf
{
	GENERATED_BODY()
public:
	URockInventory_Leaf_Image(const FObjectInitializer& ObjectInitializer);
	void SetImage(UTexture2D* Texture) const;
	void SetImage(TSoftObjectPtr<UTexture2D> Texture) const;
	void SetBoxSize(const FVector2D& Size) const;
	void SetImageSize(const FVector2D& Size) const;
	FVector2D GetImageSize() const;
	
private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonLazyImage> LazyImage_Icon;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Icon;
};
