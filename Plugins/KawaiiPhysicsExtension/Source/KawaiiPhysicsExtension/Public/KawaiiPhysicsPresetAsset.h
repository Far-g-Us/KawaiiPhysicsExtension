// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KawaiiPhysicsTypes.h"
#include "KawaiiPhysicsPresetAsset.generated.h"

/**
 * A single physics preset entry.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICSEXTENSION_API FKawaiiPhysicsPresetEntry
{
	GENERATED_BODY()

	/** Display name of the preset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
	FName PresetName;

	/** Description of when to use this preset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
	FString Description;

	/** Physics settings for this preset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
	FKawaiiPhysicsSettings PhysicsSettings;

	/** Dummy bone length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
	float DummyBoneLength = 5.0f;

	/** Whether to enable world collision */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
	bool bEnableWorldCollision = false;

	/** Number of warm up frames */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
	int32 WarmUpFrames = 5;
};

/**
 * DataAsset containing a library of physics presets.
 * Create via Content Browser -> Animation -> Kawaii Physics Preset Asset.
 */
UCLASS(BlueprintType)
class KAWAIIPHYSICSEXTENSION_API UKawaiiPhysicsPresetAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** List of physics presets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presets", meta=(TitleProperty="PresetName"))
	TArray<FKawaiiPhysicsPresetEntry> Presets;

	/**
	 * Find a preset by name.
	 * @param PresetName - Name of the preset to find
	 * @param OutPreset - Found preset (output)
	 * @return true if preset was found
	 */
	UFUNCTION(BlueprintCallable, Category="Kawaii Physics|Preset")
	bool GetPreset(FName PresetName, FKawaiiPhysicsPresetEntry& OutPreset) const
	{
		for (const FKawaiiPhysicsPresetEntry& Preset : Presets)
		{
			if (Preset.PresetName == PresetName)
			{
				OutPreset = Preset;
				return true;
			}
		}
		return false;
	}

	/** Get all preset names */
	UFUNCTION(BlueprintCallable, Category="Kawaii Physics|Preset")
	TArray<FName> GetPresetNames() const
	{
		TArray<FName> Names;
		for (const FKawaiiPhysicsPresetEntry& Preset : Presets)
		{
			Names.Add(Preset.PresetName);
		}
		return Names;
	}

	/** Create default jiggle presets for body physics (breast, butt, belly) */
	UFUNCTION(BlueprintCallable, Category="Kawaii Physics|Preset")
	static UKawaiiPhysicsPresetAsset* CreateDefaultJigglePresets(UObject* Outer)
	{
		UKawaiiPhysicsPresetAsset* Asset = NewObject<UKawaiiPhysicsPresetAsset>(Outer);

		// --- BREAST PHYSICS ---
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Breast_Small");
			Preset.Description = TEXT("Small breasts - subtle jiggle, high stiffness");
			Preset.PhysicsSettings.Damping = 0.4f;
			Preset.PhysicsSettings.Stiffness = 0.08f;
			Preset.PhysicsSettings.LimitAngle = 25.0f;
			Preset.PhysicsSettings.bUseTwistSwingLimits = true;
			Preset.PhysicsSettings.TwistLimitAngle = 15.0f;
			Preset.PhysicsSettings.SwingLimitYAngle = 20.0f;
			Preset.PhysicsSettings.SwingLimitZAngle = 25.0f;
			Preset.PhysicsSettings.Radius = 2.0f;
			Preset.PhysicsSettings.VolumePreservation = 0.3f;
			Preset.PhysicsSettings.Pressure = 0.5f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.85f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.9f;
			Preset.DummyBoneLength = 3.0f;
			Preset.WarmUpFrames = 3;
			Asset->Presets.Add(Preset);
		}
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Breast_Medium");
			Preset.Description = TEXT("Medium breasts - moderate jiggle");
			Preset.PhysicsSettings.Damping = 0.35f;
			Preset.PhysicsSettings.Stiffness = 0.05f;
			Preset.PhysicsSettings.LimitAngle = 35.0f;
			Preset.PhysicsSettings.bUseTwistSwingLimits = true;
			Preset.PhysicsSettings.TwistLimitAngle = 20.0f;
			Preset.PhysicsSettings.SwingLimitYAngle = 30.0f;
			Preset.PhysicsSettings.SwingLimitZAngle = 35.0f;
			Preset.PhysicsSettings.Radius = 3.0f;
			Preset.PhysicsSettings.VolumePreservation = 0.5f;
			Preset.PhysicsSettings.Pressure = 1.0f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.8f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.85f;
			Preset.DummyBoneLength = 4.0f;
			Preset.WarmUpFrames = 5;
			Asset->Presets.Add(Preset);
		}
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Breast_Large");
			Preset.Description = TEXT("Large breasts - heavy jiggle, more damping");
			Preset.PhysicsSettings.Damping = 0.3f;
			Preset.PhysicsSettings.Stiffness = 0.03f;
			Preset.PhysicsSettings.LimitAngle = 45.0f;
			Preset.PhysicsSettings.bUseTwistSwingLimits = true;
			Preset.PhysicsSettings.TwistLimitAngle = 25.0f;
			Preset.PhysicsSettings.SwingLimitYAngle = 40.0f;
			Preset.PhysicsSettings.SwingLimitZAngle = 45.0f;
			Preset.PhysicsSettings.Radius = 4.0f;
			Preset.PhysicsSettings.VolumePreservation = 0.7f;
			Preset.PhysicsSettings.Pressure = 1.5f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.75f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.8f;
			Preset.DummyBoneLength = 5.0f;
			Preset.WarmUpFrames = 8;
			Asset->Presets.Add(Preset);
		}

		// --- BUTT PHYSICS ---
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Butt_Firm");
			Preset.Description = TEXT("Firm buttocks - subtle bounce");
			Preset.PhysicsSettings.Damping = 0.5f;
			Preset.PhysicsSettings.Stiffness = 0.1f;
			Preset.PhysicsSettings.LimitAngle = 20.0f;
			Preset.PhysicsSettings.bUseTwistSwingLimits = true;
			Preset.PhysicsSettings.TwistLimitAngle = 10.0f;
			Preset.PhysicsSettings.SwingLimitYAngle = 15.0f;
			Preset.PhysicsSettings.SwingLimitZAngle = 20.0f;
			Preset.PhysicsSettings.Radius = 3.0f;
			Preset.PhysicsSettings.VolumePreservation = 0.4f;
			Preset.PhysicsSettings.Pressure = 0.8f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.9f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.95f;
			Preset.DummyBoneLength = 4.0f;
			Preset.WarmUpFrames = 3;
			Asset->Presets.Add(Preset);
		}
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Butt_Soft");
			Preset.Description = TEXT("Soft buttocks - noticeable jiggle");
			Preset.PhysicsSettings.Damping = 0.4f;
			Preset.PhysicsSettings.Stiffness = 0.06f;
			Preset.PhysicsSettings.LimitAngle = 30.0f;
			Preset.PhysicsSettings.bUseTwistSwingLimits = true;
			Preset.PhysicsSettings.TwistLimitAngle = 15.0f;
			Preset.PhysicsSettings.SwingLimitYAngle = 25.0f;
			Preset.PhysicsSettings.SwingLimitZAngle = 30.0f;
			Preset.PhysicsSettings.Radius = 4.0f;
			Preset.PhysicsSettings.VolumePreservation = 0.6f;
			Preset.PhysicsSettings.Pressure = 1.2f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.85f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.9f;
			Preset.DummyBoneLength = 5.0f;
			Preset.WarmUpFrames = 5;
			Asset->Presets.Add(Preset);
		}

		// --- BELLY PHYSICS ---
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Belly_Subtle");
			Preset.Description = TEXT("Subtle belly movement (pregnancy simulation)");
			Preset.PhysicsSettings.Damping = 0.6f;
			Preset.PhysicsSettings.Stiffness = 0.15f;
			Preset.PhysicsSettings.LimitAngle = 10.0f;
			Preset.PhysicsSettings.Radius = 2.0f;
			Preset.PhysicsSettings.VolumePreservation = 0.8f;
			Preset.PhysicsSettings.Pressure = 0.3f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.95f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.95f;
			Preset.DummyBoneLength = 3.0f;
			Preset.WarmUpFrames = 3;
			Asset->Presets.Add(Preset);
		}

		// --- HAIR PHYSICS (for reference) ---
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Hair_Long");
			Preset.Description = TEXT("Long hair - flowing movement");
			Preset.PhysicsSettings.Damping = 0.5f;
			Preset.PhysicsSettings.Stiffness = 0.03f;
			Preset.PhysicsSettings.LimitAngle = 60.0f;
			Preset.PhysicsSettings.Radius = 2.0f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.7f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.8f;
			Preset.DummyBoneLength = 5.0f;
			Preset.WarmUpFrames = 5;
			Asset->Presets.Add(Preset);
		}
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Hair_Short");
			Preset.Description = TEXT("Short hair - stiff, minimal movement");
			Preset.PhysicsSettings.Damping = 0.6f;
			Preset.PhysicsSettings.Stiffness = 0.1f;
			Preset.PhysicsSettings.LimitAngle = 30.0f;
			Preset.PhysicsSettings.Radius = 1.5f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.85f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.9f;
			Preset.DummyBoneLength = 3.0f;
			Preset.WarmUpFrames = 3;
			Asset->Presets.Add(Preset);
		}

		// --- CLOTHING PHYSICS ---
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Skirt_Pleated");
			Preset.Description = TEXT("Pleated skirt - structured, moderate swing");
			Preset.PhysicsSettings.Damping = 0.45f;
			Preset.PhysicsSettings.Stiffness = 0.08f;
			Preset.PhysicsSettings.LimitAngle = 50.0f;
			Preset.PhysicsSettings.Radius = 2.0f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.75f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.85f;
			Preset.DummyBoneLength = 4.0f;
			Preset.WarmUpFrames = 5;
			Asset->Presets.Add(Preset);
		}
		{
			FKawaiiPhysicsPresetEntry Preset;
			Preset.PresetName = FName("Cape_Weighted");
			Preset.Description = TEXT("Weighted cape - heavy, slow movement");
			Preset.PhysicsSettings.Damping = 0.3f;
			Preset.PhysicsSettings.Stiffness = 0.02f;
			Preset.PhysicsSettings.LimitAngle = 80.0f;
			Preset.PhysicsSettings.Radius = 3.0f;
			Preset.PhysicsSettings.WorldDampingLocation = 0.6f;
			Preset.PhysicsSettings.WorldDampingRotation = 0.7f;
			Preset.DummyBoneLength = 6.0f;
			Preset.WarmUpFrames = 8;
			Asset->Presets.Add(Preset);
		}

		return Asset;
	}
};
