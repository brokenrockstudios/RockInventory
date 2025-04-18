// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RockWorldItemInterface.h"
#include "GameFramework/Actor.h"
#include "Item/RockItemStack.h"
#include "RockInventoryWorldItem.generated.h"

// Note: This is only an example of a WorldItem.
// You can use any actor with the added interface or optionally also replace the Transactions with your own
UCLASS()
class ROCKINVENTORYRUNTIME_API ARockInventoryWorldItem : public AActor, public IRockWorldItemInterface
{
	GENERATED_BODY()
public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	// Sets default values for this actor's properties
	ARockInventoryWorldItem(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
	void SetItemStack_Implementation(const FRockItemStack& InItemStack);
	FRockItemStack GetItemStack_Implementation() const;
	void PickedUp_Implementation(AActor* InInstigator) override;
	
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

