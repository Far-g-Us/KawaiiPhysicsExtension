// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnimNode_KawaiiPhysics.h"
#include "KawaiiPhysicsLibrary.h"
#include "KawaiiPhysicsDebugWidget.generated.h"

/**
 * Runtime debug widget for KawaiiPhysics.
 * Displays per-bone physics data (Damping, Stiffness, Velocity) in real-time.
 * Add to viewport via Blueprint: Create Widget -> KawaiiPhysicsDebugWidget -> Add to Viewport.
 * 
 * Usage:
 * 1. Create a Blueprint subclass of this widget
 * 2. In your character's Blueprint, on BeginPlay: Create Widget (KawaiiPhysicsDebugWidget) -> Add to Viewport
 * 3. Call SetKawaiiPhysicsNode() with the AnimNode reference
 * 
 * The widget updates every frame and shows:
 * - Bone names with Damping/Stiffness values
 * - Active collision count
 * - Simulation time (CPU load)
 * - Total bone count
 */
UCLASS(BlueprintType, Blueprintable)
class KAWAIIPHYSICSEXTENSION_API UKawaiiPhysicsDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Set the KawaiiPhysics AnimNode reference for data display.
	 * Call this from Blueprint after creating the widget.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Debug")
	void SetKawaiiPhysicsNode(const FKawaiiPhysicsReference& InKawaiiPhysics);

	/**
	 * Get current debug data for display.
	 * Called every frame to refresh the display.
	 */
	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|Debug")
	void GetDebugData(TArray<FString>& OutBoneNames, TArray<float>& OutDamping,
	                   TArray<float>& OutStiffness, TArray<float>& OutVelocities,
	                   int32& OutBoneCount, int32& OutCollisionCount, float& OutSimTimeMs) const;

	/** Whether the debug widget is currently active */
	UPROPERTY(BlueprintReadOnly, Category = "Kawaii Physics|Debug")
	bool bIsActive = false;

	/** Refresh rate in seconds (how often to update display) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kawaii Physics|Debug")
	float RefreshRate = 0.1f;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	FKawaiiPhysicsReference KawaiiPhysicsNode;

	float LastRefreshTime = 0.0f;

	// Cached debug data
	TArray<FString> CachedBoneNames;
	TArray<float> CachedDamping;
	TArray<float> CachedStiffness;
	TArray<float> CachedVelocities;
	int32 CachedBoneCount = 0;
	int32 CachedCollisionCount = 0;
	float CachedSimTimeMs = 0.0f;
};
