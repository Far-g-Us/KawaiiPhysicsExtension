// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "AnimNode_KawaiiPhysics.h"
#include "ExternalForces/KawaiiPhysicsExternalForce.h"
#include "KawaiiPhysicsPresetAsset.h"
#include "KawaiiPhysicsPhysicsMaterial.h"
#include "KawaiiPhysicsCharacterProfile.h"
#include "KawaiiPhysicsAnimDynamicsBridge.h"
#include "Animation/AnimNodeReference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KawaiiPhysicsLibrary.generated.h"

UENUM()
enum class EKawaiiPhysicsAccessExternalForceResult : uint8
{
	Valid,
	NotValid,
};

#define KAWAIIPHYSICS_VALUE_SETTER(PropertyType, PropertyName) \
{ \
    KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>( \
        TEXT("Set" #PropertyName), \
        [PropertyName](FAnimNode_KawaiiPhysics& InKawaiiPhysics) { \
            InKawaiiPhysics.PropertyName = PropertyName; \
        }); \
    return KawaiiPhysics; \
}

#define KAWAIIPHYSICS_VALUE_GETTER(PropertyType, PropertyName) \
 { \
    PropertyType Value; \
    KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>( \
        TEXT("Get" #PropertyName), \
        [&Value](FAnimNode_KawaiiPhysics& InKawaiiPhysics) { \
            Value = InKawaiiPhysics.PropertyName; \
        }); \
    return Value; \
}


USTRUCT(BlueprintType)
struct FKawaiiPhysicsReference : public FAnimNodeReference
{
	GENERATED_BODY()

	using FInternalNodeType = FAnimNode_KawaiiPhysics;
};

/**
 * Debug info for a single bone (used by GetAllBonesDebugInfo).
 */
USTRUCT(BlueprintType)
struct FKawaiiPhysicsDebugInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Kawaii Physics|Debug")
	FName BoneName;

	UPROPERTY(BlueprintReadOnly, Category = "Kawaii Physics|Debug")
	float CurrentDamping = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Kawaii Physics|Debug")
	float CurrentStiffness = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Kawaii Physics|Debug")
	float VelocityMagnitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Kawaii Physics|Debug")
	float AngularVelocityMagnitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Kawaii Physics|Debug")
	bool bColliding = false;
};

/**
 * Exposes operations to be performed on a blend space anim node.
 */
UCLASS()
class KAWAIIPHYSICSEXTENSION_API UKawaiiPhysicsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Get a KawaiiPhysics from an anim node */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta = (BlueprintThreadSafe, ExpandEnumAsExecs = "Result"))
	static FKawaiiPhysicsReference ConvertToKawaiiPhysics(const FAnimNodeReference& Node,
	                                                      EAnimNodeReferenceConversionResult& Result);

	/** Get a KawaiiPhysics from an anim node (pure). */
	UFUNCTION(BlueprintPure, Category = "Kawaii Physics",
		meta = (BlueprintThreadSafe, DisplayName = "Convert to Kawaii Physics (Pure)"))
	static void ConvertToKawaiiPhysicsPure(const FAnimNodeReference& Node, FKawaiiPhysicsReference& KawaiiPhysics,
	                                       bool& Result)
	{
		EAnimNodeReferenceConversionResult ConversionResult;
		KawaiiPhysics = ConvertToKawaiiPhysics(Node, ConversionResult);
		Result = (ConversionResult == EAnimNodeReferenceConversionResult::Succeeded);
	}

	/** Collect KawaiiPhysics Node References from AnimInstance(ABP)  */
	static bool CollectKawaiiPhysicsNodes(TArray<FKawaiiPhysicsReference>& Nodes,
	                                      UAnimInstance* AnimInstance, const FGameplayTagContainer& FilterTags,
	                                      bool bFilterExactMatch);

	/** Collect KawaiiPhysics Node References from SkeletalMeshComponent  */
	static bool CollectKawaiiPhysicsNodes(TArray<FKawaiiPhysicsReference>& Nodes,
	                                      USkeletalMeshComponent* MeshComp, const FGameplayTagContainer& FilterTags,
	                                      bool bFilterExactMatch);

	/** ResetDynamics */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference ResetDynamics(const FKawaiiPhysicsReference& KawaiiPhysics);

	/** Set RootBone */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetRootBoneName(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                               UPARAM(ref) FName& RootBoneName);
	/** Get RootBone */
	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FName GetRootBoneName(const FKawaiiPhysicsReference& KawaiiPhysics);

	/** Set ExcludeBones */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetExcludeBoneNames(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                   UPARAM(ref) TArray<FName>& ExcludeBoneNames);
	/** Get ExcludeBones */
	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static TArray<FName> GetExcludeBoneNames(const FKawaiiPhysicsReference& KawaiiPhysics);

	// PhysicsSettings
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetPhysicsSettings(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                  UPARAM(ref) FKawaiiPhysicsSettings& PhysicsSettings)
	{
		KAWAIIPHYSICS_VALUE_SETTER(FKawaiiPhysicsSettings, PhysicsSettings);
	}
	
	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsSettings GetPhysicsSettings(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(FKawaiiPhysicsSettings, PhysicsSettings);
	}

	// DummyBoneLength
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetDummyBoneLength(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                  float DummyBoneLength)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetDummyBoneLength"),
			[DummyBoneLength](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.DummyBoneLength = FMath::Max(DummyBoneLength, 0.0f);
				InKawaiiPhysics.RequestModifyBonesReinit();
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static float GetDummyBoneLength(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(float, DummyBoneLength);
	}

	// BoneSubdivisionCount
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneSubdivisionCount(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                       int32 BoneSubdivisionCount)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneSubdivisionCount"),
			[BoneSubdivisionCount](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.BoneSubdivisionCount = FMath::Clamp(BoneSubdivisionCount, 0, 10);
				InKawaiiPhysics.RequestModifyBonesReinit();
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static int32 GetBoneSubdivisionCount(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(int32, BoneSubdivisionCount);
	}

	// BoneSubdivisionCollisionOnly
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneSubdivisionCollisionOnly(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                               bool bBoneSubdivisionCollisionOnly)
	{
		KAWAIIPHYSICS_VALUE_SETTER(bool, bBoneSubdivisionCollisionOnly);
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool GetBoneSubdivisionCollisionOnly(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(bool, bBoneSubdivisionCollisionOnly);
	}

	// BoneConstraintSubdivisionCount
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneConstraintSubdivisionCount(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                                 int32 BoneConstraintSubdivisionCount)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneConstraintSubdivisionCount"),
			[BoneConstraintSubdivisionCount](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.BoneConstraintSubdivisionCount = FMath::Clamp(BoneConstraintSubdivisionCount, 0, 10);
				InKawaiiPhysics.RequestModifyBonesReinit();
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static int32 GetBoneConstraintSubdivisionCount(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(int32, BoneConstraintSubdivisionCount);
	}

	// BoneConstraintSubdivisionFeedbackScale（ランタイムスカラー: トポロジ不変のためreinit不要 / runtime scalar, no reinit）
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneConstraintSubdivisionFeedbackScale(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                                         float BoneConstraintSubdivisionFeedbackScale)
	{
		KAWAIIPHYSICS_VALUE_SETTER(float, BoneConstraintSubdivisionFeedbackScale);
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static float GetBoneConstraintSubdivisionFeedbackScale(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(float, BoneConstraintSubdivisionFeedbackScale);
	}

	// --- Chain Linking ---

	/**
	 * Create a constraint between the end bone of one chain and the start bone of another.
	 * Useful for connecting chains (e.g., chain end → necklace start, belt → skirt).
	 *
	 * @param Chain1EndBone End bone of the first chain
	 * @param Chain2StartBone Start bone of the second chain
	 * @param ComplianceType Material type for the constraint (determines stiffness)
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Chain Linking", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddChainLink(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName Chain1EndBone,
		FName Chain2StartBone,
		EXPBDComplianceType ComplianceType = EXPBDComplianceType::Leather)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddChainLink"),
			[Chain1EndBone, Chain2StartBone, ComplianceType](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				FModifyBoneConstraint NewConstraint;
				NewConstraint.Bone1.BoneName = Chain1EndBone;
				NewConstraint.Bone2.BoneName = Chain2StartBone;
				NewConstraint.ComplianceType = ComplianceType;
				NewConstraint.bOverrideCompliance = true;
				InKawaiiPhysics.BoneConstraints.Add(NewConstraint);
			});
		return KawaiiPhysics;
	}

	/**
	 * Remove a chain link constraint between two bones.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Chain Linking", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference RemoveChainLink(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName Chain1EndBone,
		FName Chain2StartBone)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("RemoveChainLink"),
			[Chain1EndBone, Chain2StartBone](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.BoneConstraints.RemoveAll([&Chain1EndBone, &Chain2StartBone](const FModifyBoneConstraint& C) {
					return (C.Bone1.BoneName == Chain1EndBone && C.Bone2.BoneName == Chain2StartBone)
						|| (C.Bone1.BoneName == Chain2StartBone && C.Bone2.BoneName == Chain1EndBone);
				});
			});
		return KawaiiPhysics;
	}

	/**
	 * Remove all chain link constraints.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Chain Linking", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference ClearChainLinks(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("ClearChainLinks"),
			[](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.BoneConstraints.Empty();
			});
		return KawaiiPhysics;
	}

	/** TeleportDistanceThreshold */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetTeleportDistanceThreshold(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                            float TeleportDistanceThreshold)
	{
		KAWAIIPHYSICS_VALUE_SETTER(float, TeleportDistanceThreshold);
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static float GetTeleportDistanceThreshold(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(float, TeleportDistanceThreshold);
	}

	/** TeleportRotationThreshold */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetTeleportRotationThreshold(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                            float TeleportRotationThreshold)
	{
		KAWAIIPHYSICS_VALUE_SETTER(float, TeleportRotationThreshold);
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static float GetTeleportRotationThreshold(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(float, TeleportRotationThreshold);
	}

	/** Gravity */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetGravity(const FKawaiiPhysicsReference& KawaiiPhysics, FVector Gravity)
	{
		KAWAIIPHYSICS_VALUE_SETTER(FVector, Gravity);
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FVector GetGravity(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(FVector, Gravity);
	}

	/** EnableWind */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetEnableWind(const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnableWind)
	{
		KAWAIIPHYSICS_VALUE_SETTER(bool, bEnableWind);
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool GetEnableWind(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(bool, bEnableWind);
	}

	/** WindScale */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetWindScale(const FKawaiiPhysicsReference& KawaiiPhysics, float WindScale)
	{
		KAWAIIPHYSICS_VALUE_SETTER(float, WindScale);
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static float GetWindScale(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(float, WindScale);
	}

	/** AllowWorldCollision */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetAllowWorldCollision(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                      bool bAllowWorldCollision)
	{
		KAWAIIPHYSICS_VALUE_SETTER(bool, bAllowWorldCollision);
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool GetAllowWorldCollision(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(bool, bAllowWorldCollision);
	}

	/** NeedWarmUp */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetNeedWarmUp(const FKawaiiPhysicsReference& KawaiiPhysics, bool bNeedWarmUp)
	{
		KAWAIIPHYSICS_VALUE_SETTER(bool, bNeedWarmUp);
	}

	/** NeedWarmUp */
	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool GetNeedWarmUp(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(bool, bNeedWarmUp);
	}

	/** LimitsDataAsset */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetLimitsDataAsset(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                  UKawaiiPhysicsLimitsDataAsset* LimitsDataAsset)
	{
		KAWAIIPHYSICS_VALUE_SETTER(TObjectPtr<UKawaiiPhysicsLimitsDataAsset>, LimitsDataAsset);
	}

	/** LimitsDataAsset */
	UFUNCTION(BlueprintPure, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static UKawaiiPhysicsLimitsDataAsset* GetLimitsDataAsset(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(TObjectPtr<UKawaiiPhysicsLimitsDataAsset>, LimitsDataAsset);
	}

	/** Add ExternalForce With ExecResult */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FKawaiiPhysicsReference AddExternalForceWithExecResult(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                              const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                              FInstancedStruct& ExternalForce, UObject* Owner);

	/** Add ExternalForce */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool AddExternalForce(const FKawaiiPhysicsReference& KawaiiPhysics,
	                             FInstancedStruct& ExternalForce, UObject* Owner, bool bIsOneShot = false);

	/** Add ExternalForces to SkeletalMeshComponent */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool AddExternalForcesToComponent(USkeletalMeshComponent* MeshComp,
	                                         UPARAM(ref) TArray<FInstancedStruct>& ExternalForces, UObject* Owner,
	                                         UPARAM(ref) FGameplayTagContainer& FilterTags,
	                                         bool bFilterExactMatch = false,
	                                         bool bIsOneShot = false);

	/** Remove ExternalForces from SkeletalMeshComponent (by Owner) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool RemoveExternalForcesFromComponent(USkeletalMeshComponent* MeshComp, UObject* Owner,
	                                              UPARAM(ref) FGameplayTagContainer& FilterTags,
	                                              bool bFilterExactMatch = false);

	/**
	 * Set alpha (input) to all KawaiiPhysics nodes in the component (and linked/post-process instances).
	 * This is intended for AnimNotifyState usage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool SetAlphaToComponent(USkeletalMeshComponent* MeshComp, float Alpha,
	                                UPARAM(ref) FGameplayTagContainer& FilterTags,
	                                bool bFilterExactMatch = false);

	/** Get current alpha (input) from the first matched KawaiiPhysics node in the component. */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics", meta=(BlueprintThreadSafe))
	static bool GetAlphaFromComponent(USkeletalMeshComponent* MeshComp, float& OutAlpha,
	                                  UPARAM(ref) FGameplayTagContainer& FilterTags,
	                                  bool bFilterExactMatch = false);

	// --- Shared Collision ---

	// --- Twist/Swing Limits ---

	/** Set bUseTwistSwingLimits */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Twist Swing", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetUseTwistSwingLimits(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                      bool bUseTwistSwingLimits)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetbUseTwistSwingLimits"),
			[bUseTwistSwingLimits](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.PhysicsSettings.bUseTwistSwingLimits = bUseTwistSwingLimits;
			});
		return KawaiiPhysics;
	}

	/** Set TwistLimitAngle */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Twist Swing", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetTwistLimitAngle(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                  float TwistLimitAngle)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetTwistLimitAngle"),
			[TwistLimitAngle](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.PhysicsSettings.TwistLimitAngle = FMath::Max(TwistLimitAngle, 0.0f);
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|Twist Swing", meta=(BlueprintThreadSafe))
	static float GetTwistLimitAngle(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		float Value = 0.0f;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetTwistLimitAngle"),
			[&Value](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				Value = InKawaiiPhysics.PhysicsSettings.TwistLimitAngle;
			});
		return Value;
	}

	/** Set SwingLimitYAngle */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Twist Swing", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetSwingLimitYAngle(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                   float SwingLimitYAngle)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetSwingLimitYAngle"),
			[SwingLimitYAngle](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.PhysicsSettings.SwingLimitYAngle = FMath::Max(SwingLimitYAngle, 0.0f);
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|Twist Swing", meta=(BlueprintThreadSafe))
	static float GetSwingLimitYAngle(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		float Value = 0.0f;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetSwingLimitYAngle"),
			[&Value](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				Value = InKawaiiPhysics.PhysicsSettings.SwingLimitYAngle;
			});
		return Value;
	}

	/** Set SwingLimitZAngle */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Twist Swing", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetSwingLimitZAngle(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                   float SwingLimitZAngle)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetSwingLimitZAngle"),
			[SwingLimitZAngle](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.PhysicsSettings.SwingLimitZAngle = FMath::Max(SwingLimitZAngle, 0.0f);
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|Twist Swing", meta=(BlueprintThreadSafe))
	static float GetSwingLimitZAngle(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		float Value = 0.0f;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetSwingLimitZAngle"),
			[&Value](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				Value = InKawaiiPhysics.PhysicsSettings.SwingLimitZAngle;
			});
		return Value;
	}

	// --- Predictive Physics ---

	/** Set PredictionScale (0.0 = no prediction, 1.0 = 1-frame prediction) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Predictive", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetPredictionScale(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                  float PredictionScale)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetPredictionScale"),
			[PredictionScale](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.PredictionScale = FMath::Clamp(PredictionScale, 0.0f, 5.0f);
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|Predictive", meta=(BlueprintThreadSafe))
	static float GetPredictionScale(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		float Value = 0.0f;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetPredictionScale"),
			[&Value](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				Value = InKawaiiPhysics.PredictionScale;
			});
		return Value;
	}

	// --- Runtime Parameter Control ---

	/** Enable or disable physics simulation on this node (disables all physics, not just world collision) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Control", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetPhysicsEnabled(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                 bool bEnabled)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetPhysicsEnabled"),
			[bEnabled](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bAllowWorldCollision = bEnabled;
				// Skip all simulation when disabled
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					Bone.bSkipSimulate = !bEnabled;
				}
				if (!bEnabled)
				{
					// Reset dynamics when disabling physics
					InKawaiiPhysics.ResetDynamics(ETeleportType::TeleportPhysics);
				}
			});
		return KawaiiPhysics;
	}

	/** Set Damping for a specific bone by name (runtime) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Control", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneDamping(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                              FName BoneName, float Damping)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneDamping"),
			[BoneName, Damping](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.Damping = FMath::Clamp(Damping, 0.0f, 1.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	/** Set Stiffness for a specific bone by name (runtime) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Control", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneStiffness(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                FName BoneName, float Stiffness)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneStiffness"),
			[BoneName, Stiffness](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.Stiffness = FMath::Clamp(Stiffness, 0.0f, 1.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	/** Set CollisionRadius for a specific bone by name (runtime) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Control", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneCollisionRadius(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                     FName BoneName, float Radius)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneCollisionRadius"),
			[BoneName, Radius](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.Radius = FMath::Max(Radius, 0.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	/** Set LimitAngle for a specific bone by name (runtime) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Control", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneLimitAngle(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                 FName BoneName, float LimitAngle)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneLimitAngle"),
			[BoneName, LimitAngle](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.LimitAngle = FMath::Max(LimitAngle, 0.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	// --- Volume Preservation ---

	/** Get volume preservation strength for a bone (0=disabled, 1=strong) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Volume Preservation", meta=(BlueprintThreadSafe))
	static bool GetBoneVolumePreservation(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, float& OutValue)
	{
		bool bFound = false;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetBoneVolumePreservation"),
			[BoneName, &OutValue, &bFound](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						OutValue = Bone.PhysicsSettings.VolumePreservation;
						bFound = true;
						break;
					}
				}
			});
		return bFound;
	}

	/** Set volume preservation strength for a bone (0=disabled, 1=strong) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Volume Preservation", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneVolumePreservation(
		const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, float VolumePreservation)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneVolumePreservation"),
			[BoneName, VolumePreservation](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.VolumePreservation = FMath::Clamp(VolumePreservation, 0.0f, 1.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	/** Get pressure for a bone (0=none, high=strong outward force) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Volume Preservation", meta=(BlueprintThreadSafe))
	static bool GetBonePressure(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, float& OutValue)
	{
		bool bFound = false;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetBonePressure"),
			[BoneName, &OutValue, &bFound](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						OutValue = Bone.PhysicsSettings.Pressure;
						bFound = true;
						break;
					}
				}
			});
		return bFound;
	}

	/** Set pressure for a bone (0=none, high=strong outward force) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Volume Preservation", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBonePressure(
		const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, float Pressure)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBonePressure"),
			[BoneName, Pressure](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.Pressure = FMath::Clamp(Pressure, 0.0f, 5.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	// --- Bone Scaling ---

	/** Get bone scale compensation for a bone (0=disabled, 1=strong bulging) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Bone Scaling", meta=(BlueprintThreadSafe))
	static bool GetBoneScaleCompensation(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, float& OutValue)
	{
		bool bFound = false;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetBoneScaleCompensation"),
			[BoneName, &OutValue, &bFound](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						OutValue = Bone.PhysicsSettings.BoneScaleCompensation;
						bFound = true;
						break;
					}
				}
			});
		return bFound;
	}

	/** Set bone scale compensation for a bone (0=disabled, 1=strong bulging) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Bone Scaling", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneScaleCompensation(
		const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, float Compensation)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneScaleCompensation"),
			[BoneName, Compensation](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.BoneScaleCompensation = FMath::Clamp(Compensation, 0.0f, 2.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	/** Get the calculated bone scale (visual bulging effect) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Bone Scaling", meta=(BlueprintThreadSafe))
	static bool GetBoneScale(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, FVector& OutScale)
	{
		bool bFound = false;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetBoneScale"),
			[BoneName, &OutScale, &bFound](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						OutScale = Bone.BoneScale;
						bFound = true;
						break;
					}
				}
			});
		return bFound;
	}

	// --- Morph Target Driving ---

	/** Enable/disable morph target driving */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Morph Targets", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetMorphTargetDriving(
		const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnable)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetMorphTargetDriving"),
			[bEnable](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bEnableMorphTargetDriving = bEnable;
			});
		return KawaiiPhysics;
	}

	/** Add a morph target to bone mapping */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Morph Targets", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddMorphTargetMapping(
		const FKawaiiPhysicsReference& KawaiiPhysics, FName MorphTargetName, FName BoneName)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddMorphTargetMapping"),
			[MorphTargetName, BoneName](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.MorphTargetBoneMapping.Add(MorphTargetName, BoneName);
			});
		return KawaiiPhysics;
	}

	/** Clear all morph target mappings */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Morph Targets", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference ClearMorphTargetMappings(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("ClearMorphTargetMappings"),
			[](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.MorphTargetBoneMapping.Empty();
			});
		return KawaiiPhysics;
	}

	// --- Pose-Based Physics Blending ---

	/** Enable/disable pose-based physics blending */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Pose Blending", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetPoseBlending(const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnable)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetPoseBlending"),
			[bEnable](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bEnablePoseBlending = bEnable;
			});
		return KawaiiPhysics;
	}

	/** Get detected pose (Standing/Sitting/Lying) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Pose Blending", meta=(BlueprintThreadSafe))
	static EKawaiiPhysicsDetectedPose GetDetectedPose(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		EKawaiiPhysicsDetectedPose Pose = EKawaiiPhysicsDetectedPose::Standing;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetDetectedPose"),
			[&Pose](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				Pose = InKawaiiPhysics.DetectedPose;
			});
		return Pose;
	}

	/** Set custom pose (for manual control) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Pose Blending", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetCustomPose(const FKawaiiPhysicsReference& KawaiiPhysics, EKawaiiPhysicsDetectedPose Pose)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetCustomPose"),
			[Pose](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.DetectedPose = Pose;
			});
		return KawaiiPhysics;
	}

	// --- Self-Collision ---

	/** Enable/disable self-collision between bones */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Self Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetSelfCollision(const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnable)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetSelfCollision"),
			[bEnable](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bEnableSelfCollision = bEnable;
			});
		return KawaiiPhysics;
	}

	// --- Spring-Based Volume Preservation ---

	/** Enable/disable spring-based volume preservation */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Spring Volume", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetSpringVolume(const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnable)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetSpringVolume"),
			[bEnable](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bEnableSpringVolume = bEnable;
			});
		return KawaiiPhysics;
	}

	// --- Animation State-Driven Physics ---

	/** Set current animation state (triggers physics preset switch) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|State Driven", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetAnimationState(const FKawaiiPhysicsReference& KawaiiPhysics, FName StateName)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetAnimationState"),
			[StateName](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.CurrentAnimationState = StateName;
			});
		return KawaiiPhysics;
	}

	/** Add physics settings for an animation state */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|State Driven", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddStatePhysicsSettings(
		const FKawaiiPhysicsReference& KawaiiPhysics, FName StateName, FKawaiiPhysicsSettings Settings)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddStatePhysicsSettings"),
			[StateName, Settings](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.StatePhysicsSettings.Add(StateName, Settings);
			});
		return KawaiiPhysics;
	}

	// --- Auto-Collision Generation ---

	/** Auto-generate collision primitives from skeletal mesh bounds */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Auto Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AutoGenerateCollisions(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		USkeletalMeshComponent* SkelComp,
		const TArray<FName>& ChainRoots)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AutoGenerateCollisions"),
			[SkelComp, ChainRoots](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.AutoGenerateCollisions(SkelComp, ChainRoots);
			});
		return KawaiiPhysics;
	}

	/**
	 * Auto-fit collision shapes to bone bounds.
	 * Analyzes the skeletal mesh and automatically adjusts collision sphere radii and capsule lengths
	 * to match the actual bone bounds. More accurate than AutoGenerateCollisions.
	 *
	 * @param SkelComp The skeletal mesh component to analyze
	 * @param ChainRoots Root bone names of chains to fit collisions for
	 * @param bFitSpheres Fit sphere collision radii to bone bounds
	 * @param bFitCapsules Fit capsule collision lengths/radii to bone bounds
	 * @param Padding Extra padding around bone bounds (cm)
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Auto Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AutoFitCollisions(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		USkeletalMeshComponent* SkelComp,
		const TArray<FName>& ChainRoots,
		bool bFitSpheres = true,
		bool bFitCapsules = true,
		float Padding = 1.0f)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AutoFitCollisions"),
			[SkelComp, ChainRoots, bFitSpheres, bFitCapsules, Padding](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				if (!SkelComp || !SkelComp->GetSkeleton())
				{
					return;
				}

				const USkeleton* Skeleton = SkelComp->GetSkeleton();
				const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

				// Fit sphere collisions
				if (bFitSpheres)
				{
					for (FSphericalLimit& Sphere : InKawaiiPhysics.SphericalLimits)
					{
						if (!Sphere.BoneName.IsNone())
						{
							const int32 BoneIndex = RefSkeleton.FindBoneIndex(Sphere.BoneName);
							if (BoneIndex >= 0)
							{
								// Get bone bounds
								FBox BoneBounds = RefSkeleton.GetRefBonePose()[BoneIndex].GetTranslationAndScale().GetBox();
								const float Radius = (BoneBounds.GetExtent() + Padding).GetMax();
								Sphere.LimitRadius = FMath::Max(Radius, 1.0f);
							}
						}
					}
				}

				// Fit capsule collisions
				if (bFitCapsules)
				{
					for (FCapsuleLimit& Capsule : InKawaiiPhysics.CapsuleLimits)
					{
						if (!Capsule.BoneName.IsNone())
						{
							const int32 BoneIndex = RefSkeleton.FindBoneIndex(Capsule.BoneName);
							if (BoneIndex >= 0)
							{
								// Get bone bounds and child bone for capsule length
								FBox BoneBounds = RefSkeleton.GetRefBonePose()[BoneIndex].GetTranslationAndScale().GetBox();
								const FVector Extent = BoneBounds.GetExtent();

								// Fit radius from X/Y extent
								Capsule.Radius = FMath::Max(FMath::Max(Extent.X, Extent.Y) + Padding, 1.0f);

								// Fit length from Z extent or child bone distance
								if (BoneIndex + 1 < RefSkeleton.GetNum())
								{
									const FVector BonePos = RefSkeleton.GetRefBonePose()[BoneIndex].GetLocation();
									const FVector ChildPos = RefSkeleton.GetRefBonePose()[BoneIndex + 1].GetLocation();
									const float BoneLength = FVector::Dist(BonePos, ChildPos);
									Capsule.HalfHeight = FMath::Max(BoneLength * 0.5f + Padding, Capsule.Radius);
								}
								else
								{
									Capsule.HalfHeight = FMath::Max(Extent.Z + Padding, Capsule.Radius);
								}
							}
						}
					}
				}
			});
		return KawaiiPhysics;
	}

	// --- Budget Allocator ---

	/** Enable/disable budget allocator integration */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Budget", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBudgetAllocator(
		const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnable, float MicroSeconds = 0.0f, int32 Priority = 5)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBudgetAllocator"),
			[bEnable, MicroSeconds, Priority](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bEnableBudgetAllocator = bEnable;
				InKawaiiPhysics.BudgetMicroSeconds = MicroSeconds;
				InKawaiiPhysics.BudgetPriority = Priority;
			});
		return KawaiiPhysics;
	}

	/** Check if simulation was skipped last frame due to budget */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Budget", meta=(BlueprintThreadSafe))
	static bool WasBudgetSkipped(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		bool bSkipped = false;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("WasBudgetSkipped"),
			[&bSkipped](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				bSkipped = InKawaiiPhysics.bBudgetSkippedLastFrame;
			});
		return bSkipped;
	}

	// --- Physics Material ---

	/** Set physics material for per-bone settings */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Material", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetPhysicsMaterial(
		const FKawaiiPhysicsReference& KawaiiPhysics, UKawaiiPhysicsPhysicsMaterial* Material)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetPhysicsMaterial"),
			[Material](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.PhysicsMaterial = Material;
			});
		return KawaiiPhysics;
	}

	// --- Ragdoll Blend ---

	/** Set ragdoll blend weight (0 = full Kawaii, 1 = full ragdoll) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Ragdoll Blend", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetRagdollBlend(
		const FKawaiiPhysicsReference& KawaiiPhysics, float Weight, float Speed = 5.0f)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetRagdollBlend"),
			[Weight, Speed](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.RagdollBlendWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
				InKawaiiPhysics.RagdollBlendSpeed = FMath::Max(Speed, 0.1f);
			});
		return KawaiiPhysics;
	}

	// --- Spring/Damper ---

	/** Set advanced spring/damper settings for a bone */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Spring Damper", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneSpringDamper(
		const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, float Frequency, float DampingRatio)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneSpringDamper"),
			[BoneName, Frequency, DampingRatio](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.bUseAdvancedSpring = true;
						Bone.PhysicsSettings.SpringFrequency = FMath::Max(Frequency, 0.1f);
						Bone.PhysicsSettings.SpringDampingRatio = FMath::Clamp(DampingRatio, 0.0f, 5.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	// --- Cloth Constraints ---

	/** Set cloth-like constraints for a bone */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Cloth Constraints", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBoneClothConstraints(
		const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName,
		float Shear, float Bend, float StretchLimit, float CompressLimit)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBoneClothConstraints"),
			[BoneName, Shear, Bend, StretchLimit, CompressLimit](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Bone.PhysicsSettings.ShearStiffness = FMath::Clamp(Shear, 0.0f, 1.0f);
						Bone.PhysicsSettings.BendStiffness = FMath::Clamp(Bend, 0.0f, 1.0f);
						Bone.PhysicsSettings.StretchLimit = FMath::Max(StretchLimit, 1.0f);
						Bone.PhysicsSettings.CompressLimit = FMath::Clamp(CompressLimit, 0.1f, 1.0f);
						break;
					}
				}
			});
		return KawaiiPhysics;
	}

	// --- AnimDynamics Bridge ---

	/** Convert AnimDynamics settings to KawaiiPhysics settings */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Migration", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsSettings ConvertAnimDynamicsSettings(const FAnimNode_AnimDynamics& AnimDynamics)
	{
		return UKawaiiPhysicsAnimDynamicsBridge::ConvertFromAnimDynamics(AnimDynamics);
	}

	/** Get recommended settings for common use cases */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Migration", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsSettings GetRecommendedPhysicsSettings(FName UseCase)
	{
		return UKawaiiPhysicsAnimDynamicsBridge::GetRecommendedSettings(UseCase);
	}

	// --- Character Focus ---

	/** Set character physics profile */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Character", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetCharacterProfile(
		const FKawaiiPhysicsReference& KawaiiPhysics, UKawaiiPhysicsCharacterProfile* Profile)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetCharacterProfile"),
			[Profile](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.CharacterProfile = Profile;
			});
		return KawaiiPhysics;
	}

	/** Enable auto-detection of bone chains */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Character", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetAutoDetectBoneChains(const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnable)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetAutoDetectBoneChains"),
			[bEnable](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bAutoDetectBoneChains = bEnable;
			});
		return KawaiiPhysics;
	}

	/** Add bones to skip list */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Character", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddSkipBones(
		const FKawaiiPhysicsReference& KawaiiPhysics, const TArray<FName>& BoneNames)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddSkipBones"),
			[BoneNames](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FName& Name : BoneNames)
				{
					InKawaiiPhysics.SkipBoneNames.AddUnique(Name);
				}
			});
		return KawaiiPhysics;
	}

	/** Enable weapon/accessory pendulum physics */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Character", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetWeaponPhysics(
		const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnable, float Stiffness = 0.3f, float Damping = 0.5f)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetWeaponPhysics"),
			[bEnable, Stiffness, Damping](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bEnableWeaponPhysics = bEnable;
				InKawaiiPhysics.WeaponPendulumStiffness = FMath::Clamp(Stiffness, 0.0f, 1.0f);
				InKawaiiPhysics.WeaponPendulumDamping = FMath::Clamp(Damping, 0.0f, 1.0f);
			});
		return KawaiiPhysics;
	}

	/** Add weapon chain root bones */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Character", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddWeaponChainRoots(
		const FKawaiiPhysicsReference& KawaiiPhysics, const TArray<FName>& RootBones)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddWeaponChainRoots"),
			[RootBones](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FName& Root : RootBones)
				{
					InKawaiiPhysics.WeaponChainRoots.AddUnique(Root);
				}
			});
		return KawaiiPhysics;
	}

	/** Enable body mesh collision for cloth-to-body */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Character", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetBodyMeshCollision(const FKawaiiPhysicsReference& KawaiiPhysics, bool bEnable)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetBodyMeshCollision"),
			[bEnable](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bUseBodyMeshForCollision = bEnable;
			});
		return KawaiiPhysics;
	}

	// --- Physics State Serialization ---

	// --- Preset System ---

	/** Apply a physics preset from a PresetAsset */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Preset", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference ApplyPreset(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		UKawaiiPhysicsPresetAsset* PresetAsset,
		FName PresetName)
	{
		if (!PresetAsset)
		{
			return KawaiiPhysics;
		}

		FKawaiiPhysicsPresetEntry Preset;
		if (!PresetAsset->GetPreset(PresetName, Preset))
		{
			return KawaiiPhysics;
		}

		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("ApplyPreset"),
			[Preset](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.PhysicsSettings = Preset.PhysicsSettings;
				InKawaiiPhysics.DummyBoneLength = Preset.DummyBoneLength;
				InKawaiiPhysics.bAllowWorldCollision = Preset.bEnableWorldCollision;
				InKawaiiPhysics.WarmUpFrames = Preset.WarmUpFrames;
				InKawaiiPhysics.bNeedWarmUp = Preset.WarmUpFrames > 0;
				InKawaiiPhysics.RequestModifyBonesReinit();
			});
		return KawaiiPhysics;
	}

	/** Get available preset names from a PresetAsset */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Preset", meta=(BlueprintThreadSafe))
	static TArray<FName> GetPresetNames(UKawaiiPhysicsPresetAsset* PresetAsset)
	{
		if (PresetAsset)
		{
			return PresetAsset->GetPresetNames();
		}
		return {};
	}

	/** Create default jiggle presets for body physics (breast, butt, belly, hair, clothing) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Preset", meta=(BlueprintThreadSafe))
	static UKawaiiPhysicsPresetAsset* CreateDefaultJigglePresets(UObject* Outer)
	{
		return UKawaiiPhysicsPresetAsset::CreateDefaultJigglePresets(Outer);
	}

	// --- Runtime Collision Primitives ---

	// --- Bone Velocity Access ---

	/** Get the current velocity of a bone by name (location delta per frame) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Velocity", meta=(BlueprintThreadSafe))
	static bool GetBoneVelocity(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, FVector& OutVelocity)
	{
		bool bFound = false;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetBoneVelocity"),
			[BoneName, &OutVelocity, &bFound](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						OutVelocity = (Bone.Location - Bone.PrevLocation) / FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);
						bFound = true;
						break;
					}
				}
			});
		return bFound;
	}

	/** Get the current speed (magnitude of velocity) of a bone by name */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Velocity", meta=(BlueprintThreadSafe))
	static float GetBoneSpeed(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName)
	{
		FVector Velocity;
		if (GetBoneVelocity(KawaiiPhysics, BoneName, Velocity))
		{
			return Velocity.Size();
		}
		return 0.0f;
	}

	/** Get the angular velocity (rotation delta) of a bone by name */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Velocity", meta=(BlueprintThreadSafe))
	static bool GetBoneAngularVelocity(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName, FVector& OutAngularVelocity)
	{
		bool bFound = false;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetBoneAngularVelocity"),
			[BoneName, &OutAngularVelocity, &bFound](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						const FQuat DeltaRot = Bone.PrevRotation.Inverse() * Bone.PoseRotation;
						FVector Axis;
						float Angle;
						DeltaRot.ToAxisAndAngle(Axis, Angle);
						OutAngularVelocity = Axis * Angle / FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);
						bFound = true;
						break;
					}
				}
			});
		return bFound;
	}

	/** Get velocities of all bones (for batch processing) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Velocity", meta=(BlueprintThreadSafe))
	static void GetAllBoneVelocities(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                 TArray<FName>& OutBoneNames, TArray<FVector>& OutVelocities)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetAllBoneVelocities"),
			[&OutBoneNames, &OutVelocities](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (!Bone.bDummy && !Bone.bBridgeDummy)
					{
						OutBoneNames.Add(Bone.BoneRef.BoneName);
						OutVelocities.Add((Bone.Location - Bone.PrevLocation) / FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f));
					}
				}
			});
	}

	/**
	 * Get debug info for all bones (for UMG widget visualization).
	 * Returns per-bone Damping, Stiffness, Velocity, AngularVelocity, and collision state.
	 * Use in Blueprint: Create UMG widget -> Tick -> GetAllBonesDebugInfo -> Display in ListView.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Debug", meta=(BlueprintThreadSafe))
	static TArray<FKawaiiPhysicsDebugInfo> GetAllBonesDebugInfo(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		TArray<FKawaiiPhysicsDebugInfo> Result;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetAllBonesDebugInfo"),
			[&Result](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				// Count active collisions per bone (simplified: any collision touching the bone)
				const int32 TotalCollisions = InKawaiiPhysics.SphericalLimits.Num()
					+ InKawaiiPhysics.CapsuleLimits.Num()
					+ InKawaiiPhysics.BoxLimits.Num();

				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.bDummy || Bone.bBridgeDummy || !Bone.BoneRef.BoneIndex.IsValid())
					{
						continue;
					}

					FKawaiiPhysicsDebugInfo Info;
					Info.BoneName = Bone.BoneRef.BoneName;
					Info.CurrentDamping = Bone.PhysicsSettings.Damping;
					Info.CurrentStiffness = Bone.PhysicsSettings.Stiffness;
					Info.VelocityMagnitude = (Bone.Location - Bone.PrevLocation).Size()
						/ FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);

					// Angular velocity magnitude
					const FQuat DeltaRot = Bone.PrevRotation.Inverse() * Bone.PoseRotation;
					FVector Axis;
					float Angle;
					DeltaRot.ToAxisAndAngle(Axis, Angle);
					Info.AngularVelocityMagnitude = FMath::Abs(Angle)
						/ FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);

					Info.bColliding = (TotalCollisions > 0);
					Result.Add(Info);
				}
			});
		return Result;
	}

	/**
	 * Export physics data as Animation Curves for MetaSounds integration.
	 * Creates curves: "Kawaii_Physics_{BoneName}_Velocity" (magnitude) and
	 * "Kawaii_Physics_{BoneName}_Acceleration" for each non-dummy bone.
	 * MetaSounds can read these via the Animation Curve interface.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Audio", meta=(BlueprintThreadSafe))
	static void ExportPhysicsCurvesForAudio(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("ExportPhysicsCurvesForAudio"),
			[](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				if (!InKawaiiPhysics.AnimInstanceProxy)
				{
					return;
				}
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.bDummy || Bone.bBridgeDummy || !Bone.BoneRef.BoneIndex.IsValid())
					{
						continue;
					}
					const float Velocity = (Bone.Location - Bone.PrevLocation).Size()
						/ FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);
					const FName VelocityCurveName = *FString::Printf(
						TEXT("Kawaii_Physics_%s_Velocity"), *Bone.BoneRef.BoneName.ToString());
					const_cast<FAnimInstanceProxy*>(InKawaiiPhysics.AnimInstanceProxy)
						->SetCurveValue(VelocityCurveName, Velocity);
				}
			});
	}

	/**
	 * Get velocity magnitude of a specific bone (cm/s).
	 * Use in Blueprint: Tick -> GetBoneVelocityMagnitude -> Set MetaSound Parameter.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Audio", meta=(BlueprintThreadSafe))
	static float GetBoneVelocityMagnitude(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName)
	{
		float Result = 0.0f;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetBoneVelocityMagnitude"),
			[&Result, &BoneName](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						Result = (Bone.Location - Bone.PrevLocation).Size()
							/ FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);
						break;
					}
				}
			});
		return Result;
	}

	/**
	 * Get acceleration magnitude of a specific bone (cm/s²).
	 * Use in Blueprint: Tick -> GetBoneAcceleration -> Set MetaSound Parameter.
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Audio", meta=(BlueprintThreadSafe))
	static float GetBoneAcceleration(const FKawaiiPhysicsReference& KawaiiPhysics, FName BoneName)
	{
		float Result = 0.0f;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetBoneAcceleration"),
			[&Result, &BoneName](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (Bone.BoneRef.BoneName == BoneName)
					{
						const FVector Velocity = (Bone.Location - Bone.PrevLocation)
							/ FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);
						const FVector PrevVelocity = (Bone.PrevLocation - Bone.PrevPrevLocation)
							/ FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);
						Result = (Velocity - PrevVelocity).Size()
							/ FMath::Max(InKawaiiPhysics.DeltaTime, 0.001f);
						break;
					}
				}
			});
		return Result;
	}

	// --- Runtime Collision Primitives ---

	/** Add a spherical collision at runtime */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddRuntimeSphereCollision(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName BoneName, FVector Offset, float Radius, bool bOuter = true)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddRuntimeSphereCollision"),
			[BoneName, Offset, Radius, bOuter](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				FSphericalLimit Sphere;
				Sphere.DrivingBone.BoneName = BoneName;
				Sphere.DrivingBone.Initialize(InKawaiiPhysics.ModifyBones.Num() > 0
					? InKawaiiPhysics.ModifyBones[0].BoneRef.GetSkeleton() : nullptr);
				Sphere.OffsetLocation = Offset;
				Sphere.Radius = Radius;
				Sphere.LimitType = bOuter ? ESphericalLimitType::Outer : ESphericalLimitType::Inner;
				Sphere.bEnable = true;
				InKawaiiPhysics.SphericalLimits.Add(Sphere);
			});
		return KawaiiPhysics;
	}

	/** Add a capsule collision at runtime */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddRuntimeCapsuleCollision(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName BoneName, FVector Offset, float Radius, float Length)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddRuntimeCapsuleCollision"),
			[BoneName, Offset, Radius, Length](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				FCapsuleLimit Capsule;
				Capsule.DrivingBone.BoneName = BoneName;
				Capsule.DrivingBone.Initialize(InKawaiiPhysics.ModifyBones.Num() > 0
					? InKawaiiPhysics.ModifyBones[0].BoneRef.GetSkeleton() : nullptr);
				Capsule.OffsetLocation = Offset;
				Capsule.Radius = Radius;
				Capsule.Length = Length;
				Capsule.bEnable = true;
				InKawaiiPhysics.CapsuleLimits.Add(Capsule);
			});
		return KawaiiPhysics;
	}

	/** Add a box collision at runtime */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddRuntimeBoxCollision(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName BoneName, FVector Offset, FVector Extent)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddRuntimeBoxCollision"),
			[BoneName, Offset, Extent](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				FBoxLimit Box;
				Box.DrivingBone.BoneName = BoneName;
				Box.DrivingBone.Initialize(InKawaiiPhysics.ModifyBones.Num() > 0
					? InKawaiiPhysics.ModifyBones[0].BoneRef.GetSkeleton() : nullptr);
				Box.OffsetLocation = Offset;
				Box.Extent = Extent;
				Box.bEnable = true;
				InKawaiiPhysics.BoxLimits.Add(Box);
			});
		return KawaiiPhysics;
	}

	/** Add a planar collision at runtime */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddRuntimePlaneCollision(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName BoneName, FVector Offset, FRotator Rotation)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddRuntimePlaneCollision"),
			[BoneName, Offset, Rotation](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				FPlanarLimit Plane;
				Plane.DrivingBone.BoneName = BoneName;
				Plane.DrivingBone.Initialize(InKawaiiPhysics.ModifyBones.Num() > 0
					? InKawaiiPhysics.ModifyBones[0].BoneRef.GetSkeleton() : nullptr);
				Plane.OffsetLocation = Offset;
				Plane.OffsetRotation = Rotation;
				Plane.bEnable = true;
				InKawaiiPhysics.PlanarLimits.Add(Plane);
			});
		return KawaiiPhysics;
	}

	/** Clear all runtime-added collision primitives */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference ClearRuntimeCollisions(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("ClearRuntimeCollisions"),
			[](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.SphericalLimits.Empty();
				InKawaiiPhysics.CapsuleLimits.Empty();
				InKawaiiPhysics.BoxLimits.Empty();
				InKawaiiPhysics.PlanarLimits.Empty();
			});
		return KawaiiPhysics;
	}

	/** Get count of active collision primitives */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Collision", meta=(BlueprintThreadSafe))
	static void GetCollisionPrimitiveCounts(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                       int32& Spheres, int32& Capsules, int32& Boxes, int32& Planes, int32& Meshes)
	{
		Spheres = 0; Capsules = 0; Boxes = 0; Planes = 0; Meshes = 0;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetCollisionPrimitiveCounts"),
			[&Spheres, &Capsules, &Boxes, &Planes, &Meshes](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				Spheres = InKawaiiPhysics.SphericalLimits.Num();
				Capsules = InKawaiiPhysics.CapsuleLimits.Num();
				Boxes = InKawaiiPhysics.BoxLimits.Num();
				Planes = InKawaiiPhysics.PlanarLimits.Num();
				Meshes = InKawaiiPhysics.MeshCollisions.Num();
			});
	}

	// --- Mesh Collision ---

	/** Add a convex hull mesh collision at runtime */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Mesh Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddRuntimeMeshCollision(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName BoneName, const TArray<FVector>& ConvexHullVertices, float MeshScale = 1.0f)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddRuntimeMeshCollision"),
			[BoneName, ConvexHullVertices, MeshScale](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				FKawaiiPhysicsMeshCollision MeshCol;
				MeshCol.DrivingBone.BoneName = BoneName;
				MeshCol.DrivingBone.Initialize(InKawaiiPhysics.ModifyBones.Num() > 0
					? InKawaiiPhysics.ModifyBones[0].BoneRef.GetSkeleton() : nullptr);
				MeshCol.MeshSource = EMeshCollisionSourceType::ConvexHull;
				MeshCol.ConvexHullVertices = ConvexHullVertices;
				MeshCol.MeshScale = MeshScale;
				MeshCol.bEnable = true;
				InKawaiiPhysics.MeshCollisions.Add(MeshCol);
			});
		return KawaiiPhysics;
	}

	/** Add a static mesh collision at runtime */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Mesh Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddRuntimeStaticMeshCollision(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName BoneName, UStaticMesh* CollisionMesh, float MeshScale = 1.0f)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddRuntimeStaticMeshCollision"),
			[BoneName, CollisionMesh, MeshScale](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				FKawaiiPhysicsMeshCollision MeshCol;
				MeshCol.DrivingBone.BoneName = BoneName;
				MeshCol.DrivingBone.Initialize(InKawaiiPhysics.ModifyBones.Num() > 0
					? InKawaiiPhysics.ModifyBones[0].BoneRef.GetSkeleton() : nullptr);
				MeshCol.MeshSource = EMeshCollisionSourceType::StaticMesh;
				MeshCol.CollisionMesh = CollisionMesh;
				MeshCol.MeshScale = MeshScale;
				MeshCol.bEnable = true;
				InKawaiiPhysics.MeshCollisions.Add(MeshCol);
			});
		return KawaiiPhysics;
	}

	/** Add a physics asset collision at runtime */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Mesh Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference AddRuntimePhysicsAssetCollision(
		const FKawaiiPhysicsReference& KawaiiPhysics,
		FName BoneName, UPhysicsAsset* PhysicsAsset, FName PhysicsBodyName, float MeshScale = 1.0f)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("AddRuntimePhysicsAssetCollision"),
			[BoneName, PhysicsAsset, PhysicsBodyName, MeshScale](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				FKawaiiPhysicsMeshCollision MeshCol;
				MeshCol.DrivingBone.BoneName = BoneName;
				MeshCol.DrivingBone.Initialize(InKawaiiPhysics.ModifyBones.Num() > 0
					? InKawaiiPhysics.ModifyBones[0].BoneRef.GetSkeleton() : nullptr);
				MeshCol.MeshSource = EMeshCollisionSourceType::PhysicsAsset;
				MeshCol.PhysicsAsset = PhysicsAsset;
				MeshCol.PhysicsBodyName = PhysicsBodyName;
				MeshCol.MeshScale = MeshScale;
				MeshCol.bEnable = true;
				InKawaiiPhysics.MeshCollisions.Add(MeshCol);
			});
		return KawaiiPhysics;
	}

	/** Clear all mesh collisions */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Mesh Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference ClearMeshCollisions(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("ClearMeshCollisions"),
			[](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.MeshCollisions.Empty();
			});
		return KawaiiPhysics;
	}

	/** Get count of active mesh collisions */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Mesh Collision", meta=(BlueprintThreadSafe))
	static int32 GetMeshCollisionCount(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		int32 Count = 0;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetMeshCollisionCount"),
			[&Count](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				Count = InKawaiiPhysics.MeshCollisions.Num();
			});
		return Count;
	}

	// --- Physics State Serialization ---

	/** Save the current physics state (bone locations and prev locations) */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|State", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsSavedState SavePhysicsState(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		FKawaiiPhysicsSavedState State;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SavePhysicsState"),
			[&State](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (const FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (!Bone.bDummy && !Bone.bBridgeDummy)
					{
						State.BoneLocations.Add(Bone.BoneRef.BoneName, Bone.Location);
						State.BonePrevLocations.Add(Bone.BoneRef.BoneName, Bone.PrevLocation);
						State.BonePrevRotations.Add(Bone.BoneRef.BoneName, Bone.PrevRotation);
						State.BonePoseLocations.Add(Bone.BoneRef.BoneName, Bone.PoseLocation);
					}
				}
				State.bIsValid = true;
			});
		return State;
	}

	/** Restore a previously saved physics state */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|State", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference RestorePhysicsState(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                   UPARAM(ref) FKawaiiPhysicsSavedState& State)
	{
		if (!State.bIsValid)
		{
			return KawaiiPhysics;
		}

		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("RestorePhysicsState"),
			[&State](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				for (FKawaiiPhysicsModifyBone& Bone : InKawaiiPhysics.ModifyBones)
				{
					if (const FVector* Loc = State.BoneLocations.Find(Bone.BoneRef.BoneName))
					{
						Bone.Location = *Loc;
					}
					if (const FVector* PrevLoc = State.BonePrevLocations.Find(Bone.BoneRef.BoneName))
					{
						Bone.PrevLocation = *PrevLoc;
					}
					if (const FQuat* PrevRot = State.BonePrevRotations.Find(Bone.BoneRef.BoneName))
					{
						Bone.PrevRotation = *PrevRot;
					}
					if (const FVector* PoseLoc = State.BonePoseLocations.Find(Bone.BoneRef.BoneName))
					{
						Bone.PoseLocation = *PoseLoc;
					}
				}
			});
		return KawaiiPhysics;
	}

	// --- Shared Collision ---

	/**
	 * このノードをコリジョン共有のSourceにするかを設定
	 * Set whether this node acts as a shared collision source
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Shared Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetSharedCollisionSource(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                        bool bSharedCollisionSource)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetbSharedCollisionSource"),
			[bSharedCollisionSource](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bSharedCollisionSource = bSharedCollisionSource;
				InKawaiiPhysics.RequestSharedCollisionReinit();
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|Shared Collision", meta=(BlueprintThreadSafe))
	static bool GetSharedCollisionSource(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(bool, bSharedCollisionSource);
	}

	/**
	 * 他のKawaiiPhysicsから共有コリジョンを使用するかを設定
	 * Set whether to use shared collision limits from other KawaiiPhysics nodes
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Shared Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetUseSharedCollision(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                     bool bUseSharedCollision)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetbUseSharedCollision"),
			[bUseSharedCollision](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bUseSharedCollision = bUseSharedCollision;
				InKawaiiPhysics.RequestSharedCollisionReinit();
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|Shared Collision", meta=(BlueprintThreadSafe))
	static bool GetUseSharedCollision(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(bool, bUseSharedCollision);
	}

	/**
	 * 共有コリジョンのグループタグを設定
	 * Set the group tag for shared collision
	 */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Shared Collision", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetSharedCollisionGroupTag(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                          FGameplayTag SharedCollisionGroupTag)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetSharedCollisionGroupTag"),
			[SharedCollisionGroupTag](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.SharedCollisionGroupTag = SharedCollisionGroupTag;
				InKawaiiPhysics.RequestSharedCollisionReinit();
			});
		return KawaiiPhysics;
	}

	UFUNCTION(BlueprintPure, Category = "Kawaii Physics|Shared Collision", meta=(BlueprintThreadSafe))
	static FGameplayTag GetSharedCollisionGroupTag(const FKawaiiPhysicsReference& KawaiiPhysics)
	{
		KAWAIIPHYSICS_VALUE_GETTER(FGameplayTag, SharedCollisionGroupTag);
	}

	/** Set ExternalForceParameter template */
	template <typename ValueType, typename PropertyType>
	static FKawaiiPhysicsReference SetExternalForceProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                        const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                        int ExternalForceIndex, FName PropertyName,
	                                                        ValueType Value);
	/** Get ExternalForceParameter template */
	template <typename ValueType>
	static ValueType GetExternalForceProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                          const FKawaiiPhysicsReference& KawaiiPhysics, int ExternalForceIndex,
	                                          FName PropertyName);

	/** Set ExternalForceParameter template struct */
	template <typename ValueType>
	static FKawaiiPhysicsReference SetExternalForceStructProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                              const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                              int ExternalForceIndex, FName PropertyName,
	                                                              ValueType Value);
	/** Get ExternalForceParameter template struct */
	template <typename ValueType>
	static ValueType GetExternalForceStructProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                int ExternalForceIndex,
	                                                FName PropertyName);

	/** Set ExternalForceParameter bool */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FKawaiiPhysicsReference SetExternalForceBoolProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                            const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                            int ExternalForceIndex, FName PropertyName,
	                                                            bool Value)
	{
		return SetExternalForceProperty<bool, FBoolProperty>(ExecResult, KawaiiPhysics, ExternalForceIndex,
		                                                     PropertyName, Value);
	}

	/** Get ExternalForceParameter bool */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static bool GetExternalForceBoolProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                         const FKawaiiPhysicsReference& KawaiiPhysics, int ExternalForceIndex,
	                                         FName PropertyName)
	{
		return GetExternalForceProperty<bool>(ExecResult, KawaiiPhysics, ExternalForceIndex, PropertyName);
	}

	/** Set ExternalForceParameter int */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FKawaiiPhysicsReference SetExternalForceIntProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                           const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                           int ExternalForceIndex, FName PropertyName,
	                                                           int32 Value)
	{
		return SetExternalForceProperty<int32, FIntProperty>(ExecResult, KawaiiPhysics, ExternalForceIndex,
		                                                     PropertyName, Value);
	}

	/** Get ExternalForceParameter int */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static int32 GetExternalForceIntProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                         const FKawaiiPhysicsReference& KawaiiPhysics, int ExternalForceIndex,
	                                         FName PropertyName)
	{
		return GetExternalForceProperty<int32>(ExecResult, KawaiiPhysics, ExternalForceIndex, PropertyName);
	}

	/** Set ExternalForceParameter float */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FKawaiiPhysicsReference SetExternalForceFloatProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                             const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                             int ExternalForceIndex, FName PropertyName,
	                                                             float Value)
	{
		return SetExternalForceProperty<float, FFloatProperty>(ExecResult, KawaiiPhysics, ExternalForceIndex,
		                                                       PropertyName, Value);
	}

	/** Get ExternalForceParameter float */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static float GetExternalForceFloatProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                           const FKawaiiPhysicsReference& KawaiiPhysics, int ExternalForceIndex,
	                                           FName PropertyName)
	{
		return GetExternalForceProperty<float>(ExecResult, KawaiiPhysics, ExternalForceIndex, PropertyName);
	}

	/** Get ExternalForceParameter Vector */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FKawaiiPhysicsReference SetExternalForceVectorProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                              const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                              int ExternalForceIndex, FName PropertyName,
	                                                              FVector Value)
	{
		return SetExternalForceStructProperty<FVector>(ExecResult, KawaiiPhysics, ExternalForceIndex,
		                                               PropertyName, Value);
	}

	/** Get ExternalForceParameter Vector */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FVector GetExternalForceVectorProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                              const FKawaiiPhysicsReference& KawaiiPhysics, int ExternalForceIndex,
	                                              FName PropertyName)
	{
		return GetExternalForceStructProperty<FVector>(ExecResult, KawaiiPhysics, ExternalForceIndex, PropertyName);
	}

	/** Get ExternalForceParameter Rotator */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FKawaiiPhysicsReference SetExternalForceRotatorProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                               const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                               int ExternalForceIndex, FName PropertyName,
	                                                               FRotator Value)
	{
		return SetExternalForceStructProperty<FRotator>(ExecResult, KawaiiPhysics, ExternalForceIndex,
		                                                PropertyName, Value);
	}

	/** Get ExternalForceParameter Rotator */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FRotator GetExternalForceRotatorProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                int ExternalForceIndex,
	                                                FName PropertyName)
	{
		return GetExternalForceStructProperty<FRotator>(ExecResult, KawaiiPhysics, ExternalForceIndex, PropertyName);
	}

	/** Get ExternalForceParameter Transform */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FKawaiiPhysicsReference SetExternalForceTransformProperty(
		EKawaiiPhysicsAccessExternalForceResult& ExecResult,
		const FKawaiiPhysicsReference& KawaiiPhysics,
		int ExternalForceIndex, FName PropertyName,
		FTransform Value)
	{
		return SetExternalForceStructProperty<FTransform>(ExecResult, KawaiiPhysics, ExternalForceIndex,
		                                                  PropertyName, Value);
	}

	/** Get ExternalForceParameter Transform */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult"))
	static FTransform GetExternalForceTransformProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                                    const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                    int ExternalForceIndex,
	                                                    FName PropertyName)
	{
		return GetExternalForceStructProperty<FTransform>(ExecResult, KawaiiPhysics, ExternalForceIndex, PropertyName);
	}

	/** Set ExternalForceParameter Wildcard */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult", CustomStructureParam = "Value"))
	static void SetExternalForceWildcardProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                             const FKawaiiPhysicsReference& KawaiiPhysics, int ExternalForceIndex,
	                                             FName PropertyName, const int32& Value)
	{
		checkNoEntry();
	}


	/** Get ExternalForceParameter Wildcard */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Kawaii Physics",
		meta=(BlueprintThreadSafe, ExpandEnumAsExecs = "ExecResult", CustomStructureParam = "Value"))
	static void GetExternalForceWildcardProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
	                                             const FKawaiiPhysicsReference& KawaiiPhysics, int ExternalForceIndex,
	                                             FName PropertyName, int32& Value)
	{
		checkNoEntry();
	}

