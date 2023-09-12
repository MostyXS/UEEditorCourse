// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetActions/QuickMaterialCreationWIdget.h"
#include "DebugHeader.h"
#include "EditorAssetLibrary.h"
#include "EditorUtilityLibrary.h"
#include "AssetToolsModule.h"
#include "Factories/MaterialFactoryNew.h"

#pragma region QuickMaterialCreationCore

void UQuickMaterialCreationWIdget::CreateMaterialFromSelectedTextures()
{
	if(bCustomMaterialName)
	{
		if(MaterialName.IsEmpty() || MaterialName.Equals("M_"))
		{
			DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("Please enter a valid name"));
			return;
		}
	}

	TArray<FAssetData> SelectedAssetsData = UEditorUtilityLibrary::GetSelectedAssetData();
	TArray<UTexture2D*> SelectedTexturesArray;
	FString SelectedTextureFolderPath;
	uint32 PinsConnectedCounter = 0 ;
	
	if(!ProcessSelectedData(SelectedAssetsData, SelectedTexturesArray, SelectedTextureFolderPath)) return;

	if(CheckIsNameUsed(SelectedTextureFolderPath, MaterialName)) return;

	UMaterial* CreatedMaterial = CreateMaterialAsset(MaterialName, SelectedTextureFolderPath);

	if(!CreatedMaterial)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("Failed to create material"));
	}
	DebugHeader::PrintLog(SelectedTextureFolderPath);

	for(UTexture2D* SelectedTexture : SelectedTexturesArray)
	{
		if (!SelectedTexture) continue;

		Default_CreateMaterialNodes(CreatedMaterial, SelectedTexture, PinsConnectedCounter);
		
	}
	
	
}

void UQuickMaterialCreationWIdget::InitializeSupportedTextureArrays()
{
	if(bBaseArraysAlreadyInitialized) return;


	BaseColorArray.Init({"zalupa", "zalupa"}, 2)
	//= new TArray<FString>(["_BaseColor", "_Albedo", "_Diffuse", "_diff"]);
	
	bBaseArraysAlreadyInitialized = true;
}

//Process the selected data, will filter out textures, and return false if non-texture selected
bool UQuickMaterialCreationWIdget::ProcessSelectedData(const TArray<FAssetData>& SelectedDataToProcess,
	TArray<UTexture2D*>& OutSelectedTexturesArray, FString& OutSelectedTexturePackagePath)
{
	if(SelectedDataToProcess.Num() == 0)
	{
		DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("No texture selected"));
		return false;
	}

	bool bMaterialNameSet = false;
	for (const FAssetData& SelectedData : SelectedDataToProcess)
	{
		UObject* SelectedAsset = SelectedData.GetAsset();

		if(!SelectedAsset) continue;

		UTexture2D* SelectedTexture = Cast<UTexture2D>(SelectedAsset);

		if(!SelectedTexture)
		{
			DebugHeader::ShowMsgDialog(EAppMsgType::Ok, TEXT("Please select only textures\n") +
				SelectedAsset->GetName() + TEXT(" is not a texture"));

			return false;
		}

		OutSelectedTexturesArray.Add(SelectedTexture);
		if(OutSelectedTexturePackagePath.IsEmpty())
		{
			OutSelectedTexturePackagePath = SelectedData.PackagePath.ToString();			
		}

		if(!bCustomMaterialName && !bMaterialNameSet)
		{
			MaterialName = SelectedAsset->GetName();
			MaterialName.RemoveFromStart(TEXT("T_"));
			MaterialName.InsertAt(0, TEXT("M_"));

			bMaterialNameSet = true;
		}
	}

	return true;
}

//Will return true if the material name is used by asset under the specified folder
bool UQuickMaterialCreationWIdget::CheckIsNameUsed(const FString& FolderPathToCheck, const FString& MaterialNameToCheck)
{
	TArray<FString> ExistingAssetPaths = UEditorAssetLibrary::ListAssets(FolderPathToCheck, false);

	for(const FString&  ExistingAssetPath:ExistingAssetPaths)
	{
		const FString ExistingAssetName =  FPaths::GetBaseFilename(ExistingAssetPath);

		if(ExistingAssetName.Equals(MaterialNameToCheck))
		{
			DebugHeader::ShowMsgDialog(EAppMsgType::Ok, MaterialNameToCheck + TEXT(" is already used by asset"));

			return true;
		}
	}

	return false;
}

UMaterial* UQuickMaterialCreationWIdget::CreateMaterialAsset(const FString& NameOfTheMaterial, const FString& PathToPutMaterial)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	
	UObject* CreatedObject = AssetToolsModule.Get().CreateAsset(NameOfTheMaterial, PathToPutMaterial,
		UMaterial::StaticClass(), MaterialFactory);

	return Cast<UMaterial>(CreatedObject);
}

void UQuickMaterialCreationWIdget::Default_CreateMaterialNodes(UMaterial* CreatedMaterial, UTexture2D* SelectedTexture,
	uint32& PinsConnectedCounter)
{
	UMaterialExpressionTextureSample* TextureSampleNode =
		NewObject<UMaterialExpressionTextureSample>(CreatedMaterial);

	if(!TextureSampleNode) return;

	if(CreatedMaterial->BaseColor.IsConnected())
	{
		if(TryConnectBaseColor(TextureSampleNode, SelectedTexture, CreatedMaterial))
		{
			PinsConnectedCounter++;
			return;
		}
	}
}

bool UQuickMaterialCreationWIdget::TryConnectBaseColor(UMaterialExpressionTextureSample* TextureSampleNode,
	UTexture2D* SelectedTexture, UMaterial* CreatedMaterial)
{
	
	for(const FString& BaseColorName:BaseColorArray)
	{
		if(SelectedTexture->GetName().Contains(BaseColorName))
		{
			//Connect pins to base color socket here
			TextureSampleNode->Texture = SelectedTexture;

			CreatedMaterial->Expressions.Add(TextureSampleNode);
			CreatedMaterial->BaseColor.Expression = TextureSampleNode;
			CreatedMaterial->PostEditChange();

			return true;
		}
	}
	return false;
}

#pragma endregion	
