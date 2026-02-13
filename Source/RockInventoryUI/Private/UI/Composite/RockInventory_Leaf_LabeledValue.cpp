// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "UI/Composite/RockInventory_Leaf_LabeledValue.h"

#include "Components/TextBlock.h"

void URockInventory_Leaf_LabeledValue::SetText_Label(const FText& Text, bool bCollapse) const
{
	if (bCollapse)
	{
		Text_Label->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Text_Label->SetText(Text);
}

void URockInventory_Leaf_LabeledValue::SetText_Value(const FText& Text, bool bCollapse) const
{
	if (bCollapse)
	{
		Text_Value->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	Text_Value->SetText(Text);
}

void URockInventory_Leaf_LabeledValue::NativePreConstruct()
{
	Super::NativePreConstruct();

	FSlateFontInfo FontInfo_Label = Text_Label->GetFont();
	FontInfo_Label.Size = FontSize_Label;
	
	Text_Label->SetFont(FontInfo_Label);

	FSlateFontInfo FontInfo_Value = Text_Value->GetFont();
	FontInfo_Value.Size = FontSize_Value;
	
	Text_Value->SetFont(FontInfo_Value);
}
