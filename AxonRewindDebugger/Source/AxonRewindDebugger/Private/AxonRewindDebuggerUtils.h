#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "IRewindDebugger.h"
#include "IAnimationProvider.h"
#include "IGameplayProvider.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/Model/Frames.h"

class IRewindDebugger;

namespace AxonRewindDebugger
{
	static constexpr int32 DefaultLimit = 64;
	static constexpr int32 MaxPoseBones = 256;

	struct FSampleContext
	{
		IRewindDebugger* Debugger = nullptr;
		const TraceServices::IAnalysisSession* Session = nullptr;
		const IAnimationProvider* AnimProvider = nullptr;
		const IGameplayProvider* GameplayProvider = nullptr;
		double ScrubTime = 0.0;
		double TraceTime = 0.0;
		double StartTime = 0.0;
		double EndTime = 0.0;
		int32 Limit = DefaultLimit;
		bool bMovedScrub = false;
	};

	bool ParseObjectId(const TSharedPtr<FJsonObject>& Params, uint64& OutId, FString& OutError, bool bRequired = true);
	int32 ParseLimit(const TSharedPtr<FJsonObject>& Params, int32 DefaultValue = DefaultLimit);
	bool ParseBool(const TSharedPtr<FJsonObject>& Params, const TCHAR* Field, bool DefaultValue);

	/** Resolve debugger + session + optional scrub time override 뿯↽ frame window. */
	FString ResolveSampleContext(const TSharedPtr<FJsonObject>& Params, FSampleContext& Out, bool bAllowScrubWrite = true);

	TSharedPtr<FJsonObject> MakeObjectInfoJson(const IGameplayProvider* Provider, uint64 ObjectId);
	TSharedPtr<FJsonObject> MakeTransformJson(const FTransform& Xform);
	TSharedPtr<FJsonObject> MakeVariantValueJson(const FVariantValue& Value, const IAnimationProvider* AnimProvider);

	void AppendDebugObjectTree(const TSharedPtr<FDebugObjectInfo>& Info, TArray<TSharedPtr<FJsonValue>>& Out, int32 Depth, int32& Remaining);
}
