// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RockInventory_ContainerBase.generated.h"

class UUniformGridPanel;
class URockInventory;
/**
 * 
 */
UCLASS()
class ROCKINVENTORYRUNTIME_API URockInventory_ContainerBase : public UUserWidget
{
	GENERATED_BODY()
public:
	// Bind to Widget
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> UniformGrid;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> ItemSlotWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> ItemSlotWidgetClass_Empty;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Width = 4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Height = 4;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 TabIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<URockInventory> Inventory;


	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventory(URockInventory* InInventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void GenerateGrid();
	
	
};
