// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "Item/World/RockInventoryWorldItem.h"

#include "RockInventoryLogging.h"
#include "Engine/StreamableManager.h"
#include "Item/RockItemDefinition.h"
#include "Library/RockItemStackLibrary.h"
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
	SetItemStack(ItemStack);
}

void ARockInventoryWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARockInventoryWorldItem, ItemStack);
}

void ARockInventoryWorldItem::SetItemStack(const FRockItemStack& InItemStack)
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
	else
	{
		UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItem::SetItemStack - ItemStack is invalid"));
	}
}

FRockItemStack ARockInventoryWorldItem::GetItemStack() const
{
	return ItemStack;
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
