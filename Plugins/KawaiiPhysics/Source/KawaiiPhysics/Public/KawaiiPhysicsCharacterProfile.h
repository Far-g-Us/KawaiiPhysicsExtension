// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KawaiiPhysicsTypes.h"
#include "KawaiiPhysicsCollisionLimits.h"
#include "KawaiiPhysicsPhysicsMaterial.h"
#include "KawaiiPhysicsCharacterProfile.generated.h"

/**
 * Bone chain type for auto-detection.
 */
UENUM(BlueprintType)
enum class EBoneChainType : uint8
{
	/** Hair chain */
	Hair,
	/** Skirt/Cape chain */
	Skirt,
	/** Tail chain */
	Tail,
	/** Breast chain */
	Breast,
	/** Weapon/Accessory chain (pendulum) */
	Weapon,
	/** Custom chain */
	Custom,
};

/**
 * Preset for a specific bone chain (hair, skirt, etc.).
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysicsChainPreset
{
	GENERATED_BODY()

	/** Chain type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	EBoneChainType ChainType = EBoneChainType::Custom;

	/** Root bone name for this chain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	FName RootBoneName;

	/** Physics settings for this chain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	FKawaiiPhysicsSettings PhysicsSettings;

	/** Collision primitives for this chain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	TArray<FSphericalLimit> ChainCollisions;

	/** Use custom collision from body mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
	bool bUseBodyMeshCollision = false;
};

/**
 * Character Physics Profile — complete physics setup for a character.
 * Contains presets for all bone chains, body collisions, and weapon physics.
 */
UCLASS(BlueprintType)
class KAWAIIPHYSICS_API UKawaiiPhysicsCharacterProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Character name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	FName CharacterName = FName("Default");

	/** Chain presets (hair, skirt, tail, breast, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	TArray<FKawaiiPhysicsChainPreset> ChainPresets;

	/** Global physics material */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	TObjectPtr<UKawaiiPhysicsPhysicsMaterial> PhysicsMaterial = nullptr;

	/** Body mesh for auto-collision */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	TObjectPtr<USkeletalMesh> BodyMeshForCollision = nullptr;

	/** Bones to skip (non-moving bones like spine, pelvis) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	TArray<FName> SkipBones;

	/**
	 * Find chain preset by root bone name.
	 */
	bool GetChainPreset(FName RootBoneName, FKawaiiPhysicsChainPreset& OutPreset) const
	{
		for (const FKawaiiPhysicsChainPreset& Preset : ChainPresets)
		{
			if (Preset.RootBoneName == RootBoneName)
			{
				OutPreset = Preset;
				return true;
			}
		}
		return false;
	}

	/**
	 * Get all chain root bone names.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Character Profile")
	TArray<FName> GetChainRootBones() const
	{
		TArray<FName> Bones;
		for (const FKawaiiPhysicsChainPreset& Preset : ChainPresets)
		{
			Bones.Add(Preset.RootBoneName);
		}
		return Bones;
	}
};
