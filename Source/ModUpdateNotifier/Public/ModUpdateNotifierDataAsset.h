

#pragma once

#include "CoreMinimal.h"
#include "MUNMenuModule.h"
#include "Engine/DataAsset.h"
#include "ModUpdateNotifierDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class MODUPDATENOTIFIER_API UModUpdateNotifierDataAsset : public UDataAsset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FModUpdateNotifierInfo MUNInfo;
	
};