private:
	DECLARE_FUNCTION(execSetExternalForceWildcardProperty);
	DECLARE_FUNCTION(execGetExternalForceWildcardProperty);
};

template <typename ValueType, typename PropertyType>
FKawaiiPhysicsReference UKawaiiPhysicsLibrary::SetExternalForceProperty(
	EKawaiiPhysicsAccessExternalForceResult& ExecResult, const FKawaiiPhysicsReference& KawaiiPhysics,
	int ExternalForceIndex, FName PropertyName, ValueType Value)
{
	ExecResult = EKawaiiPhysicsAccessExternalForceResult::NotValid;

	KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
		TEXT("SetExternalForceProperty"),
		[&ExecResult, &ExternalForceIndex, &PropertyName, &Value](FAnimNode_KawaiiPhysics& InKawaiiPhysics)
		{
			if (InKawaiiPhysics.ExternalForces.IsValidIndex(ExternalForceIndex) &&
				InKawaiiPhysics.ExternalForces[ExternalForceIndex].IsValid())
			{
				const auto* ScriptStruct = InKawaiiPhysics.ExternalForces[ExternalForceIndex].GetScriptStruct();
				auto& Force = InKawaiiPhysics.ExternalForces[ExternalForceIndex].GetMutable<
					FKawaiiPhysics_ExternalForce>();

				if (const PropertyType* Property = FindFProperty<PropertyType>(ScriptStruct, PropertyName))
				{
					if (void* ValuePtr = Property->template ContainerPtrToValuePtr<uint8>(&Force))
					{
						Property->SetPropertyValue(ValuePtr, Value);
						ExecResult = EKawaiiPhysicsAccessExternalForceResult::Valid;
					}
				}
			}
		});

	return KawaiiPhysics;
}

