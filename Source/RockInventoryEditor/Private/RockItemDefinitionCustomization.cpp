// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "RockItemDefinitionCustomization.h"


TSharedRef<IDetailCustomization> FRockItemDefinitionCustomization::MakeInstance()
{
	return MakeShareable(new FRockItemDefinitionCustomization());
}

void FRockItemDefinitionCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
}
