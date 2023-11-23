// VRM4U Copyright (c) 2021-2022 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmUtil.h"

#include "VRM4U.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "InterchangeHelper.h"

#include "Modules/ModuleManager.h"



namespace {
	//same as URigHierarchy

	void LocalSanitizeName(FString& InOutName)
	{
		// Sanitize the name
		for (int32 i = 0; i < InOutName.Len(); ++i)
		{
			TCHAR& C = InOutName[i];

			const bool bGoodChar = FChar::IsAlpha(C) ||				// Any letter
				//(C == '_') || (C == '-') || (C == '.') ||			// _  - . anytime
				(FChar::IsDigit(C));// ||							// 0-9 anytime
				//((i > 0) && (C == ' '));							// Space after the first character to support virtual bones

			/*
			const bool bGoodChar =
				((C >= 'A') && (C <= 'Z')) || ((C >= 'a') && (C <= 'z')) ||		// A-Z (upper and lowercase) anytime
				(C == '_') ||													// _ anytime
				((i > 0) && (C >= '0') && (C <= '9'));							// 0-9 after the first character
			*/

			if (!bGoodChar)
			{
				C = '_';
			}
		}

		//if (InOutName.Len() > GetMaxNameLength())
		//{
		//	InOutName.LeftChopInline(InOutName.Len() - GetMaxNameLength());
		//}
	}

	FString LocalGetSanitizedName(const FString& InName)
	{
		FString Name = InName;
		LocalSanitizeName(Name);

		return Name;
	}

	FString LocalGetSafeNewName(const FString& InName, TFunction<bool(const FString&)> IsNameAvailableFunction)
	{
		check(IsNameAvailableFunction);

		FString SanitizedName = InName;
		LocalSanitizeName(SanitizedName);
		FString Name = SanitizedName;

		int32 Suffix = 1;
		while (!IsNameAvailableFunction(Name))
		{
			FString BaseString = SanitizedName;
			//if (BaseString.Len() > GetMaxNameLength() - 4)
			//{
			//	BaseString.LeftChopInline(BaseString.Len() - (GetMaxNameLength() - 4));
			//}
			Name = *FString::Printf(TEXT("%s_%d"), *BaseString, ++Suffix);
		}
		return Name;
	}

	bool LocalIsNameAvailable(const FString& InName)// , ERigElementType InElementType, FString* OutErrorMessage)
	{
		FString UnsanitizedName = InName;

		FString SanitizedName = UnsanitizedName;
		LocalSanitizeName(SanitizedName);

		if (SanitizedName != UnsanitizedName)
		{
			return false;
		}

		return true;
	}

	FString LocalGetSafeNewName(const FString& InPotentialNewName)
	{
		return LocalGetSafeNewName(InPotentialNewName, [](const FString& InName) -> bool {
			return LocalIsNameAvailable(InName);
			});
	}
}

bool VRMUtil::IsNoSafeName(const FString& str) {
	for (int i = 0; i < str.Len(); ++i) {
		if (str[i] != '_') {
			return false;
		}
	}
	return true;
}


FString VRMUtil::GetSafeNewName(const FString& str) {

	return LocalGetSafeNewName(str);
}

FString VRMUtil::MakeName(const FString& str, bool IsJoint) {
	return UE::Interchange::MakeName(str, IsJoint);
}

