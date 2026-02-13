// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockInventoryTooltipSubsystem.h"

#include "RockInventoryUILogging.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "UI/RockInventoryUISettings.h"
#include "UI/ItemDescription/RockInventory_ItemDescription.h"

URockInventoryTooltipSubsystem* URockInventoryTooltipSubsystem::GetTooltipSubsystem(UWorld* World)
{
	if (!World)
	{
		ensureMsgf(false, TEXT("GetTooltipSubsystem: World is null"));
		return nullptr;
	}
	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		ensureMsgf(false, TEXT("GetTooltipSubsystem: GameInstance is null"));
		return nullptr;
	}

	return GI->GetSubsystem<URockInventoryTooltipSubsystem>();
}

void URockInventoryTooltipSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Lazy creation in EnsureLayer/EnsureContent

	const auto* Settings = GetDefault<URockInventoryUISettings>();
	if (!Settings)
	{
		ensureMsgf(false, TEXT("Tooltip settings object missing"));
		return;
	}

	if (Settings->TooltipLayerClass.IsNull())
	{
		ensureMsgf(false, TEXT("TooltipLayerClass not set in Project Settings."));
		return;
	}

	// Load synchronously once; cache the class
	TooltipLayerClass = Settings->TooltipLayerClass.LoadSynchronous();
	TooltipDefaultContentClass = Settings->TooltipDefaultContentClass.LoadSynchronous();
	ensureMsgf(TooltipLayerClass, TEXT("RockInventoryTooltipSubsystem: TooltipLayerClass failed to load!"));
}

void URockInventoryTooltipSubsystem::Deinitialize()
{
	Hide();
	TooltipLayer = nullptr;
	TooltipContent = nullptr;
	Super::Deinitialize();
}

UUserWidget* URockInventoryTooltipSubsystem::GetOrCreateLayer()
{
	if (TooltipLayer)
	{
		return TooltipLayer;
	}

	// You can either use a minimalist C++ layer widget or a simple empty BP widget with a CanvasPanel.
	// For brevity, create a plain UserWidget and add to viewport:
	//TooltipLayer = CreateWidget<UUserWidget>(GetWorld(), UUserWidget::StaticClass());
	TooltipLayer = CreateWidget<UUserWidget>(GetWorld(), TooltipLayerClass);
	if (!TooltipLayer)
	{
		UE_LOG(LogRockInventoryUI, Error, TEXT("RockInventoryTooltipSubsystem: Failed to create TooltipLayer."));
		return nullptr;
	}

	// TODO: reworking into a 'game specific overlay panel' instead of direct to viewport.
	// Since in practice this should be underneath main menu and game modals and such.
	TooltipLayer->AddToViewport(10000); // high Z
	SetLayerVisibility(false);
	return TooltipLayer;
}

UUserWidget* URockInventoryTooltipSubsystem::GetOrCreateContent()
{
	if (TooltipContent)
	{
		return TooltipContent;
	}

	// If your tooltip is a BP class, substitute its generated class here.
	TooltipContent = CreateWidget<URockInventory_ItemDescription>(GetWorld(), TooltipDefaultContentClass);
	if (!TooltipContent)
	{
		UE_LOG(LogRockInventoryUI, Error, TEXT("RockInventoryTooltipSubsystem: Failed to create TooltipContent."));

		return nullptr;
	}


	if (TooltipLayer)
	{
		if (UCanvasPanel* canvasPanel = Cast<UCanvasPanel>(TooltipLayer->GetRootWidget()))
		{
			UCanvasPanelSlot* CanvasChild = canvasPanel->AddChildToCanvas(TooltipContent);
			// Size to Content
			CanvasChild->SetAutoSize(true);
		}
		else if (UPanelWidget* panelWidget = Cast<UPanelWidget>(TooltipLayer->GetRootWidget()))
		{
			panelWidget->AddChild(TooltipContent);
		}
		else
		{
			UE_LOG(LogRockInventoryUI,
				Error,
				TEXT("RockInventoryTooltipSubsystem: TooltipLayer has no root panel; setting TooltipContent as root directly."));
		}
		TooltipContent->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return TooltipContent;
}

void URockInventoryTooltipSubsystem::SetLayerVisibility(bool bVisible)
{
	if (!TooltipLayer)
	{
		return;
	}
	TooltipLayer->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void URockInventoryTooltipSubsystem::ShowItemTooltip(UUserWidget* SourceWidget, const FRockItemStack& Item, const FVector2D& ScreenPos)
{
	if (!SourceWidget || !SourceWidget->IsValidLowLevel())
	{
		return;
	}

	GetOrCreateLayer();
	GetOrCreateContent();
	if (!TooltipLayer || !TooltipContent)
	{
		return;
	}

	CurrentSource = SourceWidget;
	if (IRockInventory_ItemDescriptionInterface* itemDescriptionWidget = Cast<IRockInventory_ItemDescriptionInterface>(TooltipContent))
	{
		itemDescriptionWidget->SetFromItem(Item); // implement on your widget
	}

	UpdatePosition(ScreenPos);
	SetLayerVisibility(true);
}

void URockInventoryTooltipSubsystem::UpdatePosition(const FVector2D& ScreenPos)
{
	if (!TooltipLayer || !TooltipContent || !TooltipLayer->IsInViewport())
	{
		return;
	}

	// Jitter suppression
	// if (FVector2D::Distance(ScreenPos, LastScreenPos) < MoveThreshold)
	// {
	// 	return;
	// }
	LastScreenPos = ScreenPos;

	if (UCanvasPanelSlot* CanvasToolTip = Cast<UCanvasPanelSlot>(TooltipContent->Slot))
	{
		//TooltipContent->Prepas
		auto SizeOther = TooltipContent->GetDesiredSize(); // force layout pre-pass

		auto Size = CanvasToolTip->GetSize(); // force layout pre-pass
		auto Offset = FVector2D(-0.5f * SizeOther.X, 0);
		//FVector2D(0.f, 12.f));
		CanvasToolTip->SetPosition(ScreenPos + Offset);

		// If for some reason we need to get into 'local coordinates' of the canvas
		//const FGeometry canvasGeo = TooltipCanvas->GetCachedGeometry();
		//const FVector2D localInCanvas = canvasGeo.AbsoluteToLocal(mouseAbs);
	}
}

void URockInventoryTooltipSubsystem::HideIfFrom(UUserWidget* SourceWidget)
{
	if (CurrentSource.Get() == SourceWidget)
	{
		Hide();
	}
}

void URockInventoryTooltipSubsystem::Hide()
{
	SetLayerVisibility(false);
	CurrentSource = nullptr;
}
