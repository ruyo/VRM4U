// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmSceneCaptureComponent.h"
#include "SceneViewExtension.h"
//#include "Runtime/Engine/Public/SceneView.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "ScreenPass.h"
#include "Kismet/GameplayStatics.h"

#include "VRM4URender.h"
#include "VrmBPFunctionLibrary.h"
#include "Misc/EngineVersionComparison.h"

#include "Camera/CameraTypes.h"
#include "Engine/LocalPlayer.h"
#include "SceneView.h"

#if WITH_EDITOR
#include "Editor.h"
#include "UnrealClient.h"
#include "Slate/SceneViewport.h"
#include "LevelEditorViewport.h"
#include "Settings/LevelEditorViewportSettings.h"
#endif

#if	UE_VERSION_OLDER_THAN(5,3,0)
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessMaterial.h"
#else
#include "PostProcess/PostProcessMaterialInputs.h"
#endif

namespace VrmSceneCaptureLocal {
	/**
	 * ULocalPlayer::GetProjectionData と同じ軸拘束ロジックで透視投影行列を組む。
	 * エディタ（非プレイ）経路でのみ使用する。
	 */
	static FMatrix BuildPerspectiveMatrix(
		float FOVDegrees,
		FIntPoint ViewportSize,
		bool bConstrainAspectRatio,
		float ConstrainedAspectRatio,
		float NearPlane,
		EAspectRatioAxisConstraint AxisConstraint)
	{
		const float HalfFOV = FMath::Max(0.001f, FOVDegrees) * UE_PI / 360.0f;

		if (bConstrainAspectRatio && ConstrainedAspectRatio > 0.0f)
		{
			// 拘束ありは本体と同じ 4 引数版で一致する
			return FReversedZPerspectiveMatrix(HalfFOV, ConstrainedAspectRatio, 1.0f, NearPlane);
		}

		const float SizeX = static_cast<float>(FMath::Max(1, ViewportSize.X));
		const float SizeY = static_cast<float>(FMath::Max(1, ViewportSize.Y));

		const bool bMaintainXFOV =
			(AxisConstraint == AspectRatio_MaintainXFOV) ||
			(AxisConstraint == AspectRatio_MajorAxisFOV && SizeX > SizeY);

		const float XAxisMultiplier = bMaintainXFOV ? 1.0f : (SizeY / SizeX);
		const float YAxisMultiplier = bMaintainXFOV ? (SizeX / SizeY) : 1.0f;

		// MinZ == MaxZ で無限遠ファークリップ（本体と同じ挙動）
		return FReversedZPerspectiveMatrix(
			HalfFOV, HalfFOV, XAxisMultiplier, YAxisMultiplier, NearPlane, NearPlane);
	}

