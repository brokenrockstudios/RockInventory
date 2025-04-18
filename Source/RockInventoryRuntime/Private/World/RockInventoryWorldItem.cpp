// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.


#include "World/RockInventoryWorldItem.h"

#include "RockInventoryLogging.h"
#include "Components/RockInventoryComponent.h"
#include "Engine/StreamableManager.h"
#include "Item/RockItemDefinition.h"
#include "Net/UnrealNetwork.h"

ARockInventoryWorldItem::ARockInventoryWorldItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetMobility(EComponentMobility::Movable);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMeshComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	bReplicates = true;
	ItemStack = FRockItemStack();
}

void ARockInventoryWorldItem::BeginPlay()
{
	Super::BeginPlay();
	// Normally we shouldn't call the implementation directly, but 🤷‍♂️
	SetItemStack_Implementation(ItemStack);
}

void ARockInventoryWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARockInventoryWorldItem, ItemStack);
}

void ARockInventoryWorldItem::SetItemStack_Implementation(const FRockItemStack& InItemStack)
{
	// Should we properly initialize this InItemStack (e.g. create the item instance now or 'later'?)
	// currently assuming we will only create it upon 'looting' the item.

	ItemStack = InItemStack;
	if (GetLocalRole() == ROLE_Authority)
	{
		if (InItemStack.IsValid())
		{
			this->ForceNetUpdate();
		}
	}

	// Update the static mesh component with the item definition's mesh
	if (ItemStack.IsValid())
	{
		FStreamableManager Manager;
		TSoftObjectPtr<UStaticMesh> Mesh = ItemStack.GetDefinition()->ItemMesh;
		
		// Check if the mesh is already loaded
		if (Mesh.IsValid())
		{
			StaticMeshComponent->SetStaticMesh(Mesh.Get());
		}
		else
		{
			Manager.RequestAsyncLoad(Mesh.ToSoftObjectPath(), FStreamableDelegate::CreateWeakLambda(this, [this, Mesh]
			{
				UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItem::SetItemStack - Mesh loaded: %s"), *Mesh.ToString());
				UStaticMesh* LoadedStaticMesh = Mesh.Get();
				if (LoadedStaticMesh)
				{
					StaticMeshComponent->SetStaticMesh(LoadedStaticMesh);
				}
				else
				{
					UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItem::SetItemStack - Mesh is null"));
				}
			}));
		}
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItem::SetItemStack - ItemStack is invalid"));
	}
}

FRockItemStack ARockInventoryWorldItem::GetItemStack_Implementation() const
{
	return ItemStack;
}

void ARockInventoryWorldItem::PickedUp_Implementation(AActor* InInstigator)
{
	IRockWorldItemInterface::PickedUp_Implementation(InInstigator);
	// if instigator has an inventory, add the item to it, then destroy this actor
	URockInventoryComponent* Inventory = Cast<URockInventoryComponent>(InInstigator->GetComponentByClass(URockInventoryComponent::StaticClass()));
	if (Inventory)
	{
		int32 outExcess = ItemStack.StackSize;
		FRockInventorySlotHandle outHandle;
		if (Inventory->K2_LootItem(ItemStack, outHandle, outExcess))
		{
			ItemStack.StackSize = outExcess;
			if (outExcess <= 0)
			{
				// Successfully added to inventory, destroy this actor
				this->Destroy();
			}
		}
		else
		{
			UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItem::PickedUp - Failed to add item to inventory"));
		}
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItem::PickedUp - Instigator does not have an inventory"));
	}
}

void ARockInventoryWorldItem::OnRep_ItemStack()
{
	SetItemStack(ItemStack);
}

#if WITH_EDITOR
void ARockInventoryWorldItem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Check if the changed property is directly the ItemStack or a nested property within it
	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.GetPropertyName();
		const FName MemberPropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

		// If the property is directly ItemStack or a parent of ItemStack
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ARockInventoryWorldItem, ItemStack) ||
			(MemberPropertyName == GET_MEMBER_NAME_CHECKED(ARockInventoryWorldItem, ItemStack)))
		{
			SetItemStack(ItemStack);
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
