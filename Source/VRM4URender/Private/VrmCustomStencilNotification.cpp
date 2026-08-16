// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmCustomStencilNotification.h"

#if WITH_EDITOR

#include "Engine/World.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/Object.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace VrmCustomStencilNotification
{
	static constexpr TCHAR ConfigSection[] = TEXT("VRM4U.SceneCapture");
	static constexpr TCHAR SuppressConfigKey[] = TEXT("SuppressCustomStencilDisabledNotification");
	static TWeakObjectPtr<const UWorld> LastShownEditorWorld;

	static bool IsSuppressed()
	{
		bool bSuppressed = false;
		if (GConfig)
		{
			GConfig->GetBool(ConfigSection, SuppressConfigKey, bSuppressed, GEditorPerProjectIni);
		}
		return bSuppressed;
	}

	static void OnCheckStateChanged(ECheckBoxState NewState)
	{
		if (GConfig)
		{
			GConfig->SetBool(
				ConfigSection,
				SuppressConfigKey,
				NewState == ECheckBoxState::Checked,
				GEditorPerProjectIni);
			GConfig->Flush(false, GEditorPerProjectIni);
		}
	}

	void TryShow(const UObject& SourceObject)
	{
		if (!GIsEditor ||
			IsRunningCommandlet() ||
			SourceObject.IsTemplate() ||
			IsSuppressed())
		{
			return;
		}

		const UWorld* World = SourceObject.GetWorld();
		if (!World)
		{
			World = SourceObject.GetTypedOuter<UWorld>();
		}
		if (!World || World->WorldType != EWorldType::Editor)
		{
			return;
		}
		if (LastShownEditorWorld.Get() == World)
		{
			return;
		}

		static const auto CVarCustomDepth =
			IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.CustomDepth"));
		constexpr int32 EnabledWithStencil = 3;
		if (!CVarCustomDepth || CVarCustomDepth->GetValueOnGameThread() == EnabledWithStencil)
		{
			return;
		}

		LastShownEditorWorld = World;

		FNotificationInfo Info(NSLOCTEXT(
			"VRM4UCustomStencilNotification",
			"CustomStencilDisabledTitle",
			"VRM4U: Custom Stencil が無効です"));
		Info.SubText = FText::Format(
			NSLOCTEXT(
				"VRM4UCustomStencilNotification",
				"CustomStencilDisabledDetail",
				"通知元: {0}\nProject Settings > Rendering > Custom Depth-Stencil Pass を Enabled with Stencil に設定してください。"),
			SourceObject.GetClass()->GetDisplayNameText());
		Info.CheckBoxText = NSLOCTEXT(
			"VRM4UCustomStencilNotification",
			"SuppressCustomStencilDisabledNotification",
			"今後この通知を表示しない");
		Info.CheckBoxState = ECheckBoxState::Unchecked;
		Info.CheckBoxStateChanged = FOnCheckStateChanged::CreateStatic(&OnCheckStateChanged);
		Info.bUseThrobber = false;
		Info.bUseSuccessFailIcons = false;
		Info.bFireAndForget = true;
		Info.ExpireDuration = 12.0f;
		Info.WidthOverride = 440.0f;

		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

#endif
