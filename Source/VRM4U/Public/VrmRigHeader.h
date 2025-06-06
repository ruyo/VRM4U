// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#if	UE_VERSION_OLDER_THAN(5,0,0)

#elif UE_VERSION_OLDER_THAN(5,2,0)

#include "UObject/UnrealTypePrivate.h"
#include "IKRigDefinition.h"
#include "IKRigSolver.h"
#include "Retargeter/IKRetargeter.h"
#if WITH_EDITOR
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Solvers/IKRig_PBIKSolver.h"
#endif

#elif UE_VERSION_OLDER_THAN(5,3,0)

#include "UObject/UnrealTypePrivate.h"
#include "IKRigDefinition.h"
#include "IKRigSolver.h"
#include "Retargeter/IKRetargeter.h"
#if WITH_EDITOR
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Solvers/IKRig_FBIKSolver.h"
#endif

#elif UE_VERSION_OLDER_THAN(5,6,0)

#include "UObject/UnrealTypePrivate.h"
#include "Rig/IKRigDefinition.h"
#include "Rig/Solvers/IKRigSolver.h"
#include "Retargeter/IKRetargeter.h"
#if WITH_EDITOR
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Rig/Solvers/IKRig_FBIKSolver.h"
#endif

#else

#include "UObject/UnrealTypePrivate.h"
#include "Rig/IKRigDefinition.h"
#include "Rig/Solvers/IKRigFullBodyIK.h"
#include "Retargeter/IKRetargeter.h"
#if WITH_EDITOR
#include "RigEditor/IKRigController.h"
#include "RetargetEditor/IKRetargeterController.h"
#endif

#endif
