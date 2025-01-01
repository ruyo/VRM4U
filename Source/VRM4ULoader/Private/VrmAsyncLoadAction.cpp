// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmAsyncLoadAction.h"
#include "Async/TaskGraphInterfaces.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"

#include "LoaderBPFunctionLibrary.h"
#include "VrmAssetListObject.h"
#include "VRM4ULoaderLog.h"


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>

#if	UE_VERSION_OLDER_THAN(4,23,0)
#define TRACE_CPUPROFILER_EVENT_SCOPE(a)
#define TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(a)
#else
#endif

namespace {
	class VrmLocalAsyncAsset {
	public:
		TArray<bool> NormalBoolTable;
		TArray<bool> MaskBoolTable;
		TArray<uint8> vrmLocalRes;

		Assimp::Importer* Importer = nullptr;
		const aiScene* ScenePtr = nullptr;

		void Reset() {
			delete Importer;
			Importer = nullptr;
			ScenePtr = nullptr;

			NormalBoolTable.Empty();
			MaskBoolTable.Empty();
			vrmLocalRes.Empty();
		}
	};
	VrmLocalAsyncAsset localAsset;
}

static bool ConvTex(UVrmAssetListObject* vrmAssetList, const aiScene* mScenePtr, const FImportOptionData* option, const int TexCount, const int SubCount) {
	if (vrmAssetList == nullptr || mScenePtr == nullptr) {
		return true;
	}
	if ((int)mScenePtr->mNumTextures <= TexCount) {
		return true;
	}

	if (TexCount == 0 && SubCount==0) {
		localAsset.NormalBoolTable.Empty();
		localAsset.MaskBoolTable.Empty();
		vrmAssetList->Textures.Empty();

		localAsset.NormalBoolTable.SetNum(mScenePtr->mNumTextures);
		localAsset.MaskBoolTable.SetNum(mScenePtr->mNumTextures);
		vrmAssetList->Textures.SetNum(mScenePtr->mNumTextures);

		const VRM::VRMMetadata* meta = static_cast<const VRM::VRMMetadata*>(mScenePtr->mVRMMeta);
		if (meta) {
			for (int i = 0; i < meta->materialNum; ++i) {
				int t = 0;

				t = meta->material[i].textureProperties._BumpMap;
				if (localAsset.NormalBoolTable.IsValidIndex(t)) {
					localAsset.NormalBoolTable[t] = true;
				}

				t = meta->material[i].textureProperties._OutlineWidthTexture;
				if (localAsset.MaskBoolTable.IsValidIndex(t)) {
					localAsset.MaskBoolTable[t] = true;
				}
			}
		}
	}

	if (mScenePtr->HasTextures()) {
		// Note: PNG format.  Other formats are supported

		//for (uint32_t i = 0; i < mScenePtr->mNumTextures; ++i) {
		{
			uint32_t i = TexCount;
			if (SubCount == 0) {
				auto& t = *mScenePtr->mTextures[i];
				int Width = t.mWidth;
				int Height = t.mHeight;

				FString baseName = VRMConverter::NormalizeFileName(t.mFilename.C_Str());
				if (baseName.Len() == 0) {
					baseName = TEXT("texture") + FString::FromInt(i);
				}
				bool bNormalGreenFlip = localAsset.NormalBoolTable[i];
				if (bNormalGreenFlip) {
					baseName += TEXT("_N");
				}

				FString name = FString(TEXT("T_")) + baseName;
				auto* pkg = GetTransientPackage();
				UTexture2D* NewTexture2D = VRMLoaderUtil::CreateTextureFromImage(name, pkg, t.pcData, t.mWidth, false, localAsset.NormalBoolTable[i], bNormalGreenFlip&&(VRMConverter::IsImportMode() == false));
				vrmAssetList->Textures[i] = NewTexture2D;
			}

			if (SubCount == 1) {
				UTexture2D* NewTexture2D = vrmAssetList->Textures[i];

#if WITH_EDITOR
				NewTexture2D->DeferCompression = false;
#endif

				// Set options
				if (localAsset.NormalBoolTable[i]) {
					NewTexture2D->CompressionSettings = TC_Normalmap;
					NewTexture2D->SRGB = 0;
#if WITH_EDITOR
					if (VRMConverter::IsImportMode()) {
						NewTexture2D->bFlipGreenChannel = true;
					}
#endif
				}
				if (localAsset.MaskBoolTable[i]) {
					// comment for material warning...
					//NewTexture2D->SRGB = 0;
				}

				if (NewTexture2D->SRGB) {
					if (option->bBC7Mode) {
						NewTexture2D->CompressionSettings = TC_BC7;
					}
				}
				if (option->bMipmapGenerateMode == false) {
#if WITH_EDITORONLY_DATA
					NewTexture2D->MipGenSettings = TMGS_NoMipmaps;
#endif
				}

				NewTexture2D->UpdateResource();
#if WITH_EDITOR
				//NewTexture2D->PostEditChange();
#endif

			}
		}
	}
	return false;
}




