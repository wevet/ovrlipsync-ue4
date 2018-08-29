#pragma once
#include "OVRLipSyncModule.h"
#include "OVRLipSyncContextComponent.h"

#define ovrLipSyncSuccess 0


// Error codes that may return from Lip-Sync engine
enum class ovrLipSyncError : int
{
	Unknown = -2200,	//< An unknown error has occurred
	CannotCreateContext = -2201, 	//< Unable to create a context
	InvalidParam = -2202,	//< An invalid parameter, e.g. NULL pointer or out of range
	BadSampleRate = -2203,	//< An unsupported sample rate was declared
	MissingDLL = -2204,	//< The DLL or shared library could not be found
	BadVersion = -2205,	//< Mismatched versions between header and libs
	UndefinedFunction = -2206	//< An undefined function
};

/// Flags
enum class ovrLipSyncFlag : int
{
	None = 0x0000,
	DelayCompensateAudio = 0x0001

};

// Enum for sending lip-sync engine specific signals
enum class ovrLipSyncSignals : int
{
	VisemeOn,
	VisemeOff,
	VisemeAmount,
	VisemeSmoothing,
	SignalCount
};

class FOVRLipSync
{
	static int sOVRLipSyncInit;

public:
	static void Initialize();
	static int IsInitialized();
	static void Shutdown();

	static int CreateContext(unsigned int* Context, ovrLipSyncContextProvider Provider);
	static int DestroyContext(unsigned int Context);
	static int ResetContext(unsigned int Context);
	static int SendSignal(unsigned int Context, ovrLipSyncSignals Signal, int Arg1, int Arg2);
	static int ProcessFrame(unsigned int Context, float* AudioBuffer, ovrLipSyncFlag Flags, FOVRLipSyncFrame* Frame);
	static int ProcessFrameInterleaved(unsigned int Context, float* AudioBuffer, ovrLipSyncFlag Flags, FOVRLipSyncFrame* Frame);
};