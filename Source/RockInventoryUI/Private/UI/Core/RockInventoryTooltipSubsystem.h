// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RockInventoryTooltipSubsystem.generated.h"

class URockInventory_ItemDescription;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventoryTooltipSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:

	// TODO: Move this to a common header
	static URockInventoryTooltipSubsystem* GetTooltipSubsystem(UWorld* World);
	
	// Lazily creates the layer the first time it's needed
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Show at screen position with provided content. Source must be alive while visible.
	void ShowItemTooltip(UUserWidget* SourceWidget, const struct FRockItemStack& Item, const FVector2D& ScreenPos);
	// Reposition (e.g., on mouse move)
	void UpdatePosition(const FVector2D& ScreenPos);
	// Hide only if the current tooltip belongs to this source (prevents flicker on fast swaps)
	void HideIfFrom(UUserWidget* SourceWidget);
	// Force hide regardless of source
	UFUNCTION(BlueprintCallable)
	void Hide();

	// Optional: small hysteresis so you don’t flicker on tiny moves
	void SetMoveThreshold(float Pixels)
	{
		MoveThreshold = Pixels;
	}
	
private:
	
	// TODO: Although we are creating this here for now, later on we should try and 'fetch' some common CanvasPanel TooltipLayer Widget or something
	// So that we don't end up with multiple CanvasPanels.
	// Root layer that sits in viewport and anchors the tooltip content
	UPROPERTY()
	TObjectPtr<UUserWidget> TooltipLayer = nullptr;
	
	UPROPERTY()
	TSubclassOf<UUserWidget> TooltipLayerClass;
	UPROPERTY()
	TSubclassOf<UUserWidget> TooltipDefaultContentClass;
	
	// Actual tooltip content widget (your item description)
	UPROPERTY()
	TObjectPtr<UUserWidget> TooltipContent = nullptr;

	// Who asked to show the current tooltip
	UPROPERTY()
	TWeakObjectPtr<UUserWidget> CurrentSource;

	// For basic jitter suppression
	FVector2D LastScreenPos = FVector2D::ZeroVector;


	float MoveThreshold = 2.0f;
	
	UUserWidget* GetOrCreateLayer();
	UUserWidget* GetOrCreateContent();
	void SetLayerVisibility(bool bVisible);
};