	/** アスペクト拘束時のレターボックス／ピラーボックス後の描画領域を求める */
	static FIntPoint ComputeConstrainedSize(FIntPoint ViewportSize, bool bConstrain, float TargetAspect)
	{
		if (!bConstrain || TargetAspect <= 0.0f || ViewportSize.X <= 0 || ViewportSize.Y <= 0)
		{
			return ViewportSize;
		}

		const float ViewportAspect = static_cast<float>(ViewportSize.X) / static_cast<float>(ViewportSize.Y);
		if (ViewportAspect > TargetAspect)
		{
			// ピラーボックス：横を削る
			return FIntPoint(FMath::RoundToInt(ViewportSize.Y * TargetAspect), ViewportSize.Y);
		}
		// レターボックス：縦を削る
		return FIntPoint(ViewportSize.X, FMath::RoundToInt(ViewportSize.X / TargetAspect));
	}
}



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
		if (InView.Family && InView.Family->bThumbnailRendering) return;
		if (InView.Family && InView.Family->Scene)
		{
			if (const UWorld* ViewWorld = InView.Family->Scene->GetWorld())
			{
				if (ViewWorld->WorldType == EWorldType::EditorPreview) return;
			}
		}
		FRDGTextureRef DstRDGTex = nullptr;
		FRDGTextureRef SrcRDGTex = nullptr;



		if (RT_BaseColor) {
			// base color
			for (auto &a : RenderTargets.Output) {
				if (a.GetTexture() == nullptr) continue;
				FString s = a.GetTexture()->Name;
				if (s.Contains("BufferC")) {
					SrcRDGTex = a.GetTexture();
				}
			}
			if (SrcRDGTex) {
				FVRM4URenderModule::AddCopyPass(GraphBuilder, InView.UnconstrainedViewRect, SrcRDGTex, RT_BaseColor);
			}
		}

		if (RT_Normal) {
			// normal
			SrcRDGTex = nullptr;
			for (auto& a : RenderTargets.Output) {
				if (a.GetTexture() == nullptr) continue;
				FString s = a.GetTexture()->Name;
				if (s.Contains("BufferA")) {
					SrcRDGTex = a.GetTexture();
				}
			}
			if (SrcRDGTex) {
				FVRM4URenderModule::AddCopyPass(GraphBuilder, InView.UnconstrainedViewRect, SrcRDGTex, RT_Normal);
			}
		}

		if (RT_Depth) {
			// depth
			SrcRDGTex = nullptr;
			SrcRDGTex = RenderTargets.DepthStencil.GetTexture();
			if (SrcRDGTex)
			{
				FVRM4URenderModule::AddCopyPass(GraphBuilder, InView.UnconstrainedViewRect, SrcRDGTex, RT_Depth);
			}
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
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RT_BaseColor = nullptr;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RT_Normal = nullptr;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RT_Depth = nullptr;
};


UVrmSceneCaptureComponent2D::UVrmSceneCaptureComponent2D(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	// ボーン・カメラの最終姿勢が確定した後に投影行列を読む
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UVrmSceneCaptureComponent2D::EnsureTextureTargetCreated()
{
	if (TextureTarget)
	{
		return;
	}

	FIntPoint ViewportSize(0, 0), BufferSize(0, 0);
	UVrmBPFunctionLibrary::VRMGetViewportSize(ViewportSize, BufferSize);

	int32 Width = BufferSize.X > 0 ? BufferSize.X : 256;
	int32 Height = BufferSize.Y > 0 ? BufferSize.Y : 256;

	if (RenderTargetResolutionDivisorX > 0)
	{
		Width = FMath::Max(1, FMath::FloorToInt(static_cast<float>(Width) / RenderTargetResolutionDivisorX));
	}
	if (RenderTargetResolutionDivisorY > 0)
	{
		Height = FMath::Max(1, FMath::FloorToInt(static_cast<float>(Height) / RenderTargetResolutionDivisorY));
	}

	UTextureRenderTarget2D* NewTextureTarget = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);
	if (NewTextureTarget == nullptr)
	{
		return;
	}

	NewTextureTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	NewTextureTarget->ClearColor = FLinearColor::Transparent;
	NewTextureTarget->InitAutoFormat(Width, Height);
	NewTextureTarget->UpdateResourceImmediate(true);

	TextureTarget = NewTextureTarget;

	ResizeRenderTargets(FIntPoint(Width, Height));
}

void UVrmSceneCaptureComponent2D::OnComponentCreated()
{
	Super::OnComponentCreated();
	EnsureTextureTargetCreated();
}

void UVrmSceneCaptureComponent2D::OnRegister()
{
	EnsureTextureTargetCreated();
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

		SceneViewExtension->RT_BaseColor = RT_BaseColor;
		SceneViewExtension->RT_Normal = RT_Normal;
		SceneViewExtension->RT_Depth = RT_Depth;
	}


#if WITH_EDITOR
	if (handle.IsValid()) {
		FEditorDelegates::OnEditorCameraMoved.Remove(handle);
	}
	handle = FEditorDelegates::OnEditorCameraMoved.AddUObject(this, &UVrmSceneCaptureComponent2D::OnCameraTransformChanged);
