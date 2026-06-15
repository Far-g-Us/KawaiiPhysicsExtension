// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KawaiiPhysicsTypes.h"
#include "KawaiiPhysicsPhysicsMaterial.generated.h"

/**
 * Physics Material for KawaiiPhysics.
 * Defines per-bone physics properties (damping, stiffness, limits) via material asset.
 * Apply to bones by name or bone group.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysicsMaterialBoneSetting
{
	GENERATED_BODY()

	/** Bone name or prefix (e.g. "breast_" matches breast_L, breast_R) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	FName BoneName;

	/** Physics settings override for this bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	FKawaiiPhysicsSettings PhysicsSettings;
};

UCLASS(BlueprintType)
class KAWAIIPHYSICS_API UKawaiiPhysicsPhysicsMaterial : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Material name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	FName MaterialName = FName("Default");

	/** Per-bone physics settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	TArray<FKawaiiPhysicsMaterialBoneSetting> BoneSettings;

	/** Global physics settings (applied to bones not in BoneSettings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	FKawaiiPhysicsSettings GlobalSettings;

	/**
	 * Find settings for a bone name (exact match or prefix match).
	 */
	bool GetBoneSettings(FName BoneName, FKawaiiPhysicsSettings& OutSettings) const
	{
		// Exact match first
		for (const FKawaiiPhysicsMaterialBoneSetting& Setting : BoneSettings)
		{
			if (Setting.BoneName == BoneName)
			{
				OutSettings = Setting.PhysicsSettings;
				return true;
			}
		}

		// Prefix match
		const FString BoneNameStr = BoneName.ToString();
		for (const FKawaiiPhysicsMaterialBoneSetting& Setting : BoneSettings)
		{
			const FString Prefix = Setting.BoneName.ToString();
			if (BoneNameStr.StartsWith(Prefix, ESearchCase::IgnoreCase))
			{
				OutSettings = Setting.PhysicsSettings;
				return true;
			}
		}

		OutSettings = GlobalSettings;
		return false;
	}
};
