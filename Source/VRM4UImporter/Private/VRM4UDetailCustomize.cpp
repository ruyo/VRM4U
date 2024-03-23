// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VRM4UDetailCustomize.h"
#include "VrmCustomStruct.h"

#include "AssetToolsModule.h"
#include "Modules/ModuleManager.h"
#include "Internationalization/Internationalization.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Misc/EngineVersionComparison.h"

#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"

#include "Animation/AnimSequenceBase.h"


FVRMRetargetSrcAnimSequenceCustomization::FVRMRetargetSrcAnimSequenceCustomization()
{
}

bool FVRMRetargetSrcAnimSequenceCustomization::ShouldFilterAsset(const FAssetData& AssetData)
{
	{
		const FString keyword[] = {
			TEXT("skel_"),
			TEXT("vrm4u"),
			TEXT("simple"),
		};

#if	UE_VERSION_OLDER_THAN(4,25,0)
		const FString* Value = AssetData.TagsAndValues.Find(TEXT("Skeleton"));
#else
		const FString V = AssetData.TagsAndValues.FindTag(TEXT("Skeleton")).GetValue();
		const FString* Value = &V;
#endif

		if (Value == nullptr) {
			return false;
		}
		const FString s = Value->ToLower();

		for (auto &a : keyword) {
			if (s.Find(a) < 0) {
				return true;
			}
		}
	}

	return false;
}

void FVRMRetargetSrcAnimSequenceCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) {

	{
		uint32 TotalChildren = 0;
		StructPropertyHandle->GetNumChildren(TotalChildren);
		for (uint32 ChildIndex = 0; ChildIndex < TotalChildren; ++ChildIndex)
		{
			TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);

			if (ChildHandle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(FVRMRetargetSrcAnimSequence, AnimSequence)) {
				StructBuilder.AddCustomRow(ChildHandle->GetPropertyDisplayName())
					.NameContent()
					[
						StructPropertyHandle->CreatePropertyNameWidget()
					]
					.ValueContent()
					.MaxDesiredWidth(250.0f)
					.MinDesiredWidth(250.0f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UAnimSequenceBase::StaticClass())
						.OnShouldFilterAsset(this, &FVRMRetargetSrcAnimSequenceCustomization::ShouldFilterAsset)
						.PropertyHandle(ChildHandle)
						.ThumbnailPool(StructCustomizationUtils.GetThumbnailPool())
					];
			}
		}
	}
}
void FVRMRetargetSrcAnimSequenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) {
}

///

