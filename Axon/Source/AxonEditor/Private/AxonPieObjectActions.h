#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"
#include "AxonPieObject.h"

class UObject;
class AActor;
class FJsonObject;

// Shared PIE-object resolution helpers for live-PIE actions
// (pie_get_object_properties / pie_call_function) and sample_pie_timeseries.
namespace AxonPieObject
{
	struct FResolvedObject
	{
		UObject* Object = nullptr;
		AActor* Actor = nullptr;
		FString ResolvedName;
		bool bSuccess = false;
		FString Error;
	};

	FResolvedObject Resolve(const TSharedPtr<FJsonObject>& Params);
}

class FAxonPieObjectActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleGetObjectProperties(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCallFunction(const TSharedPtr<FJsonObject>& Params);
};
