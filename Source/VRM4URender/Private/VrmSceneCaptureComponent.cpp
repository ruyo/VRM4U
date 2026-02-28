// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmSceneCaptureComponent.h"
#include "SceneViewExtension.h"
#include "Runtime/Engine/Public/SceneView.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ScreenPass.h"
#include "Kismet/GameplayStatics.h"

#include "VRM4URender.h"
#include "VrmBPFunctionLibrary.h"
#include "Misc/EngineVersionComparison.h"

#if	UE_VERSION_OLDER_THAN(5,3,0)
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessMaterial.h"
#else
#include "PostProcess/PostProcessMaterialInputs.h"
#endif



class FVrmSceneCaptureSceneViewExtension : public ISceneViewExtension, public TSharedFromThis<FVrmSceneCaptureSceneViewExtension, ESPMode::ThreadSafe>
{
public:
	FVrmSceneCaptureSceneViewExtension(const FAutoRegister& AutoRegister)
	{ }

	virtual ~FVrmSceneCaptureSceneViewExtension() = default;

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {};

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override
	{
	}

	virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView, const FRenderTargetBindingSlots& RenderTargets, TRDGUniformBufferRef<FSceneTextureUniformParameters> SceneTextures)
	{
		if (InView.bIsSceneCapture == false) return;
		FRDGTextureRef DstRDGTex = nullptr;
		FRDGTextureRef SrcRDGTex = nullptr;


		if (CaptureComponentWeak.IsValid() == false) return;
		if (CaptureComponentWeak->RenderTargetA == nullptr) return;
		if (CaptureComponentWeak->RenderTargetB == nullptr) return;

		//DstRDGTex = RegisterExternalTexture(GraphBuilder, CaptureComponentWeak->RenderTargetA->GetRenderTargetResource()->GetTexture2DRHI(), TEXT("VRM4U_CopyDst"));
		for (auto &a : RenderTargets.Output) {
			if (a.GetTexture() == nullptr) continue;
			FString s = a.GetTexture()->Name;
			if (s.Contains("BufferC")) {
				SrcRDGTex = a.GetTexture();
			}
		}
		if (SrcRDGTex) {
			FVRM4URenderModule::AddCopyPass(GraphBuilder, FIntPoint(InView.UnscaledViewRect.Width(), InView.UnscaledViewRect.Height()), SrcRDGTex, CaptureComponentWeak->RenderTargetA);
		}

		SrcRDGTex = nullptr;
		//DstRDGTex = RegisterExternalTexture(GraphBuilder, CaptureComponentWeak->RenderTargetA->GetRenderTargetResource()->GetTexture2DRHI(), TEXT("VRM4U_CopyDst"));
		for (auto& a : RenderTargets.Output) {
			if (a.GetTexture() == nullptr) continue;
			FString s = a.GetTexture()->Name;
			if (s.Contains("BufferA")) {
				SrcRDGTex = a.GetTexture();
			}
		}

		if (SrcRDGTex) {
			FVRM4URenderModule::AddCopyPass(GraphBuilder, FIntPoint(InView.UnscaledViewRect.Width(), InView.UnscaledViewRect.Height()), SrcRDGTex, CaptureComponentWeak->RenderTargetB);
		}

	}

	virtual void SubscribeToPostProcessingPass(EPostProcessingPass PassId, const FSceneView& View, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
	{
		if (PassId == EPostProcessingPass::Tonemap)
		{
			//InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FVrmSceneCaptureSceneViewExtension::PostProcessPassAfterTonemap_RenderThread));
		}
	}

	FScreenPassTexture PostProcessPassAfterTonemap_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessMaterialInputs& InOutInputs)
	{
#if	UE_VERSION_OLDER_THAN(5,4,0)
		FScreenPassTexture SceneTexture = const_cast<FScreenPassTexture&>(InOutInputs.Textures[(uint32)EPostProcessMaterialInput::SceneColor]);
		return SceneTexture;
#else
		return InOutInputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
#endif
	}

public:
	TWeakObjectPtr<UVrmSceneCaptureComponent2D> CaptureComponentWeak;
};


UVrmSceneCaptureComponent2D::UVrmSceneCaptureComponent2D(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UVrmSceneCaptureComponent2D::OnRegister()
{
	Super::OnRegister();
#if WITH_EDITORONLY_DATA
	if (ProxyMeshComponent)
	{
		ProxyMeshComponent->DestroyComponent();
		ProxyMeshComponent = nullptr;
	}
#endif
	if (!SceneViewExtension.IsValid())
	{
		SceneViewExtension = FSceneViewExtensions::NewExtension<FVrmSceneCaptureSceneViewExtension>();
		SceneViewExtension->CaptureComponentWeak = this;
	}
}

void UVrmSceneCaptureComponent2D::OnUnregister()
{
	Super::OnUnregister();

	if (SceneViewExtension.IsValid())
	{
		SceneViewExtensions.Remove(SceneViewExtension);
		SceneViewExtension.Reset();
	}

	//CineCameraComponent = nullptr;
}

void UVrmSceneCaptureComponent2D::OnAttachmentChanged()
{
}

void UVrmSceneCaptureComponent2D::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FTransform transform;
	float fovDegree;
	UVrmBPFunctionLibrary::VRMGetCameraTransform(this, 0, false, transform, fovDegree);

	float fov = fovDegree;

	{
		bool b1, b2, b3;
		UVrmBPFunctionLibrary::VRMGetPlayMode(b1, b2, b3);
		if (b1 == true && b2 == false) {
			auto* c = UGameplayStatics::GetPlayerCameraManager(this, 0);
		
			float ar_current = (float)RenderTargetA->SizeX / RenderTargetA->SizeY;
			float HalfX_game = FMath::DegreesToRadians(fovDegree / 2);

			float HalfY = FMath::Atan(FMath::Tan(HalfX_game) / c->ViewTarget.POV.AspectRatio);
			float HalfX_new = FMath::Atan(ar_current * FMath::Tan(HalfY));
			fov = 2.f * FMath::RadiansToDegrees(HalfX_new);
		}
	}

	this->FOVAngle = fov;

}