template <typename ValueType>
ValueType UKawaiiPhysicsLibrary::GetExternalForceProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
                                                          const FKawaiiPhysicsReference& KawaiiPhysics,
                                                          int ExternalForceIndex, FName PropertyName)
{
	ValueType Result;
	ExecResult = EKawaiiPhysicsAccessExternalForceResult::NotValid;

	KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
		TEXT("GetExternalForceProperty"),
		[&Result, &ExecResult, &ExternalForceIndex, &PropertyName](FAnimNode_KawaiiPhysics& InKawaiiPhysics)
		{
			if (InKawaiiPhysics.ExternalForces.IsValidIndex(ExternalForceIndex) &&
				InKawaiiPhysics.ExternalForces[ExternalForceIndex].IsValid())
			{
				const auto* ScriptStruct = InKawaiiPhysics.ExternalForces[ExternalForceIndex].GetScriptStruct();
				const auto& Force = InKawaiiPhysics.ExternalForces[ExternalForceIndex].GetMutable<
					FKawaiiPhysics_ExternalForce>();

				if (const FProperty* Property = FindFProperty<FProperty>(ScriptStruct, PropertyName))
				{
					Result = *(Property->ContainerPtrToValuePtr<ValueType>(&Force));
					ExecResult = EKawaiiPhysicsAccessExternalForceResult::Valid;
				}
			}
		});

	return Result;
}

