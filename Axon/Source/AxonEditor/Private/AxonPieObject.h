#pragma once

#include "CoreMinimal.h"

class FProperty;
class FJsonValue;
class UObject;
class UWorld;

namespace AxonPieObject
{
	UWorld* FindPieWorld();
	TSharedPtr<FJsonValue> ReadLeafValue(
		const FProperty* Property,
		const void* ValuePtr,
		const UObject* OwnerForExport,
		FString& OutTypeName);
	TSharedPtr<FJsonValue> ReadDottedValue(UObject* Object, const FString& Path, FString& OutTypeName);
}
