
#include "CoreMinimal.h"
#include "VRM4UImporterLog.h"
#include "VRM4UDetailCustomize.h"
#include "VrmAssetListThumbnailRenderer.h"
#include "VrmAssetListObject.h"
#include "VrmLicenseObject.h"
#include "VrmMetaObject.h"
#include "VrmCustomStruct.h"

#include "AssetToolsModule.h"
#include "Modules/ModuleManager.h"
#include "Internationalization/Internationalization.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"



#define LOCTEXT_NAMESPACE "VRM4UImporter"

DEFINE_LOG_CATEGORY(LogVRM4UImporter);

//////////////////////////////////////////////////////////////////////////
// FSpriterImporterModule

class FVRM4UImporterModule : public FDefaultModuleImpl
{
	TArray< TSharedPtr<IAssetTypeActions> >  AssetTypeActions;

public:
	virtual void StartupModule() override
	{
		{
			auto &a = UThumbnailManager::Get();
			a.RegisterCustomRenderer(UVrmAssetListObject::StaticClass(), UVrmAssetListThumbnailRenderer::StaticClass());
			a.RegisterCustomRenderer(UVrmLicenseObject::StaticClass(), UVrmAssetListThumbnailRenderer::StaticClass());
			a.RegisterCustomRenderer(UVrmMetaObject::StaticClass(), UVrmAssetListThumbnailRenderer::StaticClass());
		}

		{
			AssetTypeActions.Empty();
			AssetTypeActions.Add(MakeShareable(new FAssetTypeActions_VrmAssetList));
			AssetTypeActions.Add(MakeShareable(new FAssetTypeActions_VrmLicense));
			AssetTypeActions.Add(MakeShareable(new FAssetTypeActions_VrmMeta));

			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

			for (decltype(auto) a : AssetTypeActions) {
				AssetTools.RegisterAssetTypeActions(a.ToSharedRef());
			}
		}

		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

			PropertyEditorModule.RegisterCustomPropertyTypeLayout(
				("VRMRetargetSrcAnimSequence"),
				FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FVRMRetargetSrcAnimSequenceCustomization::MakeInstance)
			);

			PropertyEditorModule.NotifyCustomizationModuleChanged();
		}
	}

	virtual void ShutdownModule() override
	{
		if (UObjectInitialized()){
			UThumbnailManager::Get().UnregisterCustomRenderer(UVrmAssetListObject::StaticClass());
			UThumbnailManager::Get().UnregisterCustomRenderer(UVrmLicenseObject::StaticClass());
			UThumbnailManager::Get().UnregisterCustomRenderer(UVrmMetaObject::StaticClass());

			FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyEditorModule.UnregisterCustomPropertyTypeLayout(("VRMRetargetSrcAnimSequence"));


			IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for (decltype(auto) a : AssetTypeActions) {
				AssetTools.UnregisterAssetTypeActions(a.ToSharedRef());
				a.Reset();
			}
			AssetTypeActions.Empty();
		}
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FVRM4UImporterModule, VRM4UImporter);

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
