//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "ConstructionSystemUtils.h"
#include "Engine/CollisionProfile.h"
#include "ConstructionSystemDefs.h"
#include "ConstructionSystemSnap.h"
#include "PrefabActor.h"

ECollisionChannel FConstructionSystemUtils::FindPrefabSnapChannel()
{
	// UCollisionProfile::ReturnContainerIndexFromChannelName is not exported with ENGINE_API so a less optimized way is used below
	UCollisionProfile* CollisionSettings = UCollisionProfile::Get();
	UEnum* Enum = StaticEnum<ECollisionChannel>();
	int32 NumEnums = Enum->NumEnums();
	for (int i = 0; i < NumEnums; i++) {
		FName ChannelName = CollisionSettings->ReturnChannelNameFromContainerIndex(i);
		if (ChannelName == FConstructionSystemConstants::PrefabSnapChannelName) {
			return (ECollisionChannel)Enum->GetValueByIndex(i);
		}
	}
	return ECollisionChannel::ECC_WorldStatic;
}

APrefabActor* FConstructionSystemUtils::FindTopMostPrefabActor(UPrefabricatorConstructionSnapComponent* SnapComponent)
{
	AActor* Owner = SnapComponent->GetOwner();
	APrefabActor* TopmostPrefab = nullptr;
	while (APrefabActor* PrefabActor = Cast<APrefabActor>(Owner->GetAttachParentActor())) {
		TopmostPrefab = PrefabActor;
		Owner = PrefabActor;
	}
	return TopmostPrefab;
}