template <typename ValueType>
FKawaiiPhysicsReference UKawaiiPhysicsLibrary::SetExternalForceStructProperty(
	EKawaiiPhysicsAccessExternalForceResult& ExecResult, const FKawaiiPhysicsReference& KawaiiPhysics,
	int ExternalForceIndex, FName PropertyName, ValueType Value)
{
	ExecResult = EKawaiiPhysicsAccessExternalForceResult::NotValid;

	KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
		TEXT("SetExternalForceStructProperty"),
		[&ExecResult, &ExternalForceIndex, &PropertyName, &Value](FAnimNode_KawaiiPhysics& InKawaiiPhysics)
		{
			if (InKawaiiPhysics.ExternalForces.IsValidIndex(ExternalForceIndex) &&
				InKawaiiPhysics.ExternalForces[ExternalForceIndex].IsValid())
			{
				const auto* ScriptStruct = InKawaiiPhysics.ExternalForces[ExternalForceIndex].GetScriptStruct();
				auto& Force = InKawaiiPhysics.ExternalForces[ExternalForceIndex].GetMutable<
					FKawaiiPhysics_ExternalForce>();

				if (const FStructProperty* StructProperty = FindFProperty<FStructProperty>(
					ScriptStruct, PropertyName))
				{
					if (StructProperty->Struct == TBaseStructure<ValueType>::Get())
					{
						if (void* ValuePtr = StructProperty->ContainerPtrToValuePtr<uint8>(&Force))
						{
							StructProperty->CopyCompleteValue(ValuePtr, &Value);
							ExecResult = EKawaiiPhysicsAccessExternalForceResult::Valid;
						}
					}
				}
			}
		});

	return KawaiiPhysics;
}

