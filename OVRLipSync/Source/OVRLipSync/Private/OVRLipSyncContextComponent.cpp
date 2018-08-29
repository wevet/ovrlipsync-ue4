#include "OVRLipSyncContextComponent.h"
#include "OVRLipSyncPrivatePCH.h"



UOVRLipSyncContextComponent::UOVRLipSyncContextComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	struct FConstructorStatics
	{
		TArray<FName> VisemeNames;

		FConstructorStatics()
		{
			VisemeNames.SetNum((int)ovrLipSyncViseme::VisemesCount);
			VisemeNames[(int)ovrLipSyncViseme::sil] = "sil";
			VisemeNames[(int)ovrLipSyncViseme::PP] = "PP";
			VisemeNames[(int)ovrLipSyncViseme::FF] = "FF";
			VisemeNames[(int)ovrLipSyncViseme::TH] = "TH";
			VisemeNames[(int)ovrLipSyncViseme::DD] = "DD";
			VisemeNames[(int)ovrLipSyncViseme::kk] = "kk";
			VisemeNames[(int)ovrLipSyncViseme::CH] = "CH";
			VisemeNames[(int)ovrLipSyncViseme::SS] = "SS";
			VisemeNames[(int)ovrLipSyncViseme::nn] = "nn";
			VisemeNames[(int)ovrLipSyncViseme::RR] = "RR";
			VisemeNames[(int)ovrLipSyncViseme::aa] = "aa";
			VisemeNames[(int)ovrLipSyncViseme::E] = "E";
			VisemeNames[(int)ovrLipSyncViseme::ih] = "ih";
			VisemeNames[(int)ovrLipSyncViseme::oh] = "oh";
			VisemeNames[(int)ovrLipSyncViseme::ou] = "ou";
		}
	};
	static FConstructorStatics ConstructorStatics;
	VisemeNames = ConstructorStatics.VisemeNames;

	PrimaryComponentTick.bCanEverTick = true;
}

void UOVRLipSyncContextComponent::BeginPlay()
{
	Super::BeginPlay();

	FOVRLipSync::CreateContext(&CurrentContext, ContextProvider);
}

void UOVRLipSyncContextComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (CurrentContext)
	{
		FOVRLipSync::DestroyContext(CurrentContext);
	}
}

void UOVRLipSyncContextComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDebugVisemes)
	{
		FOVRLipSyncFrame LipSyncFrame;
		if (ovrLipSyncSuccess == GetPhonemeFrame(&LipSyncFrame))
		{
			for (int i = 0; i < (int)ovrLipSyncViseme::VisemesCount; ++i)
			{
				float VisemeValue = LipSyncFrame.Visemes[i];
				FString OutputString = FString::Printf(TEXT("%s: %f"), *VisemeNames[i].ToString(), VisemeValue);
				GEngine->AddOnScreenDebugMessage((uint64)-1, DeltaTime, FColor::Green, OutputString);
			}
		}
	}
}

// maybe called in another thread
void UOVRLipSyncContextComponent::ProcessFrame(const uint8* AudioData, const int32 BufferSize)
{
	check(BufferSize >= VISEME_BUF_SIZE);
	if (CurrentContext == 0)
	{
		return;
	}

	for (uint32 i = 0; i < VISEME_SAMPLES; i++)
	{
		int16 Sample = (int16)(AudioData[i * 2 + 1] << 8 | AudioData[i * 2]);
		SampleBuffer[i * 2] = Sample / (float)SHRT_MAX;
		SampleBuffer[i * 2 + 1] = Sample / (float)SHRT_MAX;
	}
	FOVRLipSyncFrame TempFrame;
	FOVRLipSync::ProcessFrameInterleaved(CurrentContext, SampleBuffer, ovrLipSyncFlag::None, &TempFrame);

	// only lock the frame copy operation
	{
		FScopeLock LipSyncFrameLock(&LipSyncFrameCriticalSection);
		CurrentFrame = TempFrame;
	}

	// dispatch the event
	if (VisemeGenerated.IsBound())
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady([=]()
		{
			VisemeGenerated.Broadcast(CurrentFrame);
		}
		, TStatId(), nullptr, ENamedThreads::GameThread);
	}
}

int UOVRLipSyncContextComponent::GetPhonemeFrame(FOVRLipSyncFrame *OutFrame)
{
	if (FOVRLipSync::IsInitialized() != ovrLipSyncSuccess)
	{
		return (int)ovrLipSyncError::Unknown;
	}

	// only lock the frame copy operation
	FScopeLock LipSyncFrameLock(&LipSyncFrameCriticalSection);
	*OutFrame = CurrentFrame;

	return ovrLipSyncSuccess;
}
