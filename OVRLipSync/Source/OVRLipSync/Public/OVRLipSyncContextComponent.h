#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OVRLipSyncContextComponent.generated.h"

#define VISEME_SAMPLES 1024
#define VISEME_BUF_SIZE 2048

// Various visemes
enum class ovrLipSyncViseme : int
{
    sil,
    PP,
    FF,
    TH,
    DD,
    kk,
    CH,
    SS,
    nn,
    RR,
    aa,
    E,
    ih,
    oh,
    ou,
    VisemesCount
};

// Enum for provider context to create
enum class ovrLipSyncContextProvider : int
{
    Main,
    Other
};

USTRUCT(BlueprintType)
struct FOVRLipSyncFrame
{
    GENERATED_BODY()

public:
    FOVRLipSyncFrame()
    {
        FrameNumber = 0;
        FrameDelay = 0;
		Visemes.AddDefaulted((int)ovrLipSyncViseme::VisemesCount);
    }

    FOVRLipSyncFrame& operator=(const FOVRLipSyncFrame& Other)
    {
        FrameNumber = Other.FrameNumber;
        FrameDelay = Other.FrameDelay;
        Visemes = Other.Visemes;
        return *this;
    }

	void CopyInput(FOVRLipSyncFrame &input)
	{
		FrameNumber = input.FrameNumber;
		FrameDelay = input.FrameDelay;
		Visemes.Empty();
		Visemes.Append(input.Visemes);
	}

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LipSync)
    int FrameNumber;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LipSync)
    int FrameDelay;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LipSync)
    TArray<float> Visemes;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVisemeGeneratedSignature, FOVRLipSyncFrame, LipSyncFrame);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OVRLIPSYNC_API UOVRLipSyncContextComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UOVRLipSyncContextComponent(const FObjectInitializer& ObjectInitializer);

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /**
    * thread safe voice processing, the result is saved in CurrentFrame
    *
    * @param	AudioData - 16000 sample rate, 1 channel
    * @param	BufferSize - must be >= VISEME_BUF_SIZE
    */
    void ProcessFrame(const uint8* AudioData, const int32 BufferSize);

    int GetPhonemeFrame(FOVRLipSyncFrame *OutFrame);

public:
    UPROPERTY(Category = Voice, BlueprintAssignable)
    FVisemeGeneratedSignature VisemeGenerated;

    UPROPERTY(Category = Visemes, EditDefaultsOnly, BlueprintReadOnly)
    TArray<FName> VisemeNames;

    UPROPERTY(Category = Visemes, EditAnywhere, BlueprintReadWrite)
    bool bDebugVisemes;

private:
    FCriticalSection LipSyncFrameCriticalSection;
    FOVRLipSyncFrame CurrentFrame;
    float SampleBuffer[VISEME_SAMPLES * 2];

    unsigned int CurrentContext = 0;
    ovrLipSyncContextProvider ContextProvider = ovrLipSyncContextProvider::Main;
};
