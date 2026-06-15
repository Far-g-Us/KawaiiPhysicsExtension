// Copyright 2019-2026 pafuhana1213. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoneContainer.h"
#include "KawaiiPhysicsTypes.generated.h"

UENUM(BlueprintType)
enum class EKawaiiPhysicsSimulationSpace : uint8
{
	/** Simulate in component space */
	ComponentSpace,
	/** Simulate in world space. This fixes the issues of root bones moving suddenly */
	WorldSpace,
	/** Simulate in another bone space */
	BaseBoneSpace,
};

/**
 * Detected pose for pose-based physics blending.
 */
UENUM(BlueprintType)
enum class EKawaiiPhysicsDetectedPose : uint8
{
	/** Standing pose (normal gravity) */
	Standing,
	/** Sitting pose (reduced gravity, more stable) */
	Sitting,
	/** Lying pose (horizontal, gravity pulls sideways) */
	Lying,
	/** Custom pose (user-defined) */
	Custom,
};

/**
 * Enum representing the planar constraint axis in KawaiiPhysics.
 */
UENUM(meta=(ScriptName = "KP_PlanarConstraint"))
enum class EPlanarConstraint : uint8
{
	/** No planar constraint */
	None,
	/** Constrain to the X axis */
	X,
	/** Constrain to the Y axis */
	Y,
	/** Constrain to the Z axis */
	Z,
};

/**
 * Enum representing the forward axis of a bone in KawaiiPhysics.
 */
UENUM()
enum class EBoneForwardAxis : uint8
{
	X_Positive,
	X_Negative,
	Y_Positive,
	Y_Negative,
	Z_Positive,
	Z_Negative,
};

/**
 * Structure representing the root bone settings for KawaiiPhysics.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysicsRootBoneSetting
{
	GENERATED_BODY()

	/** 
	* 指定ボーンとそれ以下のボーンを制御対象に
	* Control the specified bone and the bones below it
	*/
	UPROPERTY(EditAnywhere, Category = "Bones")
	FBoneReference RootBone;

	/** 
	* 指定したボーンとそれ以下のボーンを制御対象から除去
	* Do NOT control the specified bone and the bones below it
	*/
	UPROPERTY(EditAnywhere, Category = "Bones", meta = (EditCondition = "bUseOverrideExcludeBones"))
	TArray<FBoneReference> OverrideExcludeBones;
	UPROPERTY(EditAnywhere, Category = "Bones", meta = (InlineEditConditionToggle))
	bool bUseOverrideExcludeBones = false;
};


