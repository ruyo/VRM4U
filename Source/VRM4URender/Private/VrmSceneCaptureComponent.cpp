// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmSceneCaptureComponent.h"
#include "SceneViewExtension.h"
#include "Runtime/Engine/Public/SceneView.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/GameViewportClient.h"
#include "ScreenPass.h"
#include "Kismet/GameplayStatics.h"

#include "VRM4URender.h"
#include "VrmBPFunctionLibrary.h"
#include "Misc/EngineVersionComparison.h"

#if WITH_EDITOR
#include "Editor.h"
#include "UnrealClient.h"
#include "Slate/SceneViewport.h"
#include "LevelEditorViewport.h"
#endif

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
			// ビューポートに表示されている範囲（実際のバッファ内のピクセル範囲）をコピー
			FVRM4URenderModule::AddCopyPass(GraphBuilder, InView.UnconstrainedViewRect, SrcRDGTex, CaptureComponentWeak->RenderTargetA);
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
			// ビューポートに表示されている範囲（実際のバッファ内のピクセル範囲）をコピー
			FVRM4URenderModule::AddCopyPass(GraphBuilder, InView.UnconstrainedViewRect, SrcRDGTex, CaptureComponentWeak->RenderTargetB);
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

	// RenderTargetのサイズに関わらず、カメラと同じFOVを使用
	this->FOVAngle = fovDegree;

	// ビューポートの縦横比
	float ViewportAspectRatio = 0.0f;
	bool bAspectRatioFound = false;


	// ゲーム画面として縦横比取得。のちにエディタでは上書きする
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* ViewportClient = World->GetGameViewport())
			{
				FViewport* Viewport = ViewportClient->Viewport;
				if (Viewport)
				{
					// ビューポートの実際の描画領域を取得（レターボックス/ピラーボックスを除いた領域）
					FVector2D ViewRect;
					ViewportClient->GetViewportSize(ViewRect);

					int32 ViewWidth = ViewRect.X;
					int32 ViewHeight = ViewRect.Y;

					if (ViewHeight > 0)
					{
						ViewportAspectRatio = static_cast<float>(ViewWidth) / static_cast<float>(ViewHeight);
						bAspectRatioFound = true;
					}
				}
			}
		}
	}


#if WITH_EDITOR
	// エディタの場合：アクティブなエディタビューポートから取得
	if (GEditor && GEditor->GetActiveViewport())
	{
		FViewport* EditorViewport = GEditor->GetActiveViewport();
		if (EditorViewport)
		{
			FIntPoint ViewportSize = EditorViewport->GetSizeXY();
			if (ViewportSize.Y > 0)
			{
				// デフォルトは物理サイズから計算
				ViewportAspectRatio = static_cast<float>(ViewportSize.X) / static_cast<float>(ViewportSize.Y);

				// 以下、レター・ピラー対応
				bool b1, b2, b3;
				UVrmBPFunctionLibrary::VRMGetPlayMode(b1, b2, b3);
				if (b1 == false || b2 == true) {
					FViewportClient* ViewportClient = EditorViewport->GetClient();
					if (ViewportClient)
					{
						// FLevelEditorViewportClientの場合、AspectRatio制約を確認
						// Note: Cast<>は安全な動的キャストで、失敗時にnullptrを返す
						const FLevelEditorViewportClient* LevelClient = StaticCast<FLevelEditorViewportClient*>(ViewportClient);
						//if (FLevelEditorViewportClient* LevelViewportClient =
						//	Cast<FLevelEditorViewportClient>(ViewportClient))
						if (LevelClient)
						{
							if (LevelClient->IsAspectRatioConstrained()){
								// AspectRatio制約が設定されている場合はそれを使用
								if (LevelClient->AspectRatio > 0.0f)
								{
									ViewportAspectRatio = LevelClient->AspectRatio;
								}
							}
						}
					}
				}

				bAspectRatioFound = true;
			}
		}
	}
#endif


	// ビューポートの縦横比が取得できた場合、カスタム投影行列を設定
	if (bAspectRatioFound && ViewportAspectRatio > 0.0f)
	{
		// パースペクティブ投影の場合のみカスタム投影行列を設定
		if (ProjectionType == ECameraProjectionMode::Perspective)
		{
			const float HalfFOVRadians = FMath::DegreesToRadians(FOVAngle) * 0.5f;
			const float NearPlane = bOverride_CustomNearClippingPlane ? CustomNearClippingPlane : GNearClippingPlane;

			// ビューポートの縦横比を使用してパースペクティブ投影行列を作成
			// UE 5.7では4パラメータ (HalfFOV, Width, Height, MinZ) が必要
			const float Width = ViewportAspectRatio;
			const float Height = 1.0f;
			FMatrix ProjectionMatrix = FReversedZPerspectiveMatrix(HalfFOVRadians, Width, Height, NearPlane);

			this->bUseCustomProjectionMatrix = true;
			this->CustomProjectionMatrix = ProjectionMatrix;
		}
		else
		{
			// オルソグラフィック投影の場合はカスタム投影行列を無効化
			this->bUseCustomProjectionMatrix = false;
		}
	}
}