bool FPCSnapUtils::GetSnapPoint(UPrefabricatorConstructionSnapComponent* Src, UPrefabricatorConstructionSnapComponent* Dst, 
		const FVector& InRequestedSnapLocation, FTransform& OutTargetSnapTransform, int32 CursorRotationStep, float InSnapTolerrance)
{
	FTransform SrcWorldTransform = Src->GetComponentTransform();
	FVector LocalCursorSnapPosition = SrcWorldTransform.InverseTransformPosition(InRequestedSnapLocation);

	FTransform DstWorldTransform = Dst->GetComponentTransform();
	{
		APrefabActor* TopmostPrefab = FConstructionSystemUtils::FindTopMostPrefabActor(Dst);
		if (TopmostPrefab) {
			DstWorldTransform = DstWorldTransform * TopmostPrefab->GetActorTransform().Inverse();
		}
	}


	if (Src->SnapType == EPrefabricatorConstructionSnapType::Wall && Dst->SnapType == EPrefabricatorConstructionSnapType::Wall) {
		bool bUseSrcXAxis, bUseDstXAxis;
		FVector SrcBoxExtent = Src->GetUnscaledBoxExtent();
		FVector DstBoxExtent = Dst->GetUnscaledBoxExtent();
		{
			FVector SrcScaledBoxExtent = Src->GetScaledBoxExtent();
			FVector DstScaledBoxExtent = Dst->GetScaledBoxExtent();
			bUseSrcXAxis = SrcScaledBoxExtent.X > SrcScaledBoxExtent.Y;
			bUseDstXAxis = DstScaledBoxExtent.X > DstScaledBoxExtent.Y;
		}

		FVector2D SrcHalfSize2D;
		SrcHalfSize2D.X = bUseSrcXAxis ? SrcBoxExtent.X : SrcBoxExtent.Y;
		SrcHalfSize2D.Y = SrcBoxExtent.Z;

		FVector2D DstHalfSize2D;
		DstHalfSize2D.X = bUseDstXAxis ? DstBoxExtent.X : DstBoxExtent.Y;
		DstHalfSize2D.Y = DstBoxExtent.Z;

		FVector2D Cursor2D;
		Cursor2D.X = bUseSrcXAxis ? LocalCursorSnapPosition.X : LocalCursorSnapPosition.Y;
		Cursor2D.Y = LocalCursorSnapPosition.Z;

		bool bCanApplyBaseRotations = false;

		// Top
		FVector2D BestSrcPos2D = FVector2D(0, SrcHalfSize2D.Y);
		float BestSrcSnapDistance = FMath::Abs(Cursor2D.Y - SrcHalfSize2D.Y);
		FVector2D BestDstPos2D = FVector2D(0, -DstHalfSize2D.Y);

		bool bFoundBest = Src->WallConstraint.AttachTop && Dst->WallConstraint.AttachBottom;

		// Bottom
		if (Src->WallConstraint.AttachBottom && Dst->WallConstraint.AttachTop) {
			float TestDistance = FMath::Abs(Cursor2D.Y + SrcHalfSize2D.Y);
			if (!bFoundBest || TestDistance < BestSrcSnapDistance) {
				BestSrcSnapDistance = TestDistance;
				BestSrcPos2D = FVector2D(0, -SrcHalfSize2D.Y);
				BestDstPos2D = FVector2D(0, DstHalfSize2D.Y);
				bFoundBest = true;
			}
		}

		// Right
		if (Src->WallConstraint.AttachRight && Dst->WallConstraint.AttachLeft) {
			float TestDistance = FMath::Abs(Cursor2D.X - SrcHalfSize2D.X);
			if (!bFoundBest || TestDistance < BestSrcSnapDistance) {
				BestSrcSnapDistance = TestDistance;
				BestSrcPos2D = FVector2D(SrcHalfSize2D.X, -SrcHalfSize2D.Y);
				BestDstPos2D = FVector2D(-DstHalfSize2D.X, -DstHalfSize2D.Y);
				bCanApplyBaseRotations = true;
				bFoundBest = true;
			}
		}

		// Left
		if (Src->WallConstraint.AttachLeft && Dst->WallConstraint.AttachRight) {
			float TestDistance = FMath::Abs(Cursor2D.X + SrcHalfSize2D.X);
			if (!bFoundBest || TestDistance < BestSrcSnapDistance) {
				BestSrcSnapDistance = TestDistance;
				BestSrcPos2D = FVector2D(-SrcHalfSize2D.X, -SrcHalfSize2D.Y);
				BestDstPos2D = FVector2D(DstHalfSize2D.X, -DstHalfSize2D.Y);
				bCanApplyBaseRotations = true;
				bFoundBest = true;
			}
		}

		if (!bFoundBest) {
			return false;
		}

		FVector BestLocalSrcSnapLocation;
		BestLocalSrcSnapLocation.Z = BestSrcPos2D.Y;
		BestLocalSrcSnapLocation.X = bUseSrcXAxis ? BestSrcPos2D.X : 0;
		BestLocalSrcSnapLocation.Y = bUseSrcXAxis ? 0 : BestSrcPos2D.X;

		FVector BestLocalDstSnapLocation;
		BestLocalDstSnapLocation.Z = BestDstPos2D.Y;
		BestLocalDstSnapLocation.X = bUseDstXAxis ? BestDstPos2D.X : 0;
		BestLocalDstSnapLocation.Y = bUseDstXAxis ? 0 : BestDstPos2D.X;

		FVector TargetSrcSnapLocation = SrcWorldTransform.TransformPosition(BestLocalSrcSnapLocation);
		FVector TargetDstSnapLocation = DstWorldTransform.TransformPosition(BestLocalDstSnapLocation);
		FQuat DstRotation = Src->GetComponentRotation().Quaternion();
		if (bCanApplyBaseRotations) {
			FVector UpVector = DstRotation.RotateVector(FVector::UpVector);
			const FQuat BaseRotations[] = {
				FQuat::Identity,
				FQuat(UpVector, PI * 0.5f),
				FQuat(UpVector, -PI * 0.5f)
			};
			FQuat BaseRotation = BaseRotations[FMath::Abs(CursorRotationStep) % 3];
			DstRotation = BaseRotation * DstRotation;
		}

		TargetDstSnapLocation = DstRotation.RotateVector(TargetDstSnapLocation);
		FVector DstOffset = TargetSrcSnapLocation - TargetDstSnapLocation;
		OutTargetSnapTransform = FTransform(DstRotation, DstOffset);
		return true;
	}

	else if (Src->SnapType == EPrefabricatorConstructionSnapType::Floor && Dst->SnapType == EPrefabricatorConstructionSnapType::Floor) {
		const FVector SrcExtent = Src->GetUnscaledBoxExtent();
		const FVector DstExtent = Dst->GetUnscaledBoxExtent();
		const FVector LCur = LocalCursorSnapPosition;


		static const FVector Deltas[] = {
			FVector(1, 0, 0),
			FVector(-1, 0, 0),
			FVector(0, 1, 0),
			FVector(0, -1, 0),
			FVector(0, 0, 1),
			FVector(0, 0, -1)
		};
		static const FVector SrcSnapGuides[] = {
			FVector(1, 0, 1),
			FVector(-1, 0, 1),
			FVector(0, 1, 1),
			FVector(0, -1, 1),
			FVector(0, 0, 1),
			FVector(0, 0, -1)
		};
		static const FVector DstSnapGuides[] = {
			FVector(-1, 0, 1),
			FVector(1, 0, 1),
			FVector(0, -1, 1),
			FVector(0, 1, 1),
			FVector(0, 0, -1),
			FVector(0, 0, 1)
		};

		FVector BestLSrcPos = FVector::ZeroVector;
		FVector BestLDstPos = FVector::ZeroVector;
		float BestDist = MAX_flt;

		bool SrcAttachState[] = {
			Src->FloorConstraint.AttachX, Src->FloorConstraint.AttachXNegative,
			Src->FloorConstraint.AttachY, Src->FloorConstraint.AttachYNegative,
			Src->FloorConstraint.AttachZ, Src->FloorConstraint.AttachZNegative
		};

		bool DstAttachState[] = {
			Dst->FloorConstraint.AttachXNegative, Dst->FloorConstraint.AttachX,
			Dst->FloorConstraint.AttachYNegative, Dst->FloorConstraint.AttachY,
			Dst->FloorConstraint.AttachZNegative, Dst->FloorConstraint.AttachZ
		};

		const float HorizontalStackingSensorRadius = FVector2D(SrcExtent.X, SrcExtent.Y).Size() * 0.3f;
		bool bFoundBest = false;
		for (int i = 0; i < 6; i++) {
			if (!SrcAttachState[i] || !DstAttachState[i]) {
				continue;
			}

			const FVector& D = Deltas[i];
			float Dist = FMath::Abs(FVector::DotProduct(LCur - SrcExtent * D, D));
			bool bIsBest = false;
			if (Dist < BestDist) {
				// On top and bottom surfaces, only attach if we are within the radius. other attach to sides (by rejecting this)
				if (i == 4 || i == 5) {
					float DistanceFromHCenter = (LCur * FVector(1, 1, 0)).Size();
					if (DistanceFromHCenter < HorizontalStackingSensorRadius) {
						bIsBest = true;
					}
				}
				else {
					bIsBest = true;
				}
			}

			if (bIsBest) {
				BestLSrcPos = SrcSnapGuides[i] * SrcExtent;
				BestLDstPos = DstSnapGuides[i] * DstExtent;
				BestDist = Dist;
				bFoundBest = true;
			}
		}

		if (!bFoundBest) {
			return false;
		}

		FVector TargetSrcSnapLocation = SrcWorldTransform.TransformPosition(BestLSrcPos);
		FVector TargetDstSnapLocation = DstWorldTransform.TransformPosition(BestLDstPos);
		FQuat DstRotation = Src->GetComponentRotation().Quaternion();

		// Apply cursor rotation
		{
			FVector UpVector = DstRotation.RotateVector(FVector::UpVector);
			const FQuat BaseRotations[] = {
				FQuat::Identity,
				FQuat(UpVector, PI * 0.5f),
				FQuat(UpVector, PI * 1.5f),
				FQuat(UpVector, PI * 1.5f)
			};
			FQuat BaseRotation = BaseRotations[FMath::Abs(CursorRotationStep) % 4];

			TargetDstSnapLocation = DstRotation.RotateVector(TargetDstSnapLocation);
			DstRotation = BaseRotation * DstRotation;
		}

		FVector DstOffset = TargetSrcSnapLocation - TargetDstSnapLocation;
		OutTargetSnapTransform = FTransform(DstRotation, DstOffset);
		return true;
	}

	else if (Src->SnapType == EPrefabricatorConstructionSnapType::Floor && Dst->SnapType == EPrefabricatorConstructionSnapType::Wall) {
		const FVector SrcExtent = Src->GetUnscaledBoxExtent();
		const FVector DstExtent = Dst->GetUnscaledBoxExtent();
		const FVector LCur = LocalCursorSnapPosition;

		bool bUseDstXAxis;
		{
			FVector DstScaledBoxExtent = Dst->GetScaledBoxExtent();
			bUseDstXAxis = DstScaledBoxExtent.X > DstScaledBoxExtent.Y;
		}

		FVector2D DstHalfSize2D;
		DstHalfSize2D.X = bUseDstXAxis ? DstExtent.X : DstExtent.Y;
		DstHalfSize2D.Y = DstExtent.Z;


		static const FVector Deltas[] = {
			FVector(1, 0, 0),
			FVector(-1, 0, 0),
			FVector(0, 1, 0),
			FVector(0, -1, 0)
		};
		static const FVector SnapPoints[] = {
			FVector(1, 0, 1),
			FVector(-1, 0, 1),
			FVector(0, 1, 1),
			FVector(0, -1, 1)
		};

		const bool SrcConstraints[] = {
			Src->FloorConstraint.AttachX,
			Src->FloorConstraint.AttachXNegative,
			Src->FloorConstraint.AttachY,
			Src->FloorConstraint.AttachYNegative
		};

		static const FQuat SnapRotations[] = {
			FQuat(FVector::UpVector, PI * 0.5f),
			FQuat(FVector::UpVector, PI * 1.5f),
			FQuat(FVector::UpVector, PI * 1.0f),
			FQuat(FVector::UpVector, PI * 0.0f)
		};

		FVector BestLSrcPos = FVector::ZeroVector;
		FVector2D BestLDstPos2D = FVector2D(0, -DstHalfSize2D.Y);
		float BestDist = MAX_flt;
		FQuat BestLDstRot = FQuat::Identity;

		bool bFoundBest = false;
		for (int i = 0; i < 4; i++) {
			if (!SrcConstraints[i]) continue;

			const FVector& D = Deltas[i];
			float Dist = FMath::Abs(FVector::DotProduct(LCur - SrcExtent * D, D));
			if (Dist < BestDist) {
				const FVector& SnapPointMult = SnapPoints[i];
				BestLSrcPos = SnapPointMult * SrcExtent;
				BestLDstRot = SnapRotations[i];
				BestDist = Dist;
				bFoundBest = true;
			}
		}

		if (!bFoundBest) {
			return false;
		}

		FVector BestLDstPos;
		BestLDstPos.Z = BestLDstPos2D.Y;
		BestLDstPos.X = bUseDstXAxis ? BestLDstPos2D.X : 0;
		BestLDstPos.Y = bUseDstXAxis ? 0 : BestLDstPos2D.X;
		
		if (LCur.Z < 0) {
			BestLDstPos.Z = -BestLDstPos.Z;
		}

		if (LCur.Z > 0 && !Dst->WallConstraint.AttachBottom) {
			return false;
		}
		else if (LCur.Z <= 0 && !Dst->WallConstraint.AttachTop) {
			return false;
		}

		FVector TargetSrcSnapLocation = SrcWorldTransform.TransformPosition(BestLSrcPos);
		FVector TargetDstSnapLocation = DstWorldTransform.TransformPosition(BestLDstPos);
		FQuat DstRotation = BestLDstRot * Src->GetComponentRotation().Quaternion();

		TargetDstSnapLocation = DstRotation.RotateVector(TargetDstSnapLocation);
		FVector DstOffset = TargetSrcSnapLocation - TargetDstSnapLocation;
		OutTargetSnapTransform = FTransform(DstRotation, DstOffset);
		return true;
	}


	else if (Src->SnapType == EPrefabricatorConstructionSnapType::Wall && Dst->SnapType == EPrefabricatorConstructionSnapType::Floor) {
		bool bUseSrcXAxis;
		FVector SrcBoxExtent = Src->GetUnscaledBoxExtent();
		FVector DstBoxExtent = Dst->GetUnscaledBoxExtent();
		{
			FVector SrcScaledBoxExtent = Src->GetScaledBoxExtent();
			bUseSrcXAxis = SrcScaledBoxExtent.X > SrcScaledBoxExtent.Y;
		}

		FVector2D SrcHalfSize2D;
		SrcHalfSize2D.X = bUseSrcXAxis ? SrcBoxExtent.X : SrcBoxExtent.Y;
		SrcHalfSize2D.Y = SrcBoxExtent.Z;

		FVector2D Cursor2D;
		Cursor2D.X = bUseSrcXAxis ? LocalCursorSnapPosition.X : LocalCursorSnapPosition.Y;
		Cursor2D.Y = LocalCursorSnapPosition.Z;


		bool bFoundBest = false;

		// Top
		FVector2D BestSrcPos2D = FVector2D(0, SrcHalfSize2D.Y);
		float BestSrcSnapDistance = FMath::Abs(Cursor2D.Y - SrcHalfSize2D.Y);
		FVector BestDstPos = FVector(0, -DstBoxExtent.Y, DstBoxExtent.Z);
		bFoundBest = Src->WallConstraint.AttachTop;

		// Bottom
		float TestDistance = FMath::Abs(Cursor2D.Y + SrcHalfSize2D.Y);
		if (TestDistance < BestSrcSnapDistance) {
			BestSrcSnapDistance = TestDistance;
			BestSrcPos2D = FVector2D(0, -SrcHalfSize2D.Y);
			BestDstPos = FVector(0, -DstBoxExtent.Y, DstBoxExtent.Z);
			bFoundBest = true;
		}

		if (!bFoundBest) {
			return false;
		}

		// TODO: Allow rotation and check floor constraint

		FVector BestSrcPos;
		BestSrcPos.Z = BestSrcPos2D.Y;
		BestSrcPos.X = bUseSrcXAxis ? BestSrcPos2D.X : 0;
		BestSrcPos.Y = bUseSrcXAxis ? 0 : BestSrcPos2D.X;

		if (bUseSrcXAxis && LocalCursorSnapPosition.Y < 0) {
			BestDstPos.Y = -BestDstPos.Y;
		}
		else if (!bUseSrcXAxis && LocalCursorSnapPosition.X < 0) {
			BestDstPos.Y = -BestDstPos.Y;
		}

		FVector TargetSrcSnapLocation = SrcWorldTransform.TransformPosition(BestSrcPos);
		FVector TargetDstSnapLocation = DstWorldTransform.TransformPosition(BestDstPos);
		FQuat DstRotation = Src->GetComponentRotation().Quaternion();

		const bool bCanApplyBaseRotations = true;
		if (bCanApplyBaseRotations) {
			FVector UpVector = DstRotation.RotateVector(FVector::UpVector);
			const FQuat BaseRotations[] = {
				FQuat::Identity,
				FQuat(UpVector, PI * 0.5f),
				FQuat(UpVector, -PI * 0.5f)
			};
			FQuat BaseRotation = BaseRotations[FMath::Abs(CursorRotationStep) % 3];
			DstRotation = BaseRotation * DstRotation;
		}

		TargetDstSnapLocation = DstRotation.RotateVector(TargetDstSnapLocation);
		FVector DstOffset = TargetSrcSnapLocation - TargetDstSnapLocation;
		OutTargetSnapTransform = FTransform(DstRotation, DstOffset);
		return true;
	}


	return false;
}