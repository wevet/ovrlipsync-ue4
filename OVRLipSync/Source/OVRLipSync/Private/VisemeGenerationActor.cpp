// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "VisemeGenerationActor.h"
#include "OVRLipSyncPrivatePCH.h"
#include "Voice.h"

typedef int(*dll_ovrlipsyncInitializeActor)(int, int);
typedef void(*dll_ovrlipsyncShutdownActor)(void);
typedef int(*dll_ovrlipsyncGetVersionActor)(int*, int*, int*);
typedef int(*dll_ovrlipsyncCreateContextActor)(unsigned int*, int);
typedef int(*dll_ovrlipsyncDestroyContextActor)(unsigned int);
typedef int(*dll_ovrlipsyncResetContextActor)(unsigned int);
typedef int(*dll_ovrlipsyncSendSignalActor)(unsigned int, int, int, int);
typedef int(*dll_ovrlipsyncProcessFrameActor)(unsigned int, float*, int, int*, int*, float*, int);
typedef int(*dll_ovrlipsyncProcessFrameInterleavedActor)(unsigned int, float*, int, int*, int*, float*, int);

dll_ovrlipsyncInitializeActor OVRLipSyncInitializeActor;
dll_ovrlipsyncShutdownActor OVRLipSyncShutdownActor;
dll_ovrlipsyncGetVersionActor OVRLipSyncGetVersionActor;
dll_ovrlipsyncCreateContextActor OVRLipSyncCreateContextActor;
dll_ovrlipsyncDestroyContextActor OVRLipSyncDestroyContextActor;
dll_ovrlipsyncResetContextActor OVRLipSyncResetContextActor;
dll_ovrlipsyncSendSignalActor OVRLipSyncSendSignalActor;
dll_ovrlipsyncProcessFrameActor OVRLipSyncProcessFrameActor;
dll_ovrlipsyncProcessFrameInterleavedActor OVRLipSyncProcessFrameInterleavedActor;

AVisemeGenerationActor::AVisemeGenerationActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//Define Paths for direct dll bind
	FString BinariesRoot = FPaths::Combine(*FPaths::ProjectDir(), TEXT("Binaries"));
	IPluginManager &plgnMgr = IPluginManager::Get();
	TSharedPtr<IPlugin> plugin = plgnMgr.FindPlugin("OVRLipSync");
	if (!plugin.IsValid())
	{
		//UE_LOG(LogTemp, Error, TEXT("Plugin not found."));
		return;
	}

	FString PluginRoot = plugin->GetBaseDir();
	FString PlatformString;
	FString OVRDLLString;

#if _WIN64
	//64bit
	OVRDLLString = FString(TEXT("OVRLipSync_x64.dll"));
	PlatformString = FString(TEXT("Win64"));
#else
	//32bit
	OVRDLLString = FString(TEXT("OVRLipSync.dll"));
	PlatformString = FString(TEXT("Win32"));
