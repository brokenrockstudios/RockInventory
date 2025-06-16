// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "World/RockInventoryWorldItem.h"

#include "RockInventoryLogging.h"
#include "Components/RockInventoryComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Library/RockInventoryLibrary.h"
#include "Net/UnrealNetwork.h"

ARockInventoryWorldItemBase::ARockInventoryWorldItemBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	// By default, we will leave physics false, but we can set it to true if we want to simulate physics
	// perhaps when an item is dropped or thrown.
	StaticMeshComponent->SetMobility(EComponentMobility::Movable);
	StaticMeshComponent->SetSimulatePhysics(false);
	StaticMeshComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	//StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	RootComponent = StaticMeshComponent; // Set the root component to the static mesh component

	ItemStack = FRockItemStack();

	// Default stack size for world items
	// NOTE: Generally we shouldn't be setting values like this, but this is a rare exception where we want to ensure that the item stack is initialized with a sensible default.
	// Generally, we probably want an 'item generator' and not to be placing items directly, where that'd set it appropriately.
	ItemStack.StackSize = 1;
}

void ARockInventoryWorldItemBase::BeginPlay()
{
	Super::BeginPlay();
	// Normally we shouldn't call the implementation directly, but 🤷‍♂️
	SetItemStack(ItemStack);
}

void ARockInventoryWorldItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARockInventoryWorldItemBase, ItemStack);
}

FRockItemStack ARockInventoryWorldItemBase::GetItemStack(AActor* InstigatorPawn) const
{
	return ItemStack;
}

void ARockInventoryWorldItemBase::SetItemStack(const FRockItemStack& InItemStack)
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
	if (Mesh.IsNull())
	{
		StaticMeshComponent->SetStaticMesh(nullptr);
#if WITH_EDITOR
		// Set it to the CDO of this class if it exists, otherwise set to nullptr
		if (UStaticMesh* DefaultMesh = GetClass()->GetDefaultObject<ARockInventoryWorldItemBase>()->StaticMeshComponent->GetStaticMesh())
		{
			StaticMeshComponent->SetStaticMesh(DefaultMesh);
		}
#endif
		return;
	}
	else if (Mesh.IsValid())
	{
		StaticMeshComponent->SetStaticMesh(Mesh.Get());
	}
	else
	{
		FStreamableManager& Manager = UAssetManager::GetStreamableManager();
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

void ARockInventoryWorldItemBase::OnPickedUp(AActor* InInstigator)
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

void ARockInventoryWorldItemBase::OnLooted(AActor* InstigatorPawn, const FRockItemStack& LootedItem, int32 Excess)
{
	ItemStack.StackSize = Excess;
	if (ItemStack.StackSize <= 0)
	{
		this->Destroy();
	}
}

void ARockInventoryWorldItemBase::OnRep_ItemStack()
{
	SetItemStack(ItemStack);
}

#if WITH_EDITOR
void ARockInventoryWorldItemBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Check if the changed property is directly the ItemStack or a nested property within it
	if (PropertyChangedEvent.Property)
	{
		const FName PropertyName = PropertyChangedEvent.GetPropertyName();
		const FName MemberPropertyName = PropertyChangedEvent.MemberProperty ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

		// TODO: Should change this to only really need to update on Definition changes, but for now we will update on any change to the ItemStack
		// If the property is directly ItemStack or a parent of ItemStack
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ARockInventoryWorldItemBase, ItemStack))
		{
			SetItemStack(ItemStack);
		}
		else if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(ARockInventoryWorldItemBase, ItemStack))
		{
			SetItemStack(ItemStack);
		}
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
