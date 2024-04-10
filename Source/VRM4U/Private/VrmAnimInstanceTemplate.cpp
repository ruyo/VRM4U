// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "VrmAnimInstanceTemplate.h"
#include "Animation/AnimNodeBase.h"

FVrmAnimInstanceTemplateProxy::FVrmAnimInstanceTemplateProxy()
{
}

FVrmAnimInstanceTemplateProxy::FVrmAnimInstanceTemplateProxy(UAnimInstance* InAnimInstance)
	: FAnimInstanceProxy(InAnimInstance)
{
}

/////

UVrmAnimInstanceTemplate::UVrmAnimInstanceTemplate(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FAnimInstanceProxy* UVrmAnimInstanceTemplate::CreateAnimInstanceProxy() {
	myProxy = new FVrmAnimInstanceTemplateProxy(this);
	return myProxy;
}
