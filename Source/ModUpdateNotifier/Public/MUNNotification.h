// Copyright 2024 Jesse Hodgson.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MUNNotification.generated.h"

/**
 * 
 */
UCLASS()
class MODUPDATENOTIFIER_API UMUNNotification : public UUserWidget
{
	GENERATED_BODY()

public:

	UMUNNotification();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString UpdateList;
};
