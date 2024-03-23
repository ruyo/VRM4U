// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmEditorBPFunctionLibrary.h"

#include "Engine/Engine.h"
#include "Logging/MessageLog.h"
#include "Engine/Canvas.h"
#if	UE_VERSION_OLDER_THAN(4,26,0)
#include "AssetRegistryModule.h"
#else
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#include "MovieScene.h"
#include "MovieSceneSequence.h"
#include "Sections/MovieSceneFloatSection.h"

//#include "VRM4U.h"

//OnGlobalTimeChanged

int UVrmEditorBPFunctionLibrary::EvaluateCurvesFromSequence(const UMovieSceneSequence* Seq, float FrameNo, TArray<FString>& names, TArray<float> &curves) {
    names.Empty();
    curves.Empty();

#if	UE_VERSION_OLDER_THAN(4,26,0)
    return 0;
#else

    //if (Track == nullptr) return 0;

    if (Seq == nullptr) return 0;
    if (Seq->GetMovieScene() == nullptr) return 0;

    TArray<UMovieSceneSection*> Sections = Seq->GetMovieScene()->GetAllSections();
    //const auto &Sections = Track->GetAllSections();
    for (UMovieSceneSection* Section : Sections){
        UMovieSceneFloatSection* FloatSection = Cast<UMovieSceneFloatSection>(Section);
        //UMovieSceneControlRigParameterSection* s = Cast<UMovieSceneControlRigParameterSection>(Section);

        if (FloatSection == nullptr) {
            continue;
        }
        const FMovieSceneFloatChannel& Channel = FloatSection->GetChannel();
        float Value;
        FFrameTime Time((int)(FMath::Floor(FrameNo)), FMath::Frac(FrameNo));
        bool Success = Channel.Evaluate(Time, Value);
        if (Success) {
            curves.Add(Value);
            names.Add(FloatSection->GetName());
        }
    }
    return curves.Num();
#endif
}