/**
 * Structure representing the settings for KawaiiPhysics.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysicsSettings
{
	GENERATED_BODY()

	/** 
	* 減衰度：揺れの強さを制御。値が小さいほど、加速度を物理挙動に反映
	* Damping physical behavior. As the value is smaller, the acceleration is more reflected to the physical behavior
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"), category = "KawaiiPhysics")
	float Damping = 0.1f;

	/** 
	* 剛性度：値が大きいほど、元の形状を維持
	* Stiffness of physical behavior.As the value is larger, pre-physics shape is more respected
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"), category = "KawaiiPhysics")
	float Stiffness = 0.05f;

	/**
	* ワールド座標系における Skeletal Mesh Component の移動量のダンピング(抑制)量。
	* 0 = 移動量をフル反映(揺れ最大) / 1 = コンポーネントに追従(反映なし)。実際の反映率 = (1 - WorldDampingLocation)
	* Damping (suppression) of the Skeletal Mesh Component's world-space movement.
	* 0 = movement fully reflected (max sway) / 1 = follows component (no reflection). Actual reflection factor = (1 - WorldDampingLocation).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"), category = "KawaiiPhysics")
	float WorldDampingLocation = 0.8f;

	/**
	* ワールド座標系における Skeletal Mesh Component の回転量のダンピング(抑制)量。
	* 0 = 回転量をフル反映(揺れ最大) / 1 = コンポーネントに追従(反映なし)。実際の反映率 = (1 - WorldDampingRotation)
	* Damping (suppression) of the Skeletal Mesh Component's world-space rotation.
	* 0 = rotation fully reflected (max sway) / 1 = follows component (no reflection). Actual reflection factor = (1 - WorldDampingRotation).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"), category = "KawaiiPhysics")
	float WorldDampingRotation = 0.8f;

	/** 
	* 各ボーンのコリジョン半径
	* Radius of bone's collision
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", DisplayName="Collision Radius"),
		category = "KawaiiPhysics")
	float Radius = 3.0f;

	/** 
	* 物理挙動による回転制限。適切に設定することで荒ぶりを抑制
	* Rotational limitations in physical behavior. Setting the value properly can prevent rampage
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"), category = "KawaiiPhysics")
	float LimitAngle = 0.0f;

	/**
	* Twist/Swing制限を有効にするフラグ。有効にするとLimitAngleは無視され、TwistLimitAngle/SwingLimitYAngle/SwingLimitZAngleが使用される
	* Enable Twist/Swing limits. When enabled, LimitAngle is ignored and TwistLimitAngle/SwingLimitYAngle/SwingLimitZAngle are used instead.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KawaiiPhysics")
	bool bUseTwistSwingLimits = false;

	/**
	* Twist回転（BoneForwardAxis周り）の制限角度
	* Twist rotation limit angle (around the bone forward axis)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", EditCondition = "bUseTwistSwingLimits"), category = "KawaiiPhysics")
	float TwistLimitAngle = 0.0f;

	/**
	* Swing回転Y軸方向の制限角度
	* Swing rotation limit angle (Y-axis component)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", EditCondition = "bUseTwistSwingLimits"), category = "KawaiiPhysics")
	float SwingLimitYAngle = 0.0f;

	/**
	* Swing回転Z軸方向の制限角度
	* Swing rotation limit angle (Z-axis component)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", EditCondition = "bUseTwistSwingLimits"), category = "KawaiiPhysics")
	float SwingLimitZAngle = 0.0f;

	/**
	* Volume Preservation: attempts to maintain the volume of the bone chain by adjusting bone lengths.
	* Useful for breast/buttock/belly physics. 0 = disabled, 1 = strong preservation.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"), category = "KawaiiPhysics|VolumePreservation")
	float VolumePreservation = 0.0f;

	/**
	* Pressure simulation: when volume is compressed (e.g. collision), bones push outward.
	* Works with VolumePreservation. 0 = no pressure, high = strong outward force.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "5", EditCondition = "VolumePreservation > 0"), category = "KawaiiPhysics|VolumePreservation")
	float Pressure = 0.0f;

	/**
	* Bone Scaling: automatically scale bone when it moves.
	* Creates visual bulging/compression effect like UBE 2.0.
	* X axis = scale when compressed, Y axis = scale when stretched.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "2"), category = "KawaiiPhysics|BoneScaling")
	float BoneScaleCompensation = 0.0f;

	/**
	* Maximum bone scale factor (prevents extreme scaling).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1", ClampMax = "3.0", EditCondition = "BoneScaleCompensation > 0"), category = "KawaiiPhysics|BoneScaling")
	float MaxBoneScale = 1.5f;

	/**
	* Axis for bone scaling: 0 = uniform, 1 = X-axis only, 2 = Y-axis, 3 = Z-axis.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "BoneScaleCompensation > 0"), category = "KawaiiPhysics|BoneScaling")
	int32 BoneScaleAxis = 0;
};

/**
 * Structure representing a bone that can be modified by the KawaiiPhysics system.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysicsModifyBone
{
	GENERATED_USTRUCT_BODY()

	/** Reference to the bone */
	UPROPERTY()
	FBoneReference BoneRef;

	/** Index of the bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	int32 Index = -1;

	/** Index of the parent bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	int32 ParentIndex = -1;

	/** Indices of the child bones */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	TArray<int32> ChildIndices;

	/** Physics settings for the bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	FKawaiiPhysicsSettings PhysicsSettings;

	/** Current location of the bone */
	UPROPERTY(BlueprintReadWrite, Category = "KawaiiPhysics|ModifyBone")
	FVector Location = FVector::ZeroVector;

	/** Previous location of the bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	FVector PrevLocation = FVector::ZeroVector;

	/** Previous rotation of the bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	FQuat PrevRotation = FQuat::Identity;

	/** Bone scale for visual bulging/compression (calculated by ApplyBoneScaling) */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	FVector BoneScale = FVector(1.0f);

	/** Original damping value (stored for pose blending) */
	UPROPERTY()
	float OriginalDamping = 0.0f;

	/** Original stiffness value (stored for pose blending) */
	UPROPERTY()
	float OriginalStiffness = 0.0f;

	/** Pose location of the bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	FVector PoseLocation = FVector::ZeroVector;

	/** Pose rotation of the bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	FQuat PoseRotation = FQuat::Identity;

	// ===== 固定サブステップのポーズ補間用（Transient, 非シリアライズ） =====
	// For fixed-substep pose interpolation (Transient, not serialized)
	/** 前フレームのポーズ目標位置（補間の始点） / Previous-frame pose target location (lerp start) */
	FVector PrevPoseLocation = FVector::ZeroVector;
	/** 前フレームのポーズ目標回転 / Previous-frame pose target rotation */
	FQuat PrevPoseRotation = FQuat::Identity;
	/** 現フレームのポーズ目標位置のスナップショット（補間の終点）。サブステップ中 PoseLocation を上書きするため退避 */
	/** Snapshot of this frame's pose target location (lerp end); stashed because PoseLocation is overwritten during substeps */
	FVector CurrentPoseLocation = FVector::ZeroVector;
	/** 現フレームのポーズ目標回転のスナップショット / Snapshot of this frame's pose target rotation */
	FQuat CurrentPoseRotation = FQuat::Identity;

	/** Pose scale of the bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	FVector PoseScale = FVector::OneVector;

	/** Length of the bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	float BoneLength = 0.0f;

	/** Length from the root bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	float LengthFromRoot = 0.0f;

	/** Length rate from the root bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	float LengthRateFromRoot = 0.0f;

	/** Flag indicating if this is a dummy bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	bool bDummy = false;

	// ===== Twist/Swing Limits (runtime) =====
	/** Twist limit angle (runtime, from curve evaluation) */
	float TwistLimitAngle = 0.0f;
	/** Swing limit Y angle (runtime, from curve evaluation) */
	float SwingLimitYAngle = 0.0f;
	/** Swing limit Z angle (runtime, from curve evaluation) */
	float SwingLimitZAngle = 0.0f;

	/**
	* ボーン間ダミーボーンフラグ（2つの実ボーン間に挿入されたダミー）
	* Flag: this is an inter-bone dummy (inserted between two real bones)
	*/
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	bool bInterBoneDummy = false;

	/**
	* ボーン間ダミーの補間先（実子ボーン）のインデックス / Real child bone index for PoseLocation interpolation
	*/
	UPROPERTY()
	int32 InterBoneRealChildIndex = -1;

	/**
	* ボーン間ダミーの補間元（実親ボーン）のインデックス / Real parent bone index for PoseLocation interpolation
	*/
	UPROPERTY()
	int32 InterBoneRealParentIndex = -1;

	/**
	* ボーン間ダミーの補間アルファ（0.0=実親, 1.0=実子）/ Interpolation alpha between real parent and child
	*/
	UPROPERTY()
	float InterBoneAlpha = 0.0f;

	/**
	* 横方向BoneConstraint上に挿入されたコリジョン代理ダミー（縦階層に属さず、常にコリジョン専用）
	* Collision-proxy dummy inserted along a horizontal BoneConstraint (not in the vertical hierarchy; always collision-only).
	* 配置用に InterBoneRealParentIndex/InterBoneRealChildIndex/InterBoneAlpha を端点1/端点2/補間率として流用する。
	* Reuses InterBoneRealParentIndex/InterBoneRealChildIndex/InterBoneAlpha as endpoint1/endpoint2/lerp-alpha for placement.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	bool bBridgeDummy = false;

	/** Flag indicating if simulation should be skipped for this bone */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|ModifyBone")
	bool bSkipSimulate = false;

	/**
	 * Checks if the bone has a parent.
	 *
	 * @return True if the bone has a parent, false otherwise.
	 */
	bool HasParent() const { return ParentIndex >= 0; }

	/** Default constructor */
	FKawaiiPhysicsModifyBone()
	{
	}
};

