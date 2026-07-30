#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FJsonObject;

class AXONREWINDDEBUGGER_API FAxonRewindDebuggerActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Session
	static FAxonActionResult HandleGetSessionInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListDebuggedObjects(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetSelectedTrack(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetScrubTime(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleStartRecording(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleStopRecording(const TSharedPtr<FJsonObject>& Params);

	// Animation
	static FAxonActionResult HandleListAnimInstances(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSampleAnimNodes(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSampleAnimNodeValues(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSampleStateMachines(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSampleMontages(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSampleSequencePlayers(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSampleSkeletalPose(const TSharedPtr<FJsonObject>& Params);

	// Camera (optional)
	static FAxonActionResult HandleSampleCameraGraphResult(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSampleCameraWatches(const TSharedPtr<FJsonObject>& Params);

	// Object
	static FAxonActionResult HandleResolveObject(const TSharedPtr<FJsonObject>& Params);
};