#endif


}

void UVrmSceneCaptureComponent2D::OnUnregister()
{
	Super::OnUnregister();

	if (SceneViewExtension.IsValid())
	{
		SceneViewExtensions.Remove(SceneViewExtension);

		SceneViewExtension->RT_BaseColor = nullptr;
		SceneViewExtension->RT_Normal = nullptr;
		SceneViewExtension->RT_Depth = nullptr;
		SceneViewExtension.Reset();
	}

	//CineCameraComponent = nullptr;
}
#if WITH_EDITOR
void UVrmSceneCaptureComponent2D::OnCameraTransformChanged(const FVector&, const FRotator&, ELevelViewportType, int32) {
	OnCameraTransformChanged();
}
#endif

void UVrmSceneCaptureComponent2D::OnCameraTransformChanged() {

	FTransform transform;
	float fov;
	UVrmBPFunctionLibrary::VRMGetCameraTransform(this, 0, false, transform, fov);

	this->SetWorldTransform(transform);
}

void UVrmSceneCaptureComponent2D::OnAttachmentChanged()
{
}

void UVrmSceneCaptureComponent2D::ResizeRenderTargets(FIntPoint size) {


	//FIntPoint vs, bs;
	//UVrmBPFunctionLibrary::VRMGetViewportSize(vs, bs, true);
	FIntPoint bs = size;

	if (bs.X > 0 && bs.Y > 0)
	{
		if (RenderTargetResolutionDivisorX > 0)
		{
			bs.X /= RenderTargetResolutionDivisorX;
		}
		if (RenderTargetResolutionDivisorY > 0)
		{
			bs.Y /= RenderTargetResolutionDivisorY;
		}

		if (this->TextureTarget)
		{
			this->TextureTarget->ResizeTarget(bs.X, bs.Y);
		}

		if (RT_BaseColor)
		{
			RT_BaseColor->ResizeTarget(bs.X, bs.Y);
		}
		if (RT_Normal)
		{
			RT_Normal->ResizeTarget(bs.X, bs.Y);
		}

		if (RT_Depth)
		{
			RT_Depth->ResizeTarget(bs.X, bs.Y);
		}
	}
}





void UVrmSceneCaptureComponent2D::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	using namespace VrmSceneCaptureLocal;

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	OnCameraTransformChanged();

	FMatrix   NewProjectionMatrix = FMatrix::Identity;
	FIntPoint ViewSize = FIntPoint::ZeroValue;
	bool      bProjectionFound = false;

	UWorld* World = GetWorld();

	// Simulate In Editor / Eject(F8) 中は、ワールドは Game World だが実際に
	// 画を出しているのはエディタのレベルビューポート（フリーカメラ）。
	// LocalPlayer 側の投影データを見ると必ずズレるので、ここで経路を分ける。
#if WITH_EDITOR
	const bool bUseEditorView =
		(GEditor != nullptr) &&
		(GEditor->bIsSimulatingInEditor || World == nullptr || !World->IsGameWorld());
#else
	const bool bUseEditorView = false;
