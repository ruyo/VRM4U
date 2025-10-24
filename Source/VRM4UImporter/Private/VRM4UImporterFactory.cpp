// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VRM4UImporterFactory.h"
#include "VRM4UImporterLog.h"

#include "Misc/EngineVersionComparison.h"

#include "AssetToolsModule.h"
#if	UE_VERSION_OLDER_THAN(4,26,0)
#include "AssetRegistryModule.h"
#else
#include "AssetRegistry/AssetRegistryModule.h"
#endif
#include "PackageTools.h"
#include "Misc/Paths.h"
#include "Engine/SkeletalMesh.h"
#include "EditorFramework/AssetImportData.h"
#include "Components/SkeletalMeshComponent.h"
//#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "LoaderBPFunctionLibrary.h"
#include "VrmAssetListObject.h"
#include "VrmRuntimeSettings.h"
#include "VrmOptionWindow.h"
#include "Engine/Blueprint.h"
#include "Templates/SharedPointer.h"
#include "Misc/FeedbackContext.h"
#include "FileHelpers.h"
#include "TickableEditorObject.h"

#include "Interfaces/IMainFrameModule.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "VrmOptionWindow.h"
#include "VrmImportUI.h"
#include "VrmConvert.h"
#include "VrmLicenseObject.h"
#include "Vrm1LicenseObject.h"

#define LOCTEXT_NAMESPACE "VRMImporter"

// for tick Editor Object
class SWindow2 : public SWindow{
public:
	//		bCanTick = false;
	SWindow2() : SWindow() {
#if	UE_VERSION_OLDER_THAN(4,21,0)
		bCanTick = true;
#else
		SetCanTick(true);
#endif
	}

public:
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) {
		SWindow::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		FTickableEditorObject::TickObjects(InDeltaTime);
		//FAssetRegistryModule::TickAssetRegistry(-1.0f);
	}
	bool OnIsActiveChanged(const FWindowActivateEvent& ActivateEvent) {
		bool r = SWindow::OnIsActiveChanged(ActivateEvent);
#if	UE_VERSION_OLDER_THAN(4,21,0)
		bCanTick = true;
#else
		SetCanTick(true);
#endif
		return r;
	}
};

/*
UCLASS(config=EditorPerProjectUserSettings, AutoExpandCategories=(FTransform), HideCategories=Object, MinimalAPI)
class UFbxImportUI : public UObject, public IImportSettingsParser
{
}

*/


namespace {
	void RenameAsset(UObject* Asset, const FString& NewName, FText& ErrorMessage)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		TArray<FAssetRenameData> AssetsAndNames;
		const FString PackagePath = FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
		new(AssetsAndNames) FAssetRenameData(Asset, PackagePath, NewName);
		AssetToolsModule.Get().RenameAssetsWithDialog(AssetsAndNames);
	}
}

UVRM4UImporterFactory::UVRM4UImporterFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	TArray<FString> table = {
		TEXT("vrm"),
		TEXT("vrma"),
		TEXT("glb"),
		TEXT("bvh"),
	};

	const UVrmRuntimeSettings* Settings = GetDefault<UVrmRuntimeSettings>();
	for (auto& a : Settings->extList) {
		table.AddUnique(a);
	}

	if (Settings->bAllowAllAssimpFormat) {
		TArray<FString> tmp = {
			TEXT("pmx"),
			TEXT("obj"),
			TEXT("fbx"),
			TEXT("dae"),
			TEXT("gltf"),
		};
		for (auto& a : tmp) {
			table.AddUnique(a);
		}
	}

	for (auto &a : table) {
		Formats.Add(a + TEXT(";Model VRM4U"));
	}

	bCreateNew = false;
	bEditorImport = true;

	ImportPriority = FMath::Max(1, Settings->ImportPriority);
}


