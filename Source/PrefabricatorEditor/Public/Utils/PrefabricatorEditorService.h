//$ Copyright 2015-18, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Utils/PrefabricatorService.h"

class PREFABRICATOREDITOR_API FPrefabricatorEditorService : public IPrefabricatorService {
public:
	virtual void ParentActors(AActor* ParentActor, AActor* ChildActor) override;
	virtual void SelectPrefabActor(AActor* PrefabActor) override;
	virtual void GetSelectedActors(TArray<AActor*>& OutActors) override;
	virtual int GetNumSelectedActors() override;
	virtual UPrefabricatorAsset* CreatePrefabAsset() override;
	virtual void UpdateThumbnail(UPrefabricatorAsset* PrefabAsset) override;
	virtual FVector SnapToGrid(const FVector& InLocation) override;
};
