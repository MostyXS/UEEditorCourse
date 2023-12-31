// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "QuickMaterialCreationWIdget.generated.h"

/**
 * 
 */
UCLASS()
class SUPERMANAGER_API UQuickMaterialCreationWIdget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:

#pragma region QuickMaterialCreationCore

	UFUNCTION(BlueprintCallable)
	void CreateMaterialFromSelectedTextures();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =  "CreateMaterialFromSelectedTextures")
	bool bCustomMaterialName = true;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =  "CreateMaterialFromSelectedTextures", meta = (EditCondition = "bCustomMaterialName"))
	FString MaterialName = TEXT("M_");
	
#pragma endregion

#pragma	region SupportedTextureNames

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supported Texture Names")
	TArray<FString> BaseColorArray;
		

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supported Texture Names")
	//Finished up on this, couldn't resolve initialization error. Now on my own.
	TArray<FString> MetallicArray = TArray<FString>({
		TEXT("_Metallic"),
		TEXT("_metal")
	});
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supported Texture Names")
	TArray<FString> RoughnessArray = TArray<FString>({
		TEXT("_Roughness")
		TEXT("_RoughnessMap")
		TEXT("_rough")
	});
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supported Texture Names")
	TArray<FString> NormalArray =  TArray<FString>({
		TEXT("_Normal")
		TEXT("_NormalMap")
		TEXT("_nor")
	});
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Supported Texture Names")
	TArray<FString> AmbientOcclusionArray =  TArray<FString>({
		TEXT("_AmbientOcclusion")
		TEXT("_AmbientOcclusionMap")
		TEXT("_AO")
	});
	
#pragma endregion 	

private:

#pragma region QuickMaterialCreation

	bool bBaseArraysAlreadyInitialized = false;
	
	void InitializeSupportedTextureArrays();
	bool ProcessSelectedData(const TArray<FAssetData>& SelectedDataToProcess, TArray<UTexture2D*>& OutSelectedTexturesArray, FString& OutSelectedTexturePackagePath);
	bool CheckIsNameUsed(const FString& FolderPathToCheck, const FString& MaterialNameToCheck);
	UMaterial* CreateMaterialAsset(const FString& NameOfTheMaterial, const FString& PathToPutMaterial);
	void Default_CreateMaterialNodes(UMaterial* CreatedMaterial, UTexture2D* SelectedTexture, uint32& PinsConnectedCounter);
#pragma endregion

#pragma region 	CreateMaterialNodes

	bool TryConnectBaseColor(UMaterialExpressionTextureSample* TextureSampleNode, UTexture2D* SelectedTexture, UMaterial* CreatedMaterial);
	
#pragma endregion 	
	
};