bool UVRM4UImporterFactory::FactoryCanImport(const FString& Filename)
{
	fullFileName.Empty();

	const FString Extension = FPaths::GetExtension(Filename);
	bool allowAll = false;

	const UVrmRuntimeSettings* Settings = GetDefault<UVrmRuntimeSettings>();
	if (Settings) {
		if (Settings->bAllowAllAssimpFormat) {
			allowAll = true;
		}

		for (auto& a : Settings->extList) {
			if (Extension == a) {
				allowAll = true;
			}
		}
	}


	//if( Extension == TEXT("vrm") || Extension == TEXT("gltf") || Extension == TEXT("glb"))
	if(allowAll
		|| Extension == TEXT("vrm")
		|| Extension == TEXT("vrma")
		|| Extension == TEXT("glb")
		|| Extension == TEXT("bvh"))
	{
		fullFileName = Filename;
		return true;
	}

	return false;
}

UClass* UVRM4UImporterFactory::ResolveSupportedClass()
{
	UClass* ImportClass = USkeletalMesh::StaticClass();

	return ImportClass;
}

/*
UObject* UVRMImporterFactory::FactoryCreateFile
(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	const FString& InFilename,
	const TCHAR* Parms,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled
)
{
	return nullptr;
}
*/

UObject* UVRM4UImporterFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	if (fullFileName.IsEmpty()) {
		return nullptr;
	}

	if (VRMConverter::Options::Get().IsDebugIgnoreVRMValidation() == false) {
		if (ULoaderBPFunctionLibrary::IsValidVRM4UFile(fullFileName) == false) {
			return nullptr;
		}
	}

	static UVrmImportUI* ImportUI = nullptr;
#if	UE_VERSION_OLDER_THAN(5,0,0)
	TAssetPtr<UObject> refPointerToLic;
#else
	TSoftObjectPtr<UObject> refPointerToLic;
