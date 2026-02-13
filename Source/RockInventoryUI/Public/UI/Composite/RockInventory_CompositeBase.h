// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "RockInventory_CompositeBase.generated.h"

/**
 * 
 */
UCLASS()
class ROCKINVENTORYUI_API URockInventory_CompositeBase : public UUserWidget
{
	GENERATED_BODY()
public:
	FGameplayTag GetFragmentTag() const { return FragmentTag; }
	void SetFragmentTag(const FGameplayTag& Tag) { FragmentTag = Tag; }
	virtual void Collapse();
	void Expand();

	using FuncType = TFunction<void(URockInventory_CompositeBase*)>;
	virtual void ApplyFunction(FuncType Function) {}
private:

	UPROPERTY(EditAnywhere, Category = "Inventory")
	FGameplayTag FragmentTag;
};

