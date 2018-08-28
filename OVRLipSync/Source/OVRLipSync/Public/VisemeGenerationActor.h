#pragma once

#include "IPluginManager.h"
#include "TaskGraphInterfaces.h"
#include "VisemeGenerationWorker.h"
#include "OVRLipSync.h"
#include "VisemeGenerationActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class OVRLIPSYNC_API AVisemeGenerationActor : public AActor
{
	GENERATED_BODY()

private:

	void* OVRDLLHandle;
	int sOVRLipSyncInit;


	unsigned int CurrentContext = 0;

	ovrLipSyncContextProvider ContextProvider = ovrLipSyncContextProvider::Main;
	FOVRLipSyncFrame LastFrame;
	int32 instanceCtr;
	FVisemeGenerationWorker* listenerThread;

	static void VisemeGenerated_trigger(FVisemeGeneratedSignature delegate_method, FOVRLipSyncFrame LipSyncFrame);

public:

	AVisemeGenerationActor(const FObjectInitializer& ObjectInitializer);
	FOVRLipSyncFrame CurrentFrame;

	// Basic functions 
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Init", Keywords = "Viseme Generation Init"))
	bool Init();

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Shutdown", Keywords = "Viseme Generation Shutdown"))
	bool Shutdown();

	UFUNCTION()
	void VisemeGenerated_method(FOVRLipSyncFrame LipSyncFrame);

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FVisemeGeneratedSignature OnVisemeGenerated;

	int IsInitialized() { return sOVRLipSyncInit; }

	int InitLipSync(int SampleRate, int BufferSize);
	void ShutdownLipSync();
	int CreateContext(unsigned int *Context, ovrLipSyncContextProvider Provider);
	int DestroyContext(unsigned int Context);
	int ResetContext(unsigned int Context);
	int SendSignal(unsigned int Context, ovrLipSyncSignals Signal, int Arg1, int Arg2);
	int ProcessFrame(unsigned int Context, float *AudioBuffer, ovrLipSyncFlag Flags, FOVRLipSyncFrame *Frame);
	int ProcessFrameInterleaved(unsigned int Context, float *AudioBuffer, ovrLipSyncFlag Flags, FOVRLipSyncFrame *Frame);

	int ProcessFrameExternal(float *AudioBuffer, ovrLipSyncFlag Flags);

	int CreateContextExternal();
	int DestroyContextExternal();

	void ClearCurrentFrame();

	int GetPhonemeFrame(FOVRLipSyncFrame *OutFrame);

	void GetFrameInfo(int *OutFrameNumber, int *OutFrameDelay, TArray<float> *OutVisemes);
};
