// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "AnimNode_KawaiiPhysics.h"
#include "ExternalForces/KawaiiPhysicsExternalForce.h"
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
 * Exposes operations to be performed on a blend space anim node.
 */
UCLASS()
class KAWAIIPHYSICS_API UKawaiiPhysicsLibrary : public UBlueprintFunctionLibrary
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

	/** Enable or disable physics simulation on this node */
	UFUNCTION(BlueprintCallable, Category = "Kawaii Physics|Runtime Control", meta=(BlueprintThreadSafe))
	static FKawaiiPhysicsReference SetPhysicsEnabled(const FKawaiiPhysicsReference& KawaiiPhysics,
	                                                 bool bEnabled)
	{
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("SetPhysicsEnabled"),
			[bEnabled](FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				InKawaiiPhysics.bAllowWorldCollision = bEnabled;
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

	// --- Physics State Serialization ---

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
	                                       int32& Spheres, int32& Capsules, int32& Boxes, int32& Planes)
	{
		Spheres = 0; Capsules = 0; Boxes = 0; Planes = 0;
		KawaiiPhysics.CallAnimNodeFunction<FAnimNode_KawaiiPhysics>(
			TEXT("GetCollisionPrimitiveCounts"),
			[&Spheres, &Capsules, &Boxes, &Planes](const FAnimNode_KawaiiPhysics& InKawaiiPhysics) {
				Spheres = InKawaiiPhysics.SphericalLimits.Num();
				Capsules = InKawaiiPhysics.CapsuleLimits.Num();
				Boxes = InKawaiiPhysics.BoxLimits.Num();
				Planes = InKawaiiPhysics.PlanarLimits.Num();
			});
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
