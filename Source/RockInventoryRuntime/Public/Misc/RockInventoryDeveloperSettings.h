// Copyright 2025 Broken Rock Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RockInventoryDeveloperSettings.generated.h"


UENUM(BlueprintType)
enum class ERockThumbnailMode : uint8
{
	Default, // Standard DataAsset icon
	Mesh, // Render the primary static mesh
	Icon, // Use explicit icon texture
	Auto, // Icon > Mesh > Default (first available)
};

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "RockInventory"))
class ROCKINVENTORYRUNTIME_API URockInventoryDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	URockInventoryDeveloperSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void PostInitProperties() override;

	UPROPERTY(EditDefaultsOnly, Config, Category = "RockInventory")
	TSubclassOf<AActor> DefaultWorldItemClass;

	UPROPERTY(EditDefaultsOnly, Config, Category = "RockInventory|Visuals")
	TSoftObjectPtr<UStaticMesh> FallbackWorldItemMesh;

	UPROPERTY(EditAnywhere, Config, Category = "Thumbnail")
	ERockThumbnailMode ItemDefinitionThumbnailMode = ERockThumbnailMode::Default;

	/** Clients show a move at once and let the server confirm it (T-78). Off: the item moves when the server's answer replicates. */
	UPROPERTY(EditAnywhere, Config, Category = "Prediction")
	bool bPredictMoves = true;

	/** Moves a client may have sent without an answer. At the cap input is held until an answer arrives. */
	UPROPERTY(EditAnywhere, Config, Category = "Prediction", meta = (ClampMin = "1", ClampMax = "32"))
	int32 PredictionMaxInFlight = 4;

	/**
	 * Seconds the oldest unanswered move may wait before the client stops predicting, shows a pending indicator and holds input
	 * (a slow or lost connection). Tune by feel.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Prediction", meta = (ClampMin = "0.05", ForceUnits = "s"))
	float PredictionPendingThresholdSeconds = 0.5f;

	/**
	 * Recovery: a predicted move that is still pending this long after it was sent (no answer) or answered (the replicated state never
	 * reached it) is abandoned and the client shows the replicated data again. Keeps a lost answer or a stopped replication from
	 * leaving input held for good.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Prediction", meta = (ClampMin = "1", ForceUnits = "s"))
	float PredictionAbandonSeconds = 5.0f;

	/** Undo entries a client keeps (T-80); the oldest go first. 0 turns undo off. */
	UPROPERTY(EditAnywhere, Config, Category = "Undo", meta = (ClampMin = "0", ClampMax = "200"))
	int32 UndoHistoryDepth = 25;

#if WITH_EDITOR
	// data validator
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