#endif

	FString DllFilepath = FPaths::ConvertRelativePathToFull(FPaths::Combine(*PluginRoot, TEXT("Binaries"), *PlatformString, *OVRDLLString));

	UE_LOG(LogTemp, Log, TEXT("Fetching dll from %s"), *DllFilepath);

	if (!FPaths::FileExists(DllFilepath))
	{
		//UE_LOG(LogTemp, Error, TEXT("%s File is missing (Did you copy Binaries into project root?)! Hydra Unavailable."), *OVRDLLString);
		return;
	}

	OVRDLLHandle = NULL;
	OVRDLLHandle = FPlatformProcess::GetDllHandle(*DllFilepath);

	if (!OVRDLLHandle)
	{
		//UE_LOG(LogTemp, Error, TEXT("GetDllHandle failed, Hydra Unavailable."));
		//UE_LOG(LogTemp, Error, TEXT("Full path debug: %s."), *DllFilepath);
		return;
	}

	OVRLipSyncInitializeActor = (dll_ovrlipsyncInitializeActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_Initialize"));
	OVRLipSyncShutdownActor = (dll_ovrlipsyncShutdownActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_Shutdown"));
	OVRLipSyncGetVersionActor = (dll_ovrlipsyncGetVersionActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_GetVersion"));
	OVRLipSyncCreateContextActor = (dll_ovrlipsyncCreateContextActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_CreateContext"));
	OVRLipSyncDestroyContextActor = (dll_ovrlipsyncDestroyContextActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_DestroyContext"));
	OVRLipSyncResetContextActor = (dll_ovrlipsyncResetContextActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_ResetContext"));
	OVRLipSyncSendSignalActor = (dll_ovrlipsyncSendSignalActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_SendSignal"));
	OVRLipSyncProcessFrameActor = (dll_ovrlipsyncProcessFrameActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_ProcessFrame"));
	OVRLipSyncProcessFrameInterleavedActor = (dll_ovrlipsyncProcessFrameInterleavedActor)FPlatformProcess::GetDllExport(OVRDLLHandle, TEXT("ovrLipSyncDll_ProcessFrameInterleaved"));
}

bool AVisemeGenerationActor::Init()
{
	LastFrame.FrameDelay = LastFrame.FrameNumber = CurrentFrame.FrameDelay = CurrentFrame.FrameNumber = 0;

	LastFrame.Visemes.Empty();
	CurrentFrame.Visemes.Empty();

	LastFrame.Visemes.AddDefaulted((int)ovrLipSyncViseme::VisemesCount);
	CurrentFrame.Visemes.AddDefaulted((int)ovrLipSyncViseme::VisemesCount);

	// terminate any existing thread
	if (listenerThread != NULL)
	{
		Shutdown();
	}
	// start listener thread
	listenerThread = new FVisemeGenerationWorker();
	//TArray<FRecognitionKeyWord> dictionaryList;
	//listenerThread->SetLanguage(language);
	//listenerThread->AddWords(wordList);
	bool threadSuccess = listenerThread->StartThread(this);

	return threadSuccess;
}

bool AVisemeGenerationActor::Shutdown()
{
	if (listenerThread != NULL)
	{
		listenerThread->ShutDown();
		listenerThread = NULL;
		return true;
	}
	else {
		return false;
	}
}

void AVisemeGenerationActor::VisemeGenerated_trigger(FVisemeGeneratedSignature delegate_method, FOVRLipSyncFrame LipSyncFrame)
{
	delegate_method.Broadcast(LipSyncFrame);
}

void AVisemeGenerationActor::VisemeGenerated_method(FOVRLipSyncFrame LipSyncFrame)
{
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady
	(
		FSimpleDelegateGraphTask::FDelegate::CreateStatic(&VisemeGenerated_trigger, OnVisemeGenerated, LipSyncFrame)
		, TStatId()
		, nullptr
		, ENamedThreads::GameThread
	);


}

int AVisemeGenerationActor::CreateContextExternal()
{
	int result = CreateContext(&CurrentContext, ContextProvider);
	if (result != ovrLipSyncSuccess)
	{
		//UE_LOG(OVRLipSyncPluginLog, Error, TEXT("Unable to create OVR LipSync Context"));
		//Unable to create OVR LipSync Context
	}
	return result;
}

int AVisemeGenerationActor::DestroyContextExternal()
{
	return DestroyContext(CurrentContext);
}

void AVisemeGenerationActor::ClearCurrentFrame()
{
	CurrentFrame.Visemes.Empty();
	CurrentFrame.Visemes.AddDefaulted((int)ovrLipSyncViseme::VisemesCount);
}

int AVisemeGenerationActor::InitLipSync(int SampleRate, int BufferSize)
{
	sOVRLipSyncInit = OVRLipSyncInitializeActor(SampleRate, BufferSize);
	return sOVRLipSyncInit;
}

void AVisemeGenerationActor::ShutdownLipSync()
{
	OVRLipSyncShutdownActor();
}

int AVisemeGenerationActor::CreateContext(unsigned int *Context, ovrLipSyncContextProvider Provider)
{
	if (IsInitialized() != ovrLipSyncSuccess)
	{
		return (int)ovrLipSyncError::CannotCreateContext;
	}
	return OVRLipSyncCreateContextActor(Context, (int)Provider);
}

int AVisemeGenerationActor::DestroyContext(unsigned int Context)
{
	if (IsInitialized() != ovrLipSyncSuccess)
	{
		return (int)ovrLipSyncError::Unknown;
	}
	return OVRLipSyncDestroyContextActor(Context);
}

int AVisemeGenerationActor::ResetContext(unsigned int Context)
{
	if (IsInitialized() != ovrLipSyncSuccess)
	{
		return (int)ovrLipSyncError::Unknown;
	}
	return OVRLipSyncResetContextActor(Context);
}

int AVisemeGenerationActor::SendSignal(unsigned int Context, ovrLipSyncSignals Signal, int Arg1, int Arg2)
{
	if (IsInitialized() != ovrLipSyncSuccess)
	{
		return (int)ovrLipSyncError::Unknown;
	}
	return OVRLipSyncSendSignalActor(Context, (int)Signal, Arg1, Arg2);
}

int AVisemeGenerationActor::ProcessFrame(unsigned int Context, float *AudioBuffer, ovrLipSyncFlag Flags, FOVRLipSyncFrame *Frame)
{
	if (IsInitialized() != ovrLipSyncSuccess)
	{
		return (int)ovrLipSyncError::Unknown;
	}
	return OVRLipSyncProcessFrameActor(Context, AudioBuffer, (int)Flags, &Frame->FrameNumber, &Frame->FrameDelay, Frame->Visemes.GetData(), Frame->Visemes.Num());
}

int AVisemeGenerationActor::ProcessFrameInterleaved(unsigned int Context, float *AudioBuffer, ovrLipSyncFlag Flags, FOVRLipSyncFrame *Frame)
{
	if (IsInitialized() != ovrLipSyncSuccess)
	{
		return (int)ovrLipSyncError::Unknown;
	}
	return OVRLipSyncProcessFrameInterleavedActor(Context, AudioBuffer, (int)Flags, &Frame->FrameNumber, &Frame->FrameDelay, Frame->Visemes.GetData(), Frame->Visemes.Num());
}

int AVisemeGenerationActor::ProcessFrameExternal(float *AudioBuffer, ovrLipSyncFlag Flags)
{
	return ProcessFrame(CurrentContext, AudioBuffer, Flags, &CurrentFrame);
}

int AVisemeGenerationActor::GetPhonemeFrame(FOVRLipSyncFrame *OutFrame)
{
	if (IsInitialized() != ovrLipSyncSuccess)
	{
		return (int)ovrLipSyncError::Unknown;
	}
	OutFrame->CopyInput(CurrentFrame);
	return ovrLipSyncSuccess;
}

void AVisemeGenerationActor::GetFrameInfo(int *OutFrameNumber, int *OutFrameDelay, TArray<float> *OutVisemes)
{
	if (IsInitialized() != ovrLipSyncSuccess)
	{
		return;
	}
	*OutFrameNumber = CurrentFrame.FrameNumber;
	*OutFrameDelay = CurrentFrame.FrameDelay;
	OutVisemes->Append(CurrentFrame.Visemes);
}