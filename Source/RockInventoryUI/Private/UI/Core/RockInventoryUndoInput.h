// Copyright Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "RockInventoryUndoInput.generated.h"

class FRockInventoryUndoInputProcessor;
struct FInputChord;
struct FKeyEvent;

/**
 * Undo and redo keys while the inventory screen is open (T-80). One per local player: it registers a Slate input preprocessor, and a
 * key press matching `URockInventoryUISettings::UndoChords` or `RedoChords` calls Undo or Redo on the player's manager component and
 * is consumed, but only while the manager reports an inventory on screen (`IsInventoryScreenOpen`, fed by the container widgets).
 * Otherwise the key goes on to the game and the editor.
 */
UCLASS()
class URockInventoryUndoInputSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** True when the key was an undo or redo chord and the inventory screen is open (it was handled). */
	bool HandleKeyDown(const FKeyEvent& KeyEvent);

	/** Same key and the same modifiers held, no more. */
	static bool Matches(const FInputChord& Chord, const FKeyEvent& KeyEvent);

private:
	TSharedPtr<FRockInventoryUndoInputProcessor> Processor;
};
