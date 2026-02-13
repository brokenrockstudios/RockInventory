// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "UI/Composite/RockInventory_Leaf_Text.h"

#include "Components/TextBlock.h"


void URockInventory_Leaf_Text::SetText(const FText& Text) const
{
	Text_LeafText->SetText(Text);
}

void URockInventory_Leaf_Text::SetColorAndOpacity(const FSlateColor& Color) const
{
	Text_LeafText->SetColorAndOpacity(Color);
	
}

void URockInventory_Leaf_Text::NativePreConstruct()
{
	Super::NativePreConstruct();

	FSlateFontInfo FontInfo = Text_LeafText->GetFont();
	FontInfo.Size = FontSize;
	
	Text_LeafText->SetFont(FontInfo);
}
