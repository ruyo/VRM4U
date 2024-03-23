// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "VrmImportUI.h"

class SButton;

class SVrmOptionWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SVrmOptionWindow )
		: _ImportUI(NULL)
		, _WidgetWindow()
		, _FullPath()
		//, _ForcedImportType()
		//, _IsObjFormat(false)
		, _MaxWindowHeight(0.0f)
		, _MaxWindowWidth(0.0f)
		{}

		SLATE_ARGUMENT( UVrmImportUI*, ImportUI )
		SLATE_ARGUMENT( TSharedPtr<SWindow>, WidgetWindow )
		SLATE_ARGUMENT( FText, FullPath )
		//SLATE_ARGUMENT( TOptional<EFBXImportType>, ForcedImportType )
		//SLATE_ARGUMENT( bool, IsObjFormat )
		SLATE_ARGUMENT( float, MaxWindowHeight)
		SLATE_ARGUMENT(float, MaxWindowWidth)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

	FReply OnImport()
	{
		bShouldImport = true;
		if ( WidgetWindow.IsValid() )
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	FReply OnImportAll()
	{
		bShouldImportAll = true;
		return OnImport();
	}

	FReply OnCancel()
	{
		bShouldImport = false;
		bShouldImportAll = false;
		if ( WidgetWindow.IsValid() )
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override
	{
		if( InKeyEvent.GetKey() == EKeys::Escape )
		{
			return OnCancel();
		}

		return FReply::Unhandled();
	}

	bool ShouldImport() const
	{
		return bShouldImport;
	}

	bool ShouldImportAll() const
	{
		return bShouldImportAll;
	}

	SVrmOptionWindow() 
		: ImportUI(NULL)
		, bShouldImport(false)
		, bShouldImportAll(false)
	{}
		
	UVrmImportUI * GetImportUI() {
		return ImportUI;
	}
private:

	bool CanImport() const;
	FReply OnResetToDefaultClick() const;
	FText GetImportTypeDisplayText() const;

private:
	UVrmImportUI*	ImportUI;
	TSharedPtr<class IDetailsView> DetailsView;
	TWeakPtr< SWindow > WidgetWindow;
	TSharedPtr< SButton > ImportButton;
	bool			bShouldImport;
	bool			bShouldImportAll;
	bool			bIsObjFormat;
};
