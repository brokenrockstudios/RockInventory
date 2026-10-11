// Copyright Broken Rock Studios LLC. All Rights Reserved.

#include "UI/Core/RockInventoryUndoInput.h"

#include "Components/RockInventoryManagerComponent.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/InputChord.h"
#include "GameFramework/PlayerController.h"
#include "UI/RockInventoryUISettings.h"

class FRockInventoryUndoInputProcessor : public IInputProcessor
{
public:
	explicit FRockInventoryUndoInputProcessor(URockInventoryUndoInputSubsystem* InOwner) : Owner(InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		URockInventoryUndoInputSubsystem* Subsystem = Owner.Get();
		return Subsystem && Subsystem->HandleKeyDown(InKeyEvent);
	}

	virtual const TCHAR* GetDebugName() const override { return TEXT("RockInventoryUndoInput"); }

private:
	TWeakObjectPtr<URockInventoryUndoInputSubsystem> Owner;
};

void URockInventoryUndoInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (FSlateApplication::IsInitialized())
	{
		Processor = MakeShared<FRockInventoryUndoInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(Processor, EInputPreProcessorType::Game);
	}
}

void URockInventoryUndoInputSubsystem::Deinitialize()
{
	if (FSlateApplication::IsInitialized() && Processor.IsValid())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(Processor);
	}
	Processor.Reset();
	Super::Deinitialize();
}

bool URockInventoryUndoInputSubsystem::Matches(const FInputChord& Chord, const FKeyEvent& KeyEvent)
{
	return Chord.Key.IsValid() && KeyEvent.GetKey() == Chord.Key
		&& KeyEvent.IsControlDown() == Chord.NeedsControl()
		&& KeyEvent.IsShiftDown() == Chord.NeedsShift()
		&& KeyEvent.IsAltDown() == Chord.NeedsAlt()
		&& KeyEvent.IsCommandDown() == Chord.NeedsCommand();
}

bool URockInventoryUndoInputSubsystem::HandleKeyDown(const FKeyEvent& KeyEvent)
{
	// A held key does not run through the whole history
	if (KeyEvent.IsRepeat())
	{
		return false;
	}
	const URockInventoryUISettings* Settings = GetDefault<URockInventoryUISettings>();
	const auto AnyMatches = [&KeyEvent](const TArray<FInputChord>& Chords)
	{
		return Chords.ContainsByPredicate([&KeyEvent](const FInputChord& Chord) { return Matches(Chord, KeyEvent); });
	};
	const bool bUndo = AnyMatches(Settings->UndoChords);
	if (!bUndo && !AnyMatches(Settings->RedoChords))
	{
		return false;
	}
	// The keyboard belongs to the first local player (split screen is not supported by prediction either)
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer || !LocalPlayer->IsPrimaryPlayer())
	{
		return false;
	}
	URockInventoryManagerComponent* Manager = URockInventoryManagerComponent::FindFor(LocalPlayer->GetPlayerController(LocalPlayer->GetWorld()));
	if (!Manager || !Manager->IsInventoryScreenOpen())
	{
		return false;
	}
	if (bUndo)
	{
		Manager->Undo();
	}
	else
	{
		Manager->Redo();
	}
	return true;
}