#endif
	{
		if (ImportUI == nullptr) {
			ImportUI = NewObject<UVrmImportUI>(this, NAME_None, RF_NoFlags);
			ImportUI->AddToRoot();
		}

		// to default
		{
			ImportUI->bRemoveRootBoneRotation = true;
			ImportUI->bRemoveRootBonePosition = false;
			ImportUI->bVrm10RemoveLocalRotation = false;

			ImportUI->ModelScale = 1.0f;
			ImportUI->bMergeMaterial = true;
			ImportUI->bMergePrimitive = false;
			ImportUI->TitleAuthor.Empty();
			ImportUI->Thumbnail = nullptr;

			ImportUI->TitleAuthor.Empty();
			ImportUI->Thumbnail = nullptr;
			ImportUI->allowedUserName.Empty();
			ImportUI->violentUsageName.Empty();
			ImportUI->sexualUsageName.Empty();
			ImportUI->commercialUsageName.Empty();
			ImportUI->otherPermissionUrl.Empty();
			ImportUI->licenseName.Empty();
			ImportUI->otherLicenseUrl.Empty();

			ImportUI->LicenseString.Empty();
			ImportUI->LicenseBool.Empty();
			ImportUI->LicenseStringArray.Empty();
		}

		if (1) {
			UVrmLicenseObject* a = nullptr;
			UVrm1LicenseObject* b = nullptr;
			ULoaderBPFunctionLibrary::GetVRMMeta(fullFileName, a, b);
			if (a) {
				ImportUI->TitleAuthor = TEXT("\"") + a->title + TEXT("\"") + TEXT(" / ") + TEXT("\"") + a->author + TEXT("\"");
				ImportUI->Thumbnail = a->thumbnail;
				ImportUI->allowedUserName = a->allowedUserName;
				ImportUI->violentUsageName = a->violentUsageName;
				ImportUI->sexualUsageName = a->sexualUsageName;
				ImportUI->commercialUsageName = a->commercialUsageName;
				ImportUI->otherPermissionUrl = a->otherPermissionUrl;
				ImportUI->licenseName = a->licenseName;
				ImportUI->otherLicenseUrl = a->otherLicenseUrl;

				refPointerToLic = a;
			}
			if (b) {
				{
					auto *p = b->LicenseString.FindByPredicate([](const FLicenseStringDataPair &pair) {
						if (pair.key == TEXT("name")) return true;
						return false;
						});
					if (p) {
						ImportUI->TitleAuthor = p->value;
					}
				}
				ImportUI->Thumbnail = b->thumbnail;
				ImportUI->LicenseString = b->LicenseString;
				ImportUI->LicenseBool = b->LicenseBool;
				ImportUI->LicenseStringArray = b->LicenseStringArray;
				refPointerToLic = b;
			}
			{
				const FString Extension = FPaths::GetExtension(fullFileName);
				if (Extension.ToLower() == TEXT("pmx")) {
					ImportUI->ModelScale = 0.1f;
					ImportUI->bMergeMaterial = false;
					ImportUI->bMergePrimitive = false;
					ImportUI->bForceTwoSided = true;
				}
				if (Extension.ToLower() == TEXT("bvh")) {
					ImportUI->ModelScale = 0.01f;
				}
			}
#if	UE_VERSION_OLDER_THAN(5,2,0)
			{
				ImportUI->bSingleUAssetFile = true;
			}
#else
			// 5.2でも動くが、デフォルトをOFFにする
#endif


#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
			// for thumbnail update
			if (GEditor){
				auto* s = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
				if (s && ImportUI->Thumbnail) {
					s->OpenEditorForAsset(ImportUI->Thumbnail);
					s->CloseAllEditorsForAsset(ImportUI->Thumbnail);
				}
			}
#endif
		}


		TSharedPtr<SWindow> ParentWindow;

		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		// Compute centered window position based on max window size, which include when all categories are expanded
		const float FbxImportWindowWidth = 410.0f * 2.f;
		const float FbxImportWindowHeight = 750.0f;
		FVector2D FbxImportWindowSize = FVector2D(FbxImportWindowWidth, FbxImportWindowHeight); // Max window size it can get based on current slate


		FSlateRect WorkAreaRect = FSlateApplicationBase::Get().GetPreferredWorkArea();
		FVector2D DisplayTopLeft(WorkAreaRect.Left, WorkAreaRect.Top);
		FVector2D DisplaySize(WorkAreaRect.Right - WorkAreaRect.Left, WorkAreaRect.Bottom - WorkAreaRect.Top);

		float ScaleFactor = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(DisplayTopLeft.X, DisplayTopLeft.Y);
		FbxImportWindowSize *= ScaleFactor;

		FVector2D WindowPosition = (DisplayTopLeft + (DisplaySize - FbxImportWindowSize) / 2.0f) / ScaleFactor;


		TSharedRef<SWindow2> Window = SNew(SWindow2)
			.Title(NSLOCTEXT("UnrealEd", "VRMImportOpionsTitle", "VRM Import Options"))
			.SizingRule(ESizingRule::Autosized)
			.AutoCenter(EAutoCenter::None)
			.ClientSize(FbxImportWindowSize)
			.ScreenPosition(WindowPosition);

		TSharedPtr<SVrmOptionWindow> VrmOptionWindow;
		Window->SetContent
		(
			SAssignNew(VrmOptionWindow, SVrmOptionWindow)
			.ImportUI(ImportUI)
			.WidgetWindow(Window)
			.FullPath(FText::FromString(fullFileName))
			//.ForcedImportType( bForceImportType ? TOptional<EFBXImportType>( ImportType ) : TOptional<EFBXImportType>() )
			//.IsObjFormat( bIsObjFormat )
			.MaxWindowHeight(FbxImportWindowHeight)
			.MaxWindowWidth(FbxImportWindowWidth)
		);

		// @todo: we can make this slow as showing progress bar later
		FSlateApplication::Get().AddModalWindow(Window, ParentWindow);

		if (VrmOptionWindow->ShouldImport() == false) {
			bOutOperationCanceled = true;

			return nullptr;
		}

	}

	//static ConstructorHelpers::FObjectFinder<UObject> MatClass(TEXT("/Game/test/NewMaterial.NewMaterial"));
	//static ConstructorHelpers::FObjectFinder<UClass> MatClass(TEXT("Blueprint'/VRM4U/VrmObjectListBP.VrmObjectListBP_C'"));
	//static ConstructorHelpers::FObjectFinder<UVrmAssetListObject> MatClass(TEXT("Blueprint'/VRM4U/VrmObjectListBP'"));
	//static ConstructorHelpers::FObjectFinder<UObject> MatClass(TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP"));
	//UVrmAssetListObject *m = Cast<UVrmAssetListObject>(MatClass.Object);

	//UObject* objFinder = StaticLoadObject(UVrmAssetListObject::StaticClass(), nullptr, TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP_C"));
	//UObject* objFinder = NewObject<UVrmAssetListObject>(InParent, NAME_None, RF_Transactional);

	auto& importOption = VRMConverter::Options::Get();
	importOption.SetVrmOption(ImportUI->GenerateOptionData());


