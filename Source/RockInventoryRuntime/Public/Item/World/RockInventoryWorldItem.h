// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item/RockItemStack.h"
#include "RockInventoryWorldItem.generated.h"

UCLASS()
class ROCKINVENTORYRUNTIME_API ARockInventoryWorldItem : public AActor
{
	GENERATED_BODY()
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	// Sets default values for this actor's properties
	ARockInventoryWorldItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "RockInventory")
	void SetItemStack(const FRockItemStack& InItemStack);
	
	UFUNCTION(BlueprintCallable, Category = "RockInventory", BlueprintPure)
	FRockItemStack GetItemStack() const;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ItemStack)
	FRockItemStack ItemStack;
	
	UFUNCTION()
	void OnRep_ItemStack();

#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

