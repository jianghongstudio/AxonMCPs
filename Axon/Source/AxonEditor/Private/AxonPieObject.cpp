#include "AxonPieObject.h"

#include "AxonStructFieldResolver.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Dom/JsonValue.h"
#include "UObject/UnrealType.h"

namespace AxonPieObject
{
	UWorld* FindPieWorld()
	{
		if (!GEditor)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEditor->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	TSharedPtr<FJsonValue> ReadLeafValue(
		const FProperty* Property,
		const void* ValuePtr,
		const UObject* OwnerForExport,
		FString& OutTypeName)
	{
		if (!Property || !ValuePtr)
		{
			OutTypeName = TEXT("<not found>");
			return nullptr;
		}

		OutTypeName = Property->GetCPPType(nullptr, 0u);
		if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
		{
			return MakeShared<FJsonValueBoolean>(BoolProperty->GetPropertyValue(ValuePtr));
		}
		if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
		{
			return MakeShared<FJsonValueNumber>(FloatProperty->GetPropertyValue(ValuePtr));
		}
		if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
		{
			return MakeShared<FJsonValueNumber>(DoubleProperty->GetPropertyValue(ValuePtr));
		}
		if (const FIntProperty* IntProperty = CastField<FIntProperty>(Property))
		{
			return MakeShared<FJsonValueNumber>(IntProperty->GetPropertyValue(ValuePtr));
		}
		if (const FInt64Property* Int64Property = CastField<FInt64Property>(Property))
		{
			return MakeShared<FJsonValueNumber>(static_cast<double>(Int64Property->GetPropertyValue(ValuePtr)));
		}
		if (const FStrProperty* StringProperty = CastField<FStrProperty>(Property))
		{
			return MakeShared<FJsonValueString>(StringProperty->GetPropertyValue(ValuePtr));
		}
		if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			return MakeShared<FJsonValueString>(NameProperty->GetPropertyValue(ValuePtr).ToString());
		}
		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			UObject* Value = ObjectProperty->GetObjectPropertyValue(ValuePtr);
			return MakeShared<FJsonValueString>(Value ? Value->GetPathName() : TEXT("None"));
		}

		FString Exported;
		Property->ExportTextItem_Direct(Exported, ValuePtr, nullptr, const_cast<UObject*>(OwnerForExport), PPF_None);
		return MakeShared<FJsonValueString>(Exported);
	}

	TSharedPtr<FJsonValue> ReadDottedValue(UObject* Object, const FString& Path, FString& OutTypeName)
	{
		if (!Object)
		{
			OutTypeName = TEXT("<not found>");
			return nullptr;
		}

		const AxonStructField::FResolved Resolved = AxonStructField::Resolve(Object, Path);
		if (!Resolved.Leaf)
		{
			OutTypeName = FString::Printf(TEXT("<unresolved: %s>"), *Resolved.FailedSegment);
			return nullptr;
		}
		return ReadLeafValue(Resolved.Leaf, Resolved.ValuePtr, Object, OutTypeName);
	}
}