#if	UE_VERSION_OLDER_THAN(5,0,0)
	TAssetPtr<UVrmAssetListObject> vrmAssetList;
	TArray< TAssetPtr<UObject> > tt;
#else
	TSoftObjectPtr<UVrmAssetListObject> vrmAssetList;
	TArray< TSoftObjectPtr<UObject> > tt;
#endif
	tt.Add(InParent);

	//TRefCountPtr<UVrmAssetListObject> m;
	//UVrmAssetListObject *m = nullptr;
#if	UE_VERSION_OLDER_THAN(5,0,0)
	TAssetPtr<UClass> c;
#else
	TSoftObjectPtr<UClass> c;
#endif
	{
		const UVrmRuntimeSettings* Settings = GetDefault<UVrmRuntimeSettings>();

		{
			FSoftObjectPath r = Settings->AssetListObject; //(TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP"));
			UObject *u = r.TryLoad();
			if (u) {
				if (Cast<UBlueprint>(u)) {
					c = (UClass*)(Cast<UBlueprint>(u)->GeneratedClass);
				}
			}
		}
		if (c == nullptr || VRMConverter::Options::Get().IsUE5Material()) {
			FSoftObjectPath r(TEXT("/VRM4U/VrmAssetListObjectBPUE5.VrmAssetListObjectBPUE5"));
			UObject *u = r.TryLoad();
			if (u) {
				c = (UClass*)(Cast<UBlueprint>(u)->GeneratedClass);
			}
		}
		if (c == nullptr){
			FSoftObjectPath r(TEXT("/VRM4U/VrmAssetListObjectBP.VrmAssetListObjectBP"));
			UObject* u = r.TryLoad();
			if (u) {
				c = (UClass*)(Cast<UBlueprint>(u)->GeneratedClass);
			}
		}

		if (c == nullptr) {
			c = UVrmAssetListObject::StaticClass();
		}

		{
			if (ReimportBase) {
				vrmAssetList = ReimportBase;
				VRMUtil::CloseEditorWindowByFolderPath(vrmAssetList.Get());
			}
			if (vrmAssetList) {
				vrmAssetList->MarkPackageDirty();
			}else{
				vrmAssetList = NewObject<UVrmAssetListObject>((UObject*)GetTransientPackage(), c.Get());
			}
		}

	}

	//UVrmAssetListObject *m = Cast<UVrmAssetListObject>(u);
	//FSoftClassPath r(TEXT("/VRM4U/VrmObjectListBP.VrmObjectListBP"));
	//UObject *u = r.TryLoad();
	//auto aaa = NewObject<UObject>(c);

	UVrmAssetListObject* mret = nullptr;
	if (vrmAssetList) {
		//auto a = NewObject<UVrmAssetListObject>(MatClass.Object, NAME_None, RF_Transactional);
		//MatClass.Object; 
		//ULoaderBPFunctionLibrary::LoadVRMFile(nullptr, fullFileName);

		GWarn->BeginSlowTask( NSLOCTEXT("UnrealEd", "ImportVRM", "Importing VRM"), true );

		int ret = true;

		ULoaderBPFunctionLibrary::SetImportMode(true, Cast<UPackage>(InParent));
		{
			ret = ULoaderBPFunctionLibrary::LoadVRMFileLocal(vrmAssetList.Get(), mret, fullFileName);
		}

		/*
		{
			const bool bCheckDirty = false;
			const bool bPromptToSave = false;
			TArray<UPackage* > p;
			p.Add(Cast<UPackage>(InParent));

			p[0]->Modify();
			p[0]->PostEditChange();
			p[0]->FullyLoad();
			p[0]->AddToRoot();
			FEditorFileUtils::PromptForCheckoutAndSave(p, bCheckDirty, bPromptToSave);

			p[0]->SetDirtyFlag(false);
			//p[0]->AddToRoot();

			//p[0]->Rename(*(p[0]->GetName() + TEXT("aatest")), nullptr);

			//ContentBrowserUtils::RenameAsset(p[0], NewName, ErrorMessage);
			FText ErrorMessage;
			//RenameAsset(p[0], *(p[0]->GetName() + TEXT("aatest")), ErrorMessage);
			RenameAsset(p[0], (TEXT("vrm_dummy_to_delete")), ErrorMessage);

			p[0]->Rename(nullptr, GetTransientPackage(), 0);
			p[0]->RemoveFromRoot();

			//FEditorFileUtils::PromptForCheckoutAndSave(p, bCheckDirty, bPromptToSave);
		}
		*/

		ULoaderBPFunctionLibrary::SetImportMode(false, nullptr);
		importOption.SetVrmOption(nullptr);

#if	UE_VERSION_OLDER_THAN(4,22,0)
#else
		if (GEditor) {
			GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, mret);
		}
