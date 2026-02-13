// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "UI/ItemDescription/RockInventory_ItemDescription.h"

#include "RockInventoryDescriptor.h"
#include "RockInventoryUILogging.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"
#include "Rarity/RockRarityData.h"
#include "Rarity/RockRarityRegistry.h"
#include "UI/RockInventory_ContainerBase.h"
#include "UI/Composite/RockInventory_Leaf_Image.h"
#include "UI/Composite/RockInventory_Leaf_Text.h"

void URockInventory_ItemDescription::SetFromItem(const FRockItemStack& Item)
{
	// 1) Build descriptor
	FRockItemDescriptor ItemDescriptor;
	if (!FRockItemDescriptorBuilder::Get().Build(Item, ItemDescriptor)) // runs your fragment extractors
	{
		// Failed to build descriptor, hide everything
		SetVisibility(ESlateVisibility::Collapsed);
		UE_LOG(LogRockInventoryUI, Warning, TEXT("Failed to build item descriptor for item in ItemDescription widget."));
		return;
	}

	// 2) Push core typed fast, fast path. No BP
	ApplyTypedFields(ItemDescriptor);

	// 3) Give BP a chance to do custom stuff
	K2_ApplyDescriptor(ItemDescriptor);

	// 4) Extras

	// for (auto& Fragment : InventoryItemFragments)
	// {
	//  	DescriptionComposite->ApplyFunction([Fragment](URockInventory_CompositeBase* Widget)
	//  	{
	//  		Assimilate(Widget, Fragment);
	//  	});
	// }
}


void URockInventory_ItemDescription::ApplyTypedFields(const FRockItemDescriptor& ItemDesc)
{
	// You could override this class to do other things here
}

void URockInventory_ItemDescriptionDefault::ApplyTypedFields(const FRockItemDescriptor& ItemDesc)
{
	Super::ApplyTypedFields(ItemDesc);
	Expand();
	if (DisplayNameText)
	{
		DisplayNameText->SetText(ItemDesc.DisplayName);
		DisplayNameText->Expand();
	}
	if (IconImage)
	{
		IconImage->SetImage(ItemDesc.Icon.Icon.Get());
		IconImage->SetBoxSize(ItemDesc.IconSize);
		IconImage->SetImageSize(ItemDesc.IconSize);
		IconImage->Expand();
	}
	if (StackCountText)
	{
		const int32 StackCount = ItemDesc.StackCount;
		if (StackCount > 1)
		{
			StackCountText->SetText(FText::AsNumber(StackCount));
			StackCountText->Expand();
		}
		else
		{
			StackCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(ItemDesc.Description);
		DescriptionText->Expand();
	}
	if (RarityBorder)
	{
		RarityBorder->SetBrushColor(ItemDesc.RarityColor);
	}
	if (RarityText)
	{
		FLinearColor RarityColor = FLinearColor::White;
		FText RarityName = NSLOCTEXT("RockInventory", "UnknownRarity", "Unknown");

		const URockRarityRegistry* RarityRegistry = UGameplayStatics::GetGameInstance(GetWorld())->GetSubsystem<URockRarityRegistry>();
		ensureMsgf(RarityRegistry, TEXT("RarityRegistry not found in GameInstance!"));
		const URockRarityData* RarityData = RarityRegistry ? RarityRegistry->GetOrDefault(ItemDesc.Rarity) : nullptr;
		if (RarityData)
		{
			RarityColor = RarityData->Color;
			RarityName = RarityData->DisplayName;
		}

		RarityText->SetColorAndOpacity(FSlateColor(RarityColor));
		RarityText->SetText(RarityName);
		RarityText->Expand();
		
	}
}