/**
 * Structure representing a bone collision event.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysicsBoneCollisionEvent
{
	GENERATED_BODY()

	/** Name of the bone that collided */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|CollisionEvent")
	FName BoneName = NAME_None;

	/** World-space point of collision */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|CollisionEvent")
	FVector CollisionPoint = FVector::ZeroVector;

	/** World-space normal of collision */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|CollisionEvent")
	FVector CollisionNormal = FVector::ZeroVector;

	/** Time when the collision occurred */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|CollisionEvent")
	float TimeStamp = 0.0f;
};

/**
 * Saved physics state for serialization/deserialization.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysicsSavedState
{
	GENERATED_BODY()

	/** Bone names and their saved locations/prev locations */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|SavedState")
	TMap<FName, FVector> BoneLocations;

	/** Previous locations for Verlet integration continuity */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|SavedState")
	TMap<FName, FVector> BonePrevLocations;

	/** Whether this state is valid */
	UPROPERTY(BlueprintReadOnly, Category = "KawaiiPhysics|SavedState")
	bool bIsValid = false;
};

/**
 * LOD-specific settings for KawaiiPhysics.
 */
USTRUCT(BlueprintType)
struct KAWAIIPHYSICS_API FKawaiiPhysicsLODSettings
{
	GENERATED_BODY()

	/** Whether to enable physics simulation at this LOD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	bool bEnabled = true;

	/** Damping override (0 = use global setting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DampingOverride = 0.0f;

	/** Stiffness override (0 = use global setting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StiffnessOverride = 0.0f;

	/** LimitAngle override (0 = use global setting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "0.0"))
	float LimitAngleOverride = 0.0f;

	/** Whether to enable world collision at this LOD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	bool bAllowWorldCollision = false;

	/** Whether to enable bone constraints at this LOD */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	bool bAllowBoneConstraints = true;

	/** Bone constraint iteration count override (0 = use global setting) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "0", ClampMax = "10"))
	int32 BoneConstraintIterationsOverride = 0;
};
