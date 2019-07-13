//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "ConstructionSystemTool.h"
#include "Components/InputComponent.h"
#include "ConstructionSystemBuildTool.generated.h"

class UPrefabricatorAssetInterface;
class UConstructionSystemCursor;

UCLASS(BlueprintType)
class CONSTRUCTIONSYSTEMRUNTIME_API UConstructionSystemBuildTool : public UConstructionSystemTool {
	GENERATED_BODY()
public:
	//~ Begin UConstructionSystemTool Interface
	virtual void InitializeTool(UConstructionSystemComponent* ConstructionComponent) override;
	virtual void DestroyTool(UConstructionSystemComponent* ConstructionComponent) override;
	virtual void OnToolEnable(UConstructionSystemComponent* ConstructionComponent) override;
	virtual void OnToolDisable(UConstructionSystemComponent* ConstructionComponent) override;
	virtual void Update(UConstructionSystemComponent* ConstructionComponent) override;
	//~ End UConstructionSystemTool Interface


	UFUNCTION(BlueprintCallable, Category = "ConstructionSystem")
	void SetActivePrefab(UPrefabricatorAssetInterface* InActivePrefabAsset);

	UFUNCTION()
	void ConstructAtCursor();

	UFUNCTION()
	void CursorMoveNext();

	UFUNCTION()
	void CursorMovePrev();

	UFUNCTION()
	void RotateCursorStep(float NumSteps);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerConstructAtCursor();

protected: 
	virtual void RegisterInputCallbacks(UInputComponent* InputComponent) override;
	virtual void UnregisterInputCallbacks(UInputComponent* InputComponent) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConstructionSystem")
	float TraceDistance = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ConstructionSystem")
	float CursorRotationStepAngle = 15.0f;

private:
	UPROPERTY(Transient)
	UConstructionSystemCursor* Cursor;

	UPROPERTY(Transient)
	UPrefabricatorAssetInterface* ActivePrefabAsset;

	int32 CursorRotationStep = 0;
	ECollisionChannel PrefabSnapChannel;
	bool bCursorFoundHit = false;
	bool bCursorModeFreeForm = true;


	struct FCSBuildToolInputBindings {
		FInputActionBinding BuildAtCursor;
		FInputActionBinding CursorItemNext;
		FInputActionBinding CursorItemPrev;
		FInputAxisBinding CursorRotate;
	};
	FCSBuildToolInputBindings InputBindings;
};