template <typename ValueType>
ValueType UKawaiiPhysicsLibrary::GetExternalForceStructProperty(EKawaiiPhysicsAccessExternalForceResult& ExecResult,
                                                                const FKawaiiPhysicsReference& KawaiiPhysics,
                                                                int ExternalForceIndex, FName PropertyName)
{
	ValueType Result;
	ExecResult = EKawaiiPhysicsAccessExternalForceResult::NotValid;

	KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
		TEXT("GetExternalForceStructProperty"),
		[&Result, &ExecResult, &ExternalForceIndex, &PropertyName](FAnimNode_KawaiiPhysics& InKawaiiPhysics)
		{
			if (InKawaiiPhysics.ExternalForces.IsValidIndex(ExternalForceIndex) &&
				InKawaiiPhysics.ExternalForces[ExternalForceIndex].IsValid())
			{
				const auto* ScriptStruct = InKawaiiPhysics.ExternalForces[ExternalForceIndex].GetScriptStruct();
				const auto& Force = InKawaiiPhysics.ExternalForces[ExternalForceIndex].GetMutable<
					FKawaiiPhysics_ExternalForce>();

				if (const FStructProperty* StructProperty = FindFProperty<FStructProperty>(
					ScriptStruct, PropertyName))
				{
					if (StructProperty->Struct == TBaseStructure<ValueType>::Get())
					{
						Result = *(StructProperty->ContainerPtrToValuePtr<ValueType>(&Force));
						ExecResult = EKawaiiPhysicsAccessExternalForceResult::Valid;
					}
				}
			}
		});

	return Result;
}