#endif


		GWarn->EndSlowTask();

		//bool bDeleteSucceeded = ObjectTools::DeleteSingleObject( ExistingObject );
		CollectGarbage( GARBAGE_COLLECTION_KEEPFLAGS );

		if (ret == false) {
			return nullptr;
		}
	}
	//return InParent;
	return mret;// ->GetOuter();
}
UObject* UVRM4UImporterFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	return nullptr;
}

bool UVRM4UImporterFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames) {

	UVrmAssetListObject* assetList= Cast<UVrmAssetListObject>(Obj);
	TArray<FString> FactoryExtensions;
	GetSupportedFileExtensions(FactoryExtensions);

	if (assetList)
	{
		if (UAssetImportData* AssetImportData = assetList->GetAssetImportData())
		{
			AssetImportData->ExtractFilenames(OutFilenames);
			return true;
		}
		else
		{
			OutFilenames.Add(TEXT(""));
		}
		return true;
	}
	return false;
}
void UVRM4UImporterFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) {
	UVrmAssetListObject* assetList = Cast<UVrmAssetListObject>(Obj);
	if (assetList && ensure(NewReimportPaths.Num() == 1))
	{
		assetList->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}
EReimportResult::Type UVRM4UImporterFactory::Reimport(UObject* Obj) {

	UVrmAssetListObject* asset = Cast<UVrmAssetListObject>(Obj);
	if (asset == nullptr)
	{
		return EReimportResult::Failed;
	}
	if (asset->AssetImportData == nullptr) {
		return EReimportResult::Failed;
	}
#if	UE_VERSION_OLDER_THAN(4,22,0)
#else
	if (asset->AssetImportData->GetSourceFileCount() <= 0) {
		return EReimportResult::Failed;
	}
#endif

#if	UE_VERSION_OLDER_THAN(4,26,0)
#else
	ReimportBase = asset;
	asset->Package = asset->GetPackage();
	asset->ReimportBase = asset;
#endif

	{
		FString str = asset->AssetImportData->GetSourceData().SourceFiles[0].RelativeFilename;
		fullFileName = UAssetImportData::ResolveImportFilename(str, asset->Package);
	}

	const uint8_t *buf_dummy = nullptr;
	bool b_dummy = false;

	UObject* n = FactoryCreateBinary(nullptr, asset->Package, "name", (EObjectFlags)0, nullptr, nullptr, buf_dummy, buf_dummy, nullptr, b_dummy);

	ReimportBase = nullptr;
	fullFileName = "";

	if (n == nullptr) {
		return EReimportResult::Failed;
	}
	return EReimportResult::Succeeded;
}
int32 UVRM4UImporterFactory::GetPriority() const {
	return ImportPriority;
}


//////////////////////////////////////////////////////////////////////////






////

UVrmImportUI::UVrmImportUI(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsReimport = false;
	//bAutomatedImportShouldDetectType = true;
	//Make sure we are transactional to allow undo redo
	this->SetFlags(RF_Transactional);

	/*
	StaticMeshImportData = CreateDefaultSubobject<UFbxStaticMeshImportData>(TEXT("StaticMeshImportData"));
	StaticMeshImportData->SetFlags(RF_Transactional);
	StaticMeshImportData->LoadOptions();

	SkeletalMeshImportData = CreateDefaultSubobject<UFbxSkeletalMeshImportData>(TEXT("SkeletalMeshImportData"));
	SkeletalMeshImportData->SetFlags(RF_Transactional);
	SkeletalMeshImportData->LoadOptions();

	AnimSequenceImportData = CreateDefaultSubobject<UFbxAnimSequenceImportData>(TEXT("AnimSequenceImportData"));
	AnimSequenceImportData->SetFlags(RF_Transactional);
	AnimSequenceImportData->LoadOptions();

	TextureImportData = CreateDefaultSubobject<UFbxTextureImportData>(TEXT("TextureImportData"));
	TextureImportData->SetFlags(RF_Transactional);
	TextureImportData->LoadOptions();
	*/
}

void UVrmImportUI::ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson)
{
	/*
	// Skip instanced object references. 
	int64 SkipFlags = CPF_InstancedReference;
	FJsonObjectConverter::JsonObjectToUStruct(ImportSettingsJson, GetClass(), this, 0, SkipFlags);

	bAutomatedImportShouldDetectType = true;
	if(ImportSettingsJson->TryGetField("MeshTypeToImport").IsValid())
	{
		// Import type was specified by the user if MeshTypeToImport exists
		bAutomatedImportShouldDetectType = false;
	}

	const TSharedPtr<FJsonObject>* StaticMeshImportJson = nullptr;
	ImportSettingsJson->TryGetObjectField(TEXT("StaticMeshImportData"), StaticMeshImportJson);
	if(StaticMeshImportJson)
	{
		FJsonObjectConverter::JsonObjectToUStruct(StaticMeshImportJson->ToSharedRef(), StaticMeshImportData->GetClass(), StaticMeshImportData, 0, 0);
	}

	const TSharedPtr<FJsonObject>* SkeletalMeshImportJson = nullptr;
	ImportSettingsJson->TryGetObjectField(TEXT("SkeletalMeshImportData"), SkeletalMeshImportJson);
	if (SkeletalMeshImportJson)
	{
		FJsonObjectConverter::JsonObjectToUStruct(SkeletalMeshImportJson->ToSharedRef(), SkeletalMeshImportData->GetClass(), SkeletalMeshImportData, 0, 0);
	}

	const TSharedPtr<FJsonObject>* AnimImportJson = nullptr;
	ImportSettingsJson->TryGetObjectField(TEXT("AnimSequenceImportData"), AnimImportJson);
	if (AnimImportJson)
	{
		FJsonObjectConverter::JsonObjectToUStruct(AnimImportJson->ToSharedRef(), AnimSequenceImportData->GetClass(), AnimSequenceImportData, 0, 0);
	}

	const TSharedPtr<FJsonObject>* TextureImportJson = nullptr;
	ImportSettingsJson->TryGetObjectField(TEXT("TextureImportData"), TextureImportJson);
	if (TextureImportJson)
	{
		FJsonObjectConverter::JsonObjectToUStruct(TextureImportJson->ToSharedRef(), TextureImportData->GetClass(), TextureImportData, 0, 0);
	}
	*/
}

void UVrmImportUI::ResetToDefault()
{
	ReloadConfig();
}





#undef LOCTEXT_NAMESPACE