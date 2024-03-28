// Copyright 2024 Jesse Hodgson.

#include "MUNMenuModule.h"
#include "GameFramework/Actor.h"
#include "ModUpdateNotifier.h"
#include "Http.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetStringLibrary.h"
#include "ModLoading/ModLoadingLibrary.h"

UMUNMenuModule::UMUNMenuModule()
{
	// Initialize variables from config
	{
	#if WITH_EDITOR
	UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Shipping env not detected, avoiding crash"));
	#else
	MUNConfig = FModUpdateNotifier_ConfigStruct::GetActiveConfig(GetWorld());
	#endif
	}

	bDisableNotifications = MUNConfig.bDisableNotifications;

	MenuWidget = nullptr; // Initialize the widget pointer
}

void UMUNMenuModule::Init()
{
	// Get an instance of the ModLoadingLibrary to use for retrieving mod info
	const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	UModLoadingLibrary *ModLoadingLibrary = GameInstance->GetSubsystem<UModLoadingLibrary>();

	// Log metadata from ModUpdateNotifier for debug purposes
	UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Loaded ModUpdateNotifier Menu Module."));

	FModInfo MUNMetaInfo;
	ModLoadingLibrary->GetLoadedModInfo("ModUpdateNotifier", MUNMetaInfo);
	UE_LOG(LogModUpdateNotifier, Verbose, TEXT("%s"), *MUNMetaInfo.FriendlyName.Append(", " + MUNMetaInfo.Version.ToString()));
	UE_LOG(LogModUpdateNotifier, Display, TEXT("Build Date: %s %s"), ANSI_TO_TCHAR(__DATE__), ANSI_TO_TCHAR(__TIME__));

	// Add friendly names of all mods to an array
	ModFriendlyNames.Add("Better Grass");
	ModFriendlyNames.Add("SatisWHACKtory");
	ModFriendlyNames.Add("Remove All Annoyances");
	ModFriendlyNames.Add("Factory Props");
	ModFriendlyNames.Add("Discord Rich Presence");
	ModFriendlyNames.Add("2m Walls");
	ModFriendlyNames.Add("More Players");
	ModFriendlyNames.Add("Mod Update Notifier");

	// Add my mods to the array of names, will be used later for grabbing local data in place of the SMR ID. MUST BE IN THE SAME ORDER AS THE SMR IDS
	ModNames.Add("BetterGrass"); // Better Grass
	ModNames.Add("ObstacleMod"); // SatisWHACKtory
	ModNames.Add("RemoveAllAnnoyances"); // Remove All Annoyances
	ModNames.Add("Factory_Prop_Mod"); // Factory Props
	ModNames.Add("FG_DiscordRP"); // Discord Rich Presence
	ModNames.Add("TwoMeterWalls"); // 2m Walls
	ModNames.Add("MorePlayers"); // More Players
	ModNames.Add("ModUpdateNotifier");

	// Add my mods to the ID list
	ModIDs.Add("4S2xwMEFdMKymS"); // Better Grass
	ModIDs.Add("8XYLMRNbnfzc2G"); // SatisWHACKtory
	ModIDs.Add("FGnDVTV2ygmANY"); // Remove All Annoyances
	ModIDs.Add("8ivr6Mvuv4sCkX"); // Factory Props
	ModIDs.Add("2t2nCEBqMdUt1n"); // Discord Rich Presence
	ModIDs.Add("7NEYeWC3Mf5Rqa"); // 2m Walls
	ModIDs.Add("CMA7t3H6L1dkWT"); // More Players
	ModIDs.Add("8KzYMxowiUmKLn"); // Mod Update Notifier

	if (!bDisableNotifications)
	{
		// Create a ModInfo object to use for grabbing version data
		FModInfo ModInfo;

		// Get all loaded mod versions and put them into an array
		for(auto& CurrentModName : ModNames)
		{
			if(ModLoadingLibrary->IsModLoaded(CurrentModName))
			{
				ModLoadingLibrary->GetLoadedModInfo(CurrentModName, ModInfo);

				InstalledMods.Add(CurrentModName);
				ModVersions.Add(ModInfo.Version);
				// Initialize the API version array with nonsense before trying to write to it.
				APIVersionStrings.Add("Unfulfilled");
				InstalledModIDs.Add(ModIDs[ModNames.IndexOfByKey(CurrentModName)]);
				InstalledModFriendlyNames.Add(ModFriendlyNames[ModNames.IndexOfByKey(CurrentModName)]);

				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Added %s to mod version list."), *ModInfo.FriendlyName);
			}
		}

		// Loop through the array of mod IDs and create an HTTP GET request to the v1 SMR API to retrieve the latest versions of each mod
		for(auto& CurrentModID : InstalledModIDs)
		{
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Retrieving version data from API for %s"), *CurrentModID);

			// Create an HTTP GET request
			FHttpRequestRef Request =  FHttpModule::Get().CreateRequest();
			TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();
			Request->OnProcessRequestComplete().BindUObject(this, &UMUNMenuModule::OnResponseReceived);
			Request->SetURL("https://api.ficsit.app/v1/mod/" + CurrentModID + "/latest-versions");
			Request->SetVerb("GET");
			Request->ProcessRequest();
		}
	}
}

