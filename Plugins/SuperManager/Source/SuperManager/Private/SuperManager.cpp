// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperManager.h"
#include "ContentBrowserModule.h"
#include "DebugHeader.h"
#include "EditorAssetLibrary.h"
#include "ObjectTools.h"
#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "SlateWidgets/AdvanceDeletionWidget.h"
#include "CustomStyle/SuperManagerStyle.h"

#define LOCTEXT_NAMESPACE "FSuperManagerModule"

void FSuperManagerModule::StartupModule()
{

	FSuperManagerStyle::InitializeIcons();
	InitCBMenuExtension();
	
	RegisterAdvanceDeletionTab();
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

#pragma region ContentBrowserMenuExtension

void FSuperManagerModule::InitCBMenuExtension()
{
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	//Caught myself on missing reference error. It was pretty neat.
	//Get hold of all the menu extenders
	TArray<FContentBrowserMenuExtender_SelectedPaths>& ContentBrowserModuleMenuExtenders =
		ContentBrowserModule.GetAllPathViewContextMenuExtenders();

	FContentBrowserMenuExtender_SelectedPaths CustomCBMenuDelegate;
	// CustomCBMenuDelegate.BindRaw(this, &FSuperManagerModule::CustomCBMenuExtender);
	//
	ContentBrowserModuleMenuExtenders.Add(CustomCBMenuDelegate);

	//We add custom delegate to all of the existing delegates
	ContentBrowserModuleMenuExtenders.Add(FContentBrowserMenuExtender_SelectedPaths::
		CreateRaw(this, &FSuperManagerModule::CustomCBMenuExtender));
	
}


//To define the position for inserting menu entry
TSharedRef<FExtender> FSuperManagerModule::CustomCBMenuExtender(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> MenuExtender (new FExtender());

	if(SelectedPaths.Num() > 0)
	{
		MenuExtender->AddMenuExtension(FName("Delete"), //Extension hook, position to insert
		EExtensionHook::After, //Inserting before or after
		TSharedPtr<FUICommandList>(), //Custom hot keys
		FMenuExtensionDelegate::CreateRaw(this, &FSuperManagerModule::AddCBMenuEntry)); //Second binding, will define details for this menu entry

		FolderPathsSelected = SelectedPaths;
	}
	return MenuExtender;
}


//Define details for the custom menu entry
void FSuperManagerModule::AddCBMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry
	(
		FText::FromString(TEXT("Delete Unused Assets")), //Title text for menu entry
		FText::FromString(TEXT("Safely delete all unused assets under folder")), //Tooltip text
		FSlateIcon(FSuperManagerStyle::GetStyleSetName(), "ContentBrowser.DeleteUnusedAsset"),	//Custom icon
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::OnDeleteUnusedAssetButtonClicked) //The actual function to execute
	);

	MenuBuilder.AddMenuEntry
	(
		FText::FromString(TEXT("Delete Empty Folders")),
		FText::FromString(TEXT("Safely delete all empty folders")),
		FSlateIcon(FSuperManagerStyle::GetStyleSetName(),"ContentBrowser.DeleteEmptyFolders"),
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::OnDeleteEmptyFoldersButtonClicked)
		
	);

	MenuBuilder.AddMenuEntry
	(
		FText::FromString(TEXT("Advance Deletion")),
		FText::FromString(TEXT("List assets by specific condition in a tab for deleting")),
		FSlateIcon(FSuperManagerStyle::GetStyleSetName(),"ContentBrowser.AdvanceDeletion"),
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::OnAdvanceDeletionButtonClicked)
		
	);
}

void FSuperManagerModule::OnDeleteUnusedAssetButtonClicked()
{
	if(FolderPathsSelected.Num() > 1)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok,TEXT("You can only do this to one folder"));
		return;
	}

	TArray<FString> AssetsPathNames = UEditorAssetLibrary::ListAssets(FolderPathsSelected[0]);

	//Whether there are assets under the folder
	if (AssetsPathNames.Num() == 0)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("No asset found under selected folder"), false);
		return;
	}
	EAppReturnType::Type ConfirmResult =
	DebugHeader::ShowMsgDialog(EAppMsgType::YesNo, TEXT("A total of ") + FString::FromInt(AssetsPathNames.Num()) + TEXT(" assets need to be checked.\nWould you like to procceed?"), false);

	if (ConfirmResult == EAppReturnType::No) return;

	FixUpRedirectors();
	
	TArray<FAssetData> UnusedAssetsDataArray;

	for(const FString& AssetPathName:AssetsPathNames)
	{
		//Don't touch root folder
		if (AssetPathName.Contains(TEXT("Developers"))||
		 AssetPathName.Contains(TEXT("Collections"))||
		 AssetPathName.Contains(TEXT("__ExternalActors__"))||
		 AssetPathName.Contains(TEXT("__ExternalObjects__")))
		{
			continue;
		}

		if(!UEditorAssetLibrary::DoesAssetExist(AssetPathName)) continue;

		TArray<FString> AssetReferencers =
		UEditorAssetLibrary::FindPackageReferencersForAsset(AssetPathName);

		if(AssetReferencers.Num() ==0)
		{
			const FAssetData UnusedAssetData = UEditorAssetLibrary::FindAssetData(AssetPathName);
			UnusedAssetsDataArray.Add(UnusedAssetData);
		}
		
	}

	if (UnusedAssetsDataArray.Num() > 0)
	{
		ObjectTools::DeleteAssets(UnusedAssetsDataArray);
	}
	else
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("No unused asset found under selected folder"), false);
	}

}