FVrmAsyncLoadAction::FVrmAsyncLoadAction(const FLatentActionInfo& LatentInfo, FVrmAsyncLoadActionParam &p)
	: ExecutionFunction(LatentInfo.ExecutionFunction)
	, OutputLink(LatentInfo.Linkage)
	, CallbackTarget(LatentInfo.CallbackTarget)
	, param(p)
	{
}


void FVrmAsyncLoadAction::UpdateOperation(FLatentResponse& Response)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("VRMLoad UpdateOperation"))

	enum class ESequenceNo : uint32{
		Init,
		FileWait,
		AssImp,
		TextureLoop,
		AssetCreate,
		Finish,
	};

	static int TexCount = 0;
	static int SubCount = 0;
	static int FrameCount = 0;
	static double StartTime = 0.f;
	++FrameCount;

	auto logFunc = [&](FString str="") {
		UE_LOG(LogVRM4ULoader, Log, TEXT("AsyncLoad frame=%04d(%02.2lf),  SequenceNo=%02d %s"), FrameCount, FPlatformTime::Seconds()-StartTime, SequenceCount, *str);
	};
	auto logTexFunc = [&](int TexCount) {
		UE_LOG(LogVRM4ULoader, Log, TEXT("AsyncLoad frame=%04d(%02.2lf),  SequenceNo=%02d  TextureCount=%02d"), FrameCount, FPlatformTime::Seconds() - StartTime, SequenceCount, TexCount);
	};

	// async file load
	if (SequenceCount == (int)ESequenceNo::Init) {
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("VRMLoad UpdateOperation init"))
		TexCount = 0;
		SubCount = 0;
		FrameCount = 0;
		StartTime = FPlatformTime::Seconds();
		logFunc("Begin");
		++SequenceCount;


		TFunction< void() > f = [&] {
			if (FFileHelper::LoadFileToArray(localAsset.vrmLocalRes, *param.filepath)) {
				param.pData = localAsset.vrmLocalRes.GetData();
				param.dataSize = localAsset.vrmLocalRes.Num();
			}
		};

		auto p2 = ENamedThreads::AnyBackgroundThreadNormalTask;
		//p2 = ENamedThreads::AnyBackgroundThreadNormalTask;
		//p2 = ENamedThreads::GameThread_Local;
		t2 = FFunctionGraphTask::CreateAndDispatchWhenReady(f, TStatId(), nullptr, p2);
		return;
	}

	if (SequenceCount == (int)ESequenceNo::FileWait) {
		if (t2->IsComplete()) {
			logFunc();
			++SequenceCount;

			param.OutVrmAsset = Cast<UVrmAssetListObject>(StaticDuplicateObject(param.InVrmAsset, GetTransientPackage(), NAME_None));
		}
		return;
	}

	if (SequenceCount == (int)ESequenceNo::AssImp) {
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("VRMLoad UpdateOperation assimp"))
		logFunc();
		++SequenceCount;

		localAsset.Importer = new Assimp::Importer();
		localAsset.ScenePtr = localAsset.Importer->ReadFileFromMemory(localAsset.vrmLocalRes.GetData(), localAsset.vrmLocalRes.Num(),
			aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes,
			"vrm");
		return;
	}

	if (SequenceCount == (int)ESequenceNo::TextureLoop) {
		TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*FString::Printf(TEXT("VRMLoad UpdateOperation texture %d"), SubCount))
		if (localAsset.ScenePtr == nullptr) {
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
			localAsset.Reset();
			return;
		}

		if (TexCount < (int)localAsset.ScenePtr->mNumTextures) {

			if (SubCount == 0) {
				ConvTex(param.OutVrmAsset, localAsset.ScenePtr, &param.OptionForRuntimeLoad, TexCount, 0);
			}
			if (SubCount == 2) {
				ConvTex(param.OutVrmAsset, localAsset.ScenePtr, &param.OptionForRuntimeLoad, TexCount, 1);
			}
			++SubCount;

			if (SubCount >= 4) {
				logTexFunc(TexCount);
				++TexCount;
				SubCount = 0;
			}
		} else {
			logFunc();
			++SequenceCount;
		}
		return;
	}


	if (SequenceCount == (int)ESequenceNo::AssetCreate) {
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("VRMLoad UpdateOperation asset"))
		logFunc();
		++SequenceCount;
		if (param.pData) {
			ULoaderBPFunctionLibrary::LoadVRMFileFromMemory(param.InVrmAsset, param.OutVrmAsset, param.filepath, param.pData, param.dataSize);
		} else {
			ULoaderBPFunctionLibrary::LoadVRMFile(param.InVrmAsset, param.OutVrmAsset, param.filepath, param.OptionForRuntimeLoad);
		}
		return;
	}

	if (SequenceCount == (int)ESequenceNo::Finish) {
		logFunc("End");

		Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
		localAsset.Reset();
	}
}
