// Copyright Broken Rock Studios LLC. All Rights Reserved.


#include "Utils/RockInventoryUIStaticsLibrary.h"

#include "RockInventoryUIManagerComponent.h"
#include "UI/Core/RockInventoryTooltipSubsystem.h"
#include "UI/Spatial/RockInventory_InventoryBase.h"

URockInventoryUIManagerComponent* URockInventoryUIStaticsLibrary::GetInventoryUIComponent(const APlayerController* PlayerController)
{
	if (!IsValid(PlayerController)) return nullptr;
	URockInventoryUIManagerComponent* InventoryComponent = PlayerController->FindComponentByClass<URockInventoryUIManagerComponent>();
	return InventoryComponent;
}

void URockInventoryUIStaticsLibrary::ItemHovered(APlayerController* PC, const FRockItemStack& Item)
{
	URockInventoryUIManagerComponent* IC = GetInventoryUIComponent(PC);
	if (!IsValid(IC)) { return; }

	URockInventory_InventoryBase* InventoryBase = IC->GetInventoryMenu();
	if (!IsValid(InventoryBase)) { return; }

	InventoryBase->OnItemHovered(Item);
}

void URockInventoryUIStaticsLibrary::ShowItemTooltip(UUserWidget* SourceWidget, const FRockItemStack& Item, const FVector2D& ScreenPos)
{
	if (!IsValid(SourceWidget))
	{
		return;
	}
	if (URockInventoryTooltipSubsystem* Sub = URockInventoryTooltipSubsystem::GetTooltipSubsystem(SourceWidget->GetWorld()))
	{
		Sub->ShowItemTooltip(SourceWidget, Item, ScreenPos);
	}
}


void URockInventoryUIStaticsLibrary::ItemUnhovered(APlayerController* PC)
{
	if (!IsValid(PC))
	{
		return;
	}
	if (URockInventoryTooltipSubsystem* Sub = URockInventoryTooltipSubsystem::GetTooltipSubsystem(PC->GetWorld()))
	{
		Sub->Hide();
	}
}

void URockInventoryUIStaticsLibrary::UpdateTooltipPosition(APlayerController* PC, const FVector2D& ScreenPos)
{
	if (!IsValid(PC))
	{
		return;
	}
	if (URockInventoryTooltipSubsystem* Sub = URockInventoryTooltipSubsystem::GetTooltipSubsystem(PC->GetWorld()))
	{
		Sub->UpdatePosition(ScreenPos);
	}
}


URockInventory_HoverItem* URockInventoryUIStaticsLibrary::GetHoverItem(APlayerController* PC)
{
	URockInventoryUIManagerComponent* IC = GetInventoryUIComponent(PC);
	if (!IsValid(IC)) { return nullptr; }

	URockInventory_InventoryBase* InventoryBase = IC->GetInventoryMenu();
	if (!IsValid(InventoryBase)) { return nullptr; }

	return InventoryBase->GetHoverItem();
}
