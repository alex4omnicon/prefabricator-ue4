//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConstructionSystemComponent.generated.h"

class UPrefabricatorAssetInterface;
class APrefabActor;

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class CONSTRUCTIONSYSTEMRUNTIME_API UConstructionSystemComponent : public UActorComponent {
	GENERATED_BODY()
public:
	UConstructionSystemComponent();

	UFUNCTION(BlueprintCallable, Category = "ConstructionSystem")
	void EnableConstructionSystem();

	UFUNCTION(BlueprintCallable, Category = "ConstructionSystem")
	void DisableConstructionSystem();

	UFUNCTION(BlueprintCallable, Category = "ConstructionSystem")
	void SetActivePrefab(UPrefabricatorAssetInterface* InActivePrefabAsset);

	UFUNCTION(BlueprintCallable, Category = "ConstructionSystem")
	void ConstructAtCursor();

	//~ Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	//~ End UActorComponent Interface

private:
	APlayerController* GetPlayerController();
	void TransitionCameraTo(AActor* InViewTarget, float InBlendTime, float InBlendExp);
	void UpdateConstructionSystem();
	void RecreateCursorGhost();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConstructionSystem")
	float TraceDistance = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConstructionSystem")
	bool bRandomizedPlacement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
	UMaterialInterface* CursorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	AActor* ConstructionCameraActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ConstructionCameraTransitionTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ConstructionCameraTransitionExp = 1.0f;

private:
	bool bConstructionSystemEnabled = false;
	FRandomStream RandomStream;

	UPROPERTY(Transient)
	APrefabActor* CursorGhostActor;

	UPROPERTY(Transient)
	UPrefabricatorAssetInterface* ActivePrefabAsset;
};