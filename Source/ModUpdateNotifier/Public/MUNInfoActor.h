// Copyright 2024 Jesse Hodgson.

#pragma once

#include "CoreMinimal.h"
#include "MUNMenuModule.h"
#include "GameFramework/Actor.h"
#include "MUNInfoActor.generated.h"

UCLASS()
class MODUPDATENOTIFIER_API AMUNInfoActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMUNInfoActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FModUpdateNotifierInfo MUNInfo;

};