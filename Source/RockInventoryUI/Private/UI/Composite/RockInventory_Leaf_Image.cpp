// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "UI/Composite/RockInventory_Leaf_Image.h"

#include "CommonLazyImage.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

URockInventory_Leaf_Image::URockInventory_Leaf_Image(const FObjectInitializer& ObjectInitializer)
{
	// Default to a 32x32 icon size
	// SetBoxSize({32.0f, 32.0f});
}

void URockInventory_Leaf_Image::SetImage(UTexture2D* Texture) const
{
	LazyImage_Icon->SetBrushFromTexture(Texture);
}

void URockInventory_Leaf_Image::SetImage(TSoftObjectPtr<UTexture2D> Texture) const
{
	LazyImage_Icon->SetBrushFromSoftTexture(Texture);
}

void URockInventory_Leaf_Image::SetBoxSize(const FVector2D& Size) const
{
	SizeBox_Icon->SetWidthOverride(Size.X);
	SizeBox_Icon->SetHeightOverride(Size.Y);
}

void URockInventory_Leaf_Image::SetImageSize(const FVector2D& Size) const
{
	LazyImage_Icon->SetDesiredSizeOverride(Size);
}

FVector2D URockInventory_Leaf_Image::GetImageSize() const
{
	return LazyImage_Icon->GetDesiredSize();
}