void FSuperManagerModule::OnDeleteEmptyFoldersButtonClicked()
{
	//What does the code do
	FixUpRedirectors();
	
	TArray<FString> FolderPathsArray = UEditorAssetLibrary::ListAssets(FolderPathsSelected[0], true, true);
	uint32 Counter = 0;

	FString EmptyFolderPathsNames;
	TArray<FString> EmptyFoldersPathsArray;

	for(const FString& FolderPath :  FolderPathsArray)
	{
		if (FolderPath.Contains(TEXT("Developers"))||
		 FolderPath.Contains(TEXT("Collections"))||
		 FolderPath.Contains(TEXT("__ExternalActors__"))||
		 FolderPath.Contains(TEXT("__ExternalObjects__")))
		{
			continue;
		}

		if(!UEditorAssetLibrary::DoesDirectoryExist(FolderPath)) continue;
			
		if(!UEditorAssetLibrary::DoesDirectoryHaveAssets(FolderPath))
		{
			EmptyFolderPathsNames.Append(FolderPath);
			EmptyFolderPathsNames.Append(TEXT("\n"));

			EmptyFoldersPathsArray.Add(FolderPath);
		}
	}

	if(EmptyFoldersPathsArray.Num()==0)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("No empty folder found under selected folder"), false);
		return;
	}

	EAppReturnType::Type ConfirmResult = DebugHeader::ShowMsgDialog(EAppMsgType::OkCancel,
		TEXT("Empty folders found in: \n") +EmptyFolderPathsNames + TEXT("\n Would you like to delete all?"), false);

	if(ConfirmResult == EAppReturnType::Cancel) return;

	for(const FString& EmptyFolderPath : EmptyFoldersPathsArray)
	{
		//Cast was neccessary because counter has returning uint32
		UEditorAssetLibrary::DeleteDirectory(EmptyFolderPath)?
		(void)++Counter : DebugHeader::Print(TEXT("Failed to delete " + EmptyFolderPath), FColor::Red);	
	}

	if(Counter > 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("Successfully deleted ") + FString::FromInt(Counter) + TEXT("folder"));
	}
}

void FSuperManagerModule::OnAdvanceDeletionButtonClicked()
{
	FixUpRedirectors();
	
	FGlobalTabmanager::Get()->TryInvokeTab(FName("AdvanceDeletion"));
}

void FSuperManagerModule::FixUpRedirectors()
{
	TArray<UObjectRedirector*> RedirectorsToFixArray;

	FAssetRegistryModule& AssetRegistryModule = 
	FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace("/Game");
	Filter.ClassNames.Emplace("ObjectRedirector");

	TArray<FAssetData> OutRedirectors;
	AssetRegistryModule.Get().GetAssets(Filter, OutRedirectors);

	for (const FAssetData& RedirectorData:OutRedirectors)
	{
		if(UObjectRedirector* RedirectorToFix = Cast<UObjectRedirector>(RedirectorData.GetAsset()))
		{
			RedirectorsToFixArray.Add(RedirectorToFix);
		}
	}

	FAssetToolsModule& AssetToolsModule =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	AssetToolsModule.Get().FixupReferencers(RedirectorsToFixArray);
	
}


#pragma endregion 

#pragma region CustomEditorTab

void FSuperManagerModule::RegisterAdvanceDeletionTab()
{

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FName("AdvanceDeletion"),
		FOnSpawnTab::CreateRaw(this, &FSuperManagerModule::OnSpawnAdvanceDeletionTab))
	.SetDisplayName(FText::FromString(TEXT("Advance Deletion")))
	.SetIcon(FSlateIcon(FSuperManagerStyle::GetStyleSetName(), "ContentBrowser.AdvanceDeletion"));
}