#endif

	// ---------------------------------------------------------------
	// ゲーム／PIE：主ビューが実際に使う投影データをそのまま受け取る。
	// 軸拘束・レターボックス・シネカメラ・カメラシェイク・ニアプレーンが
	// すべてここで解決されるため、FOV を組み直す必要はない。
	// ---------------------------------------------------------------
	if (!bUseEditorView && World && World->IsGameWorld())
	{
		if (ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController())
		{
			if (LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->Viewport)
			{
				FSceneViewProjectionData ProjectionData;
				if (LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
				{
					NewProjectionMatrix = ProjectionData.ProjectionMatrix;
					ViewSize = ProjectionData.GetConstrainedViewRect().Size();
					bProjectionFound = true;
				}
			}
		}
	}
#if WITH_EDITOR
	// ---------------------------------------------------------------
	// エディタ（非プレイ）／Simulate／Eject：レベルビューポートから再構築する。
	// GEditor->GetActiveViewport() はフォーカス依存で別アセットエディタを
	// 返し得るため使用しない。
	// ---------------------------------------------------------------
	else if (GEditor)
	{
		FLevelEditorViewportClient* LevelClient = nullptr;

		// このコンポーネントが属するワールドを表示しているビューポートを選ぶ。
		// Simulate 中は IsSimulateInEditorViewport() が立っているものが正解。
		for (FLevelEditorViewportClient* Candidate : GEditor->GetLevelViewportClients())
		{
			if (Candidate == nullptr || Candidate->Viewport == nullptr || !Candidate->IsVisible())
			{
				continue;
			}
			if (World != nullptr && Candidate->GetWorld() != World)
			{
				continue;
			}
			if (GEditor->bIsSimulatingInEditor && !Candidate->IsSimulateInEditorViewport())
			{
				continue;
			}
			LevelClient = Candidate;
			break;
		}

		if (LevelClient == nullptr &&
			GCurrentLevelEditingViewportClient != nullptr &&
			GCurrentLevelEditingViewportClient->Viewport != nullptr)
		{
			LevelClient = GCurrentLevelEditingViewportClient;
		}

		// Simulate 中はプレイヤーカメラではなくエディタのフリーカメラが画になる。
		// 投影行列だけでなく位置・回転もこちらから取らないとズレる
		// （冒頭の OnCameraTransformChanged() の結果をここで上書きする）。
		if (LevelClient && LevelClient->Viewport)
		{
			SetWorldLocationAndRotation(
				LevelClient->GetViewLocation(),
				LevelClient->GetViewRotation());
		}

		// オルソは編集用ビューでのみ発生し、投影系が別物なので追従しない
		if (LevelClient && LevelClient->Viewport && !LevelClient->IsOrtho())
		{
			const FIntPoint ViewportSize = LevelClient->Viewport->GetSizeXY();
			if (ViewportSize.X > 0 && ViewportSize.Y > 0)
			{
				const bool  bConstrained = LevelClient->IsAspectRatioConstrained();
				const float ConstrainedAspect = LevelClient->AspectRatio;

				NewProjectionMatrix = BuildPerspectiveMatrix(
					LevelClient->ViewFOV,
					ViewportSize,
					bConstrained,
					ConstrainedAspect,
					GNearClippingPlane,
					GetDefault<ULevelEditorViewportSettings>()->AspectRatioAxisConstraint);

				ViewSize = ComputeConstrainedSize(ViewportSize, bConstrained, ConstrainedAspect);
				bProjectionFound = true;
			}
		}
	}
#endif

	if (!bProjectionFound || ViewSize.X <= 0 || ViewSize.Y <= 0)
	{
		// 取得できないフレームは前回の設定を維持する（別画角で 1 フレーム描かない）
		return;
	}

	// RT のアスペクトはビュー矩形に必ず一致させる。
	// ズレると合成結果が非等方に伸びるため、投影行列を決めた後に呼ぶ。
	ResizeRenderTargets(ViewSize);

	bUseCustomProjectionMatrix = true;
	CustomProjectionMatrix = NewProjectionMatrix;

	// 投影行列が優先されるが、FOVAngle / ProjectionType を参照する箇所があるため整合させる。
	// 透視行列は M[2][3] == 1、オルソは 0。
	const bool bPerspective = !FMath::IsNearlyZero(NewProjectionMatrix.M[2][3]);
	ProjectionType = bPerspective ? ECameraProjectionMode::Perspective : ECameraProjectionMode::Orthographic;

	if (bPerspective && NewProjectionMatrix.M[0][0] > UE_KINDA_SMALL_NUMBER)
	{
		FOVAngle = FMath::RadiansToDegrees(2.0f * FMath::Atan(1.0f / NewProjectionMatrix.M[0][0]));
	}
}