// Parse the HTTP response to extract the data we want: "mod_id" and "version"
void UMUNMenuModule::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Response Content (UMUNMenuModule): %s"), *Response->GetContentAsString());

	// Create our JSON object
	TSharedPtr<FJsonObject> ResponseObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	
	// Check if the response object is valid. This is useful if the API is down, or we don't have an internet connection. It can also prevent a crash.
	if (ResponseObj)
	{
		// Check if the response has a "data" field, if not, something is wrong and we should not continue
		if(ResponseObj->HasField("data"))
		{
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Response has field \"data\"."));

			// Create more objects to narrow down the field to a valid release, beta, or alpha version.
			const TSharedPtr<FJsonObject> DataObj = ResponseObj->GetObjectField("data");
			TSharedPtr<FJsonObject> ReleaseObj;

			// Check if there is a release of the mod available. If there is not, fall back to a beta or alpha release.
			if (DataObj->HasField("release"))
			{
				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Response has field \"release\"."));

				ReleaseObj = DataObj->GetObjectField("release");
			}
			else if (DataObj->HasField("beta"))
			{
				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Response does not contain a release. Response has field \"beta\", instead."));

				ReleaseObj = DataObj->GetObjectField("beta");

			}
			else if (DataObj->HasField("alpha"))
			{
				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Response does not contain a release or beta. Response has field \"alpha\", instead."));

				ReleaseObj = DataObj->GetObjectField("alpha");
			}
			else
			{
				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Response does not contain a release of any kind. Shutting down."));
			}

			// Put the response info into variables
			const FString ResponseID = ReleaseObj->GetStringField("mod_id");
			const FString ResponseVersion = ReleaseObj->GetStringField("version");

			// Add the version response to the API version array at the corresponding index of the mod ID for future retrieval
			APIVersionStrings[InstalledModIDs.Find(ResponseID)] = ResponseVersion;

			// DEBUG: Print out the mod ID and version from the API response
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Response version data for mod %s: %s"), *InstalledMods[InstalledModIDs.Find(ResponseID)], *ResponseVersion);

			// DEBUG: Print out the known version for the corresponding mod ID
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Known version: %s"), *ModVersions[InstalledModIDs.Find(ResponseID)].ToString());

			// DEBUG: Print out the API version array at this point in time
			for(auto& CurrentAPIVersionFromArray : APIVersionStrings)
			{
				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("API version (from array): %s"), *CurrentAPIVersionFromArray);
			}
		}
		else
		{
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Invalid reponse: no field \"data\" found."));
		}

		// If the API version list is complete, compare it to the known version list
		if(!APIVersionStrings.Contains("Unfulfilled"))
		{
			for (auto& CurrentAPIVersionString : APIVersionStrings)
			{
				// Initialize a variable that will be set to true if the mod is out of date
				bool IsModOutOfDate = false;

				// Create an index of the current version in the array that we are processing
				const int Index = APIVersionStrings.Find(CurrentAPIVersionString);
				UE_LOG (LogModUpdateNotifier, Verbose, TEXT("Index: %d"), Index);

				// Create variables for the major, minor, and patch version numbers from the API response to be compared against the known version
				FString MajorVersionOut;
				FString MinorVersionOut;
				FString PatchVersionOut;
				CurrentAPIVersionString.Split(".", &MajorVersionOut,&MinorVersionOut, ESearchCase::IgnoreCase, ESearchDir::FromStart);
				MinorVersionOut.Split(".", &MinorVersionOut, &PatchVersionOut, ESearchCase::IgnoreCase, ESearchDir::FromStart);

				UE_LOG(LogModUpdateNotifier, Verbose, TEXT("Merged version: %s.%s.%s"), *MajorVersionOut, *MinorVersionOut, *PatchVersionOut);

				// Check though the major, minor, and patch numbers from the API response sequentially to see if it is greater than the known version.
				if(UKismetStringLibrary::Conv_StringToInt(MajorVersionOut) > ModVersions[Index].Major)
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("There is a newer version of this mod available since the major version is lower than the retrieved one. %s"), *InstalledMods[Index]);

					IsModOutOfDate = true;
				}
				else if (UKismetStringLibrary::Conv_StringToInt(MinorVersionOut) > ModVersions[Index].Minor)
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("There is a newer version of this mod available since the minor version is lower than the retrieved one. %s"), *InstalledMods[Index]);

					IsModOutOfDate = true;
				}
				else if (UKismetStringLibrary::Conv_StringToInt(PatchVersionOut) > ModVersions[Index].Patch)
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("There is a newer version of this mod available since the patch version is lower than the retrieved one. %s"), *InstalledMods[Index]);

					IsModOutOfDate = true;
				}
				else
				{
					UE_LOG(LogModUpdateNotifier, Verbose, TEXT("The installed mod is up to date or newer than the available versions on SMR. %s"), *InstalledMods[Index]);
				}

				// If a mod has updates available and the list is not empty, add it to the list. Else, add it to the list on a new line
				if(OutputList.IsEmpty() && IsModOutOfDate == true)
				{
					OutputList = InstalledModFriendlyNames[Index] + " " + ModVersions[Index].ToString() + " -> " + APIVersionStrings[Index];
				}
				else if (IsModOutOfDate == true)
				{
					OutputList = OutputList + ",\n" + InstalledModFriendlyNames[Index] + " " + ModVersions[Index].ToString() + " -> " + APIVersionStrings[Index];
				}
			}
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("%s"), *OutputList);
		}
		// If there are out of date mods in the list, create the menu widget. Also check if we are running on a server and not display the menu widget.
		if (!OutputList.IsEmpty() && this->GetWorld()->GetNetMode() != NM_DedicatedServer)
		{
			// Create the menu widget, set it's desired size, and add it to the viewport
			MenuWidget = CreateWidget(this->GetWorld()->GetGameInstance()->GetFirstLocalPlayerController(), MenuWidgetClass, FName("MUNMenuWidget"));
			MenuWidget->SetDesiredSizeInViewport(FVector2D(400, 200));
			MenuWidget->AddToViewport();

			FinishedProcessing();
		}
		else
		{
			UE_LOG(LogModUpdateNotifier, Verbose, TEXT("All mods are up to date, not displaying a notification."));
		}
	}
}