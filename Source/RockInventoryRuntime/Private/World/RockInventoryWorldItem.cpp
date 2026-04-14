// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#include "World/RockInventoryWorldItem.h"

#include "RockAssetLibrary.h"
#include "RockInventoryLogging.h"
#include "Components/RockInventoryComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Item/RockItemDefinition.h"
#include "Item/RockItemInstance.h"
#include "Item/Fragment/RockItemFragment_MeshMaterialOverride.h"
#include "Library/RockInventoryLibrary.h"
#include "Misc/RockInventoryDeveloperSettings.h"
#include "Net/UnrealNetwork.h"

ARockInventoryWorldItemBase::ARockInventoryWorldItemBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetMobility(EComponentMobility::Movable);
	StaticMeshComponent->SetSimulatePhysics(false);
	StaticMeshComponent->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	RootComponent = StaticMeshComponent; // Set the root component to the static mesh component

	ItemStack = FRockItemStack();
	ItemStack.StackCount = 1;
}

void ARockInventoryWorldItemBase::BeginPlay()
{
	Super::BeginPlay();
	// TODO: Is this redundant if PostEditChangeProperty already did it?
	// What if it was stale, then we would. Need more testing.
	SetItemStack(ItemStack);


	if (HasAuthority() && ItemSeed == 0 && !ItemStack.bInitialized)
	{
		ItemSeed = FMath::Rand();
		// random for runtime spawned
		//? GetTypeHash(GetFName())  // stable for placed items
		//:

		// If the ItemDef wants some randomness,  
		// e.g. is the gun blue or red painted.  Use the above seed and do the appropriate things

		// Can the gun spawn with an optic or laser or muzzle?
		// Perhaps we'd need to ask the Settings what the 'rules' are regarding items.
		// This is the place to leverage that
	}
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

	UpdateItemVisuals();
}

void ARockInventoryWorldItemBase::OnPickedUp(AActor* InInstigator)
{
	URockInventoryComponent* InventoryComp = InInstigator->GetComponentByClass<URockInventoryComponent>();
	if (InventoryComp)
	{
		int32 outExcess = ItemStack.StackCount;
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
	ItemStack.StackCount = Excess;
	if (ItemStack.StackCount <= 0)
	{
		this->Destroy();
	}
}

void ARockInventoryWorldItemBase::ApplyThrowImpulse(const FVector& Impulse)
{
	if (StaticMeshComponent) // && StaticMeshComponent->IsSimulatingPhysics())
	{
		// Is there a better place to do this? We didn't want to always simulate on intentionally placed locations, so if we are 'throwing' it 
		StaticMeshComponent->SetSimulatePhysics(true);
		StaticMeshComponent->AddImpulse(Impulse, NAME_None, true);
	}
}

void ARockInventoryWorldItemBase::UpdateItemVisuals()
{
	// Update the static mesh component with the item definition's mesh
	const URockItemDefinition* Definition = ItemStack.GetDefinition();
	const TSoftObjectPtr<UStaticMesh> Mesh = Definition ? Definition->ItemMesh : nullptr;

	// Check if the mesh is already loaded
	if (Mesh.IsNull())
	{
		const URockInventoryDeveloperSettings* Settings = GetDefault<URockInventoryDeveloperSettings>();
		StaticMeshComponent->SetStaticMesh(Settings->FallbackWorldItemMesh.LoadSynchronous());
		UE_LOG(LogRockInventory, Warning, TEXT("ARockInventoryWorldItemBase::UpdateItemVisuals - Mesh is null, using fallback mesh"));
		return;
	}

	TArray<FSoftObjectPath> PathsToLoad;
	PathsToLoad.Add(Mesh.ToSoftObjectPath());
	if (const auto* MatOverride = FRockItemFragment_MeshMaterialOverride::Find(Definition))
	{
		MatOverride->AppendSoftPaths(PathsToLoad);
	}

	// LoadAndExecute internally handles both fast path and async path
	URockAssetLibrary::LoadAndExecute(
		PathsToLoad, this, [this, Mesh]
		{
			if (UStaticMesh* LoadedMesh = Mesh.Get())
			{
				StaticMeshComponent->SetStaticMesh(LoadedMesh);
				ApplyItemVisuals();
			}
			else
			{
				UE_LOG(LogRockInventory, Error, TEXT("ARockInventoryWorldItemBase::UpdateItemVisuals - Failed to load static mesh"));
			}
		});

	// TODO: Do we need to call if there is a runtime instance
	// RegisterReplicationWithOwner since we aren't replicating the internal runtime instance
}

void ARockInventoryWorldItemBase::ApplyItemVisuals()
{
	if (const URockItemDefinition* Definition = ItemStack.GetDefinition())
	{
		if (const FRockItemFragment_MeshMaterialOverride* MatOverride = Definition->FindFragment<FRockItemFragment_MeshMaterialOverride>())
		{
			MatOverride->ApplyTo(StaticMeshComponent);
		}
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
