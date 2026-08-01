#pragma once

#include "CoreMinimal.h"
#include "Common/ProviderLock.h"
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

	/**
	 * Session BeginRead alone is not enough in UE 5.8+: Animation/Gameplay providers
	 * each have their own FProviderLock. Reading without FProviderReadScopeLock trips
	 * "Trying to READ from provider outside of a READ scope".
	 */
	struct FSampleReadScopes
	{
		explicit FSampleReadScopes(const FSampleContext& Ctx, const TraceServices::IProvider* ExtraProvider = nullptr)
			: SessionScope(*Ctx.Session)
		{
			if (Ctx.AnimProvider)
			{
				AnimScope = MakeUnique<TraceServices::FProviderReadScopeLock>(*Ctx.AnimProvider);
			}
			if (Ctx.GameplayProvider)
			{
				GameplayScope = MakeUnique<TraceServices::FProviderReadScopeLock>(*Ctx.GameplayProvider);
			}
			if (ExtraProvider)
			{
				ExtraScope = MakeUnique<TraceServices::FProviderReadScopeLock>(*ExtraProvider);
			}
		}

		UE_NONCOPYABLE(FSampleReadScopes);

	private:
		TraceServices::FAnalysisSessionReadScope SessionScope;
		TUniquePtr<TraceServices::FProviderReadScopeLock> AnimScope;
		TUniquePtr<TraceServices::FProviderReadScopeLock> GameplayScope;
		TUniquePtr<TraceServices::FProviderReadScopeLock> ExtraScope;
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
