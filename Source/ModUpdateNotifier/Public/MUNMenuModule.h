// Copyright 2024 Jesse Hodgson.

#pragma once

#include "CoreMinimal.h"
#include "Module/MenuWorldModule.h"
#include "Util/SemVersion.h"
#include "HTTP.h"
#include "Blueprint/UserWidget.h"
#include "MUNMenuModule.generated.h"

/**
 * 
 */
UCLASS()
class MODUPDATENOTIFIER_API UMUNMenuModule : public UMenuWorldModule
{
	GENERATED_BODY()

public:

	UMUNMenuModule();

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> MenuWidgetClass;

	UPROPERTY(EditAnywhere)
	TArray<FString> ModFriendlyNames;

	UPROPERTY(EditAnywhere)
	TArray<FString> ModNames;

	UPROPERTY(EditAnywhere)
	TArray<FString> ModIDs;

	UPROPERTY(EditAnywhere)
	TArray<FVersion> ModVersions;

	UPROPERTY(EditAnywhere)
	TArray<FString> APIVersionStrings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString OutputList;

	UFUNCTION(BlueprintCallable, Category = "Mod Update Notifier")
	void Init();

	void OnResponseRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