TSharedRef<SDockTab> FSuperManagerModule::OnSpawnAdvanceDeletionTab(const FSpawnTabArgs& SpawnTabArgs)
{
	if(FolderPathsSelected.Num() == 0) return SNew(SDockTab).TabRole(NomadTab);
	return
	SNew(SDockTab).TabRole(ETabRole::NomadTab)
	[
		SNew(SAdvanceDeletionTab)
		.AssetsDataToStore(GetAllAssetDataUnderSelectedFolder())
		//Somewhy it crash on compile because of array if we are not checking the folders num
		.CurrentSelectedFolder(FolderPathsSelected[0])
	];
}

TArray<TSharedPtr<FAssetData>> FSuperManagerModule::GetAllAssetDataUnderSelectedFolder()
{
	TArray<TSharedPtr<FAssetData>> AvailableAssetsData;

	if(FolderPathsSelected.Num() == 0) return AvailableAssetsData;
	
	TArray<FString> AssetsPathNames = UEditorAssetLibrary::ListAssets(FolderPathsSelected[0]);

	for(const FString& AssetPathName :  AssetsPathNames)
	{
		if (AssetPathName.Contains(TEXT("Developers"))||
		 AssetPathName.Contains(TEXT("Collections"))||
		 AssetPathName.Contains(TEXT("__ExternalActors__"))||
		 AssetPathName.Contains(TEXT("__ExternalObjects__")))
		{
			continue;
		}

		if(!UEditorAssetLibrary::DoesAssetExist(AssetPathName)) continue;

		const FAssetData Data = UEditorAssetLibrary::FindAssetData(AssetPathName);

		AvailableAssetsData.Add(MakeShared<FAssetData>(Data));
	}

	return AvailableAssetsData;
}




#pragma endregion

#pragma region ProcessDataForAdvanceDeletionTab

bool FSuperManagerModule::DeleteSingleAssetForAssetList(const FAssetData& AssetDataToDelete)
{
	TArray<FAssetData> AssetDataForDeletion;
	AssetDataForDeletion.Add(AssetDataToDelete);

	if(ObjectTools::DeleteAssets(AssetDataForDeletion)>0)
	{
		return true;
	}

	return false;
}

bool FSuperManagerModule::DeleteMultipleAssetsForAssetList(const TArray<FAssetData>& AssetsToDelete)
{
	if(ObjectTools::DeleteAssets(AssetsToDelete) > 0)
	{
		return true;
	}
	
	return false;
}

void FSuperManagerModule::ListUnusedAssetsForAssetList(const TArray<TSharedPtr<FAssetData>>& AssetsDataToFilter,
	TArray<TSharedPtr<FAssetData>>& OutUnusedAssetsData)
{

	OutUnusedAssetsData.Empty();
	
    for (const TSharedPtr<FAssetData>& DataSharedPtr : AssetsDataToFilter)
    {
    	TArray<FString> AssetReferencers =
        UEditorAssetLibrary::FindPackageReferencersForAsset(DataSharedPtr->ObjectPath.ToString());

    	if(AssetReferencers.Num() == 0)
    	{
    		OutUnusedAssetsData.Add(DataSharedPtr);
    	}
    }
}

void FSuperManagerModule::ListSameNameAssetsForAssetList(const TArray<TSharedPtr<FAssetData>>& AssetsDataToFilter,
	TArray<TSharedPtr<FAssetData>>& OutSameNameAssetsData)
{
	OutSameNameAssetsData.Empty();

	//Multimap for supporting finding assets with same name
	TMultiMap<FString, TSharedPtr<FAssetData>> AssetsInfoMultiMap;

	for(const TSharedPtr<FAssetData>& DataSharedPtr:AssetsDataToFilter)
	{
		
		//Eror was in string conversion
		AssetsInfoMultiMap.Emplace(DataSharedPtr->AssetName.ToString(), DataSharedPtr);
	}

	for(const TSharedPtr<FAssetData>& DataSharedPtr:AssetsDataToFilter)
	{
		TArray<TSharedPtr<FAssetData>> OutAssetsData;
		AssetsInfoMultiMap.MultiFind(DataSharedPtr->AssetName.ToString(), OutAssetsData);

		if(OutAssetsData.Num() <= 1) continue;

		for(TSharedPtr<FAssetData>& SameNameData : OutAssetsData)
		{
			if(SameNameData.IsValid())
			{
				OutSameNameAssetsData.AddUnique(SameNameData);
			}
			
		}
	}
}

void FSuperManagerModule::SyncCBToClickedAssetForAssetList(const FString& AssetPathToSync)
{
	TArray<FString> AssetsPathToSync;

	AssetsPathToSync.Add(AssetPathToSync);
	
	UEditorAssetLibrary::SyncBrowserToObjects(AssetsPathToSync);
}

#pragma endregion


void FSuperManagerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FName("AdvancedDeletion"));

	FSuperManagerStyle::ShutDown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuperManagerModule, SuperManager)