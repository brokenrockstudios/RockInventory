// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "RockItemRecordSubsystem.h"

#include "Item/ItemRegistry/RockItemDefinitionRegistry.h"
#include "Misc/CoreDelegates.h"
#include "RockDataKey.h"
#include "RockDataRegistry.h"
#include "RockItemRecordProvider.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RockItemRecordSubsystem)

void URockItemRecordSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	URockItemRegistrySubsystem* Registry = Collection.InitializeDependency<URockItemRegistrySubsystem>();
	if (!Registry)
	{
		return;
	}

	FRockDataRegistry& Records = FRockDataRegistry::Get();
	Provider = MakeUnique<FRockItemRecordProvider>(Records, Registry);
	Registry->RegisterProvider(Provider.Get());
	ReloadedHandle = Records.OnReloaded().AddUObject(this, &URockItemRecordSubsystem::OnRecordsReloaded);
	// The game's first Rebuild() raises no reload event: look again once the engine is up.
	PostEngineInitHandle = FCoreDelegates::GetOnPostEngineInit().AddWeakLambda(this, []()
	{
		if (URockItemRegistrySubsystem* Registry = URockItemRegistrySubsystem::GetInstance())
		{
			Registry->NotifyProviderChanged();
		}
	});
}

void URockItemRecordSubsystem::Deinitialize()
{
	FCoreDelegates::GetOnPostEngineInit().Remove(PostEngineInitHandle);
	FRockDataRegistry::Get().OnReloaded().Remove(ReloadedHandle);
	if (Provider)
	{
		if (URockItemRegistrySubsystem* Registry = URockItemRegistrySubsystem::GetInstance())
		{
			Registry->UnregisterProvider(Provider.Get());
		}
		Provider.Reset();
	}
	Super::Deinitialize();
}

void URockItemRecordSubsystem::OnRecordsReloaded(const TArray<FRockDataKey>& /*Changed*/)
{
	if (URockItemRegistrySubsystem* Registry = URockItemRegistrySubsystem::GetInstance())
	{
		Registry->NotifyProviderChanged();
	}
}
