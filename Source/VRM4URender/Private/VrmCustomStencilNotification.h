// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#if WITH_EDITOR

class UObject;

namespace VrmCustomStencilNotification
{
	/** Show the shared editor warning once when the source object belongs to an editor world. */
	void TryShow(const UObject& SourceObject);
}

#endif
