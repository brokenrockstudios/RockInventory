// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "World/RockInventoryWorldItem.h"

#include "RockInventoryLogging.h"
#include "Components/RockInventoryComponent.h"
#include "Engine/StreamableManager.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
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
	Execute_SetItemStack(this, ItemStack);
}

void ARockInventoryWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARockInventoryWorldItem, ItemStack);
}

FRockItemStack ARockInventoryWorldItem::GetItemStack_Implementation(AActor* InstigatorPawn) const
{
	return ItemStack;
}

void ARockInventoryWorldItem::SetItemStack_Implementation(const FRockItemStack& InItemStack)
{
	// Should we properly initialize this InItemStack (e.g. create the item instance now or 'later'?)
	// currently assuming we will only create it upon 'looting' the item.


	ItemStack = InItemStack;
	ItemStack.TransferOwnership(this, nullptr);
	if (GetLocalRole() == ROLE_Authority)
	{
		this->ForceNetUpdate();
	}

	// Update the static mesh component with the item definition's mesh

	TSoftObjectPtr<UStaticMesh> Mesh = nullptr;
	if (const URockItemDefinition* Definition = ItemStack.GetDefinition())
	{
		Mesh = Definition->ItemMesh;
	}

	// Check if the mesh is already loaded
	if (Mesh == nullptr)
	{
		StaticMeshComponent->SetStaticMesh(nullptr);
		return;
	}
	else if (Mesh.IsValid())
	{
		StaticMeshComponent->SetStaticMesh(Mesh.Get());
	}
	else
	{
		FStreamableManager Manager;
		Manager.RequestAsyncLoad(Mesh.ToSoftObjectPath(), FStreamableDelegate::CreateWeakLambda(this, [this, Mesh]
		{
			UStaticMesh* LoadedStaticMesh = Mesh.Get();
			if (LoadedStaticMesh)
			{
				StaticMeshComponent->SetStaticMesh(LoadedStaticMesh);
			}
			else
			{
				UE_LOG(LogRockInventory, Error, TEXT("ARockInventoryWorldItem::SetItemStack - Failed to load static mesh for item stack"));
			}
		}));
	}
}

void ARockInventoryWorldItem::OnPickedUp_Implementation(AActor* InInstigator)
{
	URockInventoryComponent* InventoryComp = Cast<URockInventoryComponent>(InInstigator->GetComponentByClass(URockInventoryComponent::StaticClass()));
	if (InventoryComp)
	{
		int32 outExcess = ItemStack.StackSize;
		FRockInventorySlotHandle outHandle;

		if (URockInventoryLibrary::LootItemToInventory(InventoryComp->Inventory, ItemStack, outHandle, outExcess))
		{
			OnLooted(InInstigator, ItemStack, outExcess);
		}
		else
		{
			// This might just be because the inventory is full, but presumably we should have prevented from reaching this far earlier.
			UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItem::PickedUp - Failed to add item to inventory"));
		}
	}
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItem::PickedUp - Instigator does not have an inventory"));
	}
}

void ARockInventoryWorldItem::OnLooted_Implementation(AActor* InstigatorPawn, const FRockItemStack& LootedItem, int32 Excess)
{
	ItemStack.StackSize = Excess;
	if (ItemStack.StackSize <= 0)
	{
		this->Destroy();
	}
}

void ARockInventoryWorldItem::OnRep_ItemStack()
{
	Execute_SetItemStack(this, ItemStack);
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
			Execute_SetItemStack(this, ItemStack);
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
