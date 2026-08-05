#include "AxonLlmUsage.h"
#include "AxonLLMSettings.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/DateTime.h"

namespace AxonLlmUsagePrivate
{
	FString UsageDir()
	{
		FString Root;
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AxonLLM"));
		if (Plugin.IsValid())
		{
			Root = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Saved"), TEXT("usage"));
		}
		else
		{
			Root = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("AxonLLM"), TEXT("usage"));
		}
		IFileManager::Get().MakeDirectory(*Root, true);
		return Root;
	}
}

void FAxonLlmUsage::Record(
	EAxonLlmScope Scope,
	int32 WorkerIndex,
	const FString& Model,
	int32 CharsIn,
	int32 CharsOut,
	double LatencyMs,
	bool bOk,
	const FString& JobId)
{
	const UAxonLLMSettings* Settings = UAxonLLMSettings::Get();
	if (!Settings || !Settings->bLogUsage)
	{
		return;
	}

	TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
	Row->SetStringField(TEXT("ts"), FDateTime::UtcNow().ToIso8601());
	Row->SetStringField(TEXT("scope"), UAxonLLMSettings::ScopeToWire(Scope));
	Row->SetNumberField(TEXT("worker_index"), WorkerIndex);
	Row->SetStringField(TEXT("model"), Model);
	Row->SetNumberField(TEXT("chars_in"), CharsIn);
	Row->SetNumberField(TEXT("chars_out"), CharsOut);
	Row->SetNumberField(TEXT("latency_ms"), LatencyMs);
	Row->SetBoolField(TEXT("ok"), bOk);
	if (!JobId.IsEmpty())
	{
		Row->SetStringField(TEXT("job_id"), JobId);
	}

	FString Line;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Line);
	FJsonSerializer::Serialize(Row.ToSharedRef(), Writer);
	Line += LINE_TERMINATOR;

	const FString Path = FPaths::Combine(
		AxonLlmUsagePrivate::UsageDir(),
		FString::Printf(TEXT("usage-%s.jsonl"), *FDateTime::Now().ToString(TEXT("%Y%m%d"))));
	FFileHelper::SaveStringToFile(
		Line, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append);
}

TSharedPtr<FJsonObject> FAxonLlmUsage::BuildSummary(int32 LastDays)
{
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	const int32 Days = FMath::Clamp(LastDays, 1, 30);
	int32 Total = 0;
	int32 OkCount = 0;
	int64 CharsIn = 0;
	int64 CharsOut = 0;
	TMap<FString, int32> ByScope;

	const FString Dir = AxonLlmUsagePrivate::UsageDir();
	for (int32 Offset = 0; Offset < Days; ++Offset)
	{
		const FDateTime Day = FDateTime::Now() - FTimespan::FromDays(Offset);
		const FString Path = FPaths::Combine(Dir, FString::Printf(TEXT("usage-%s.jsonl"), *Day.ToString(TEXT("%Y%m%d"))));
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			continue;
		}
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, false);
		for (const FString& Line : Lines)
		{
			if (Line.TrimStartAndEnd().IsEmpty())
			{
				continue;
			}
			TSharedPtr<FJsonObject> Row;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Line);
			if (!FJsonSerializer::Deserialize(Reader, Row) || !Row.IsValid())
			{
				continue;
			}
			++Total;
			bool bOk = false;
			Row->TryGetBoolField(TEXT("ok"), bOk);
			if (bOk)
			{
				++OkCount;
			}
			double Cin = 0, Cout = 0;
			Row->TryGetNumberField(TEXT("chars_in"), Cin);
			Row->TryGetNumberField(TEXT("chars_out"), Cout);
			CharsIn += static_cast<int64>(Cin);
			CharsOut += static_cast<int64>(Cout);
			FString Scope;
			Row->TryGetStringField(TEXT("scope"), Scope);
			ByScope.FindOrAdd(Scope)++;
		}
	}

	Out->SetNumberField(TEXT("days"), Days);
	Out->SetNumberField(TEXT("calls"), Total);
	Out->SetNumberField(TEXT("ok_calls"), OkCount);
	Out->SetNumberField(TEXT("chars_in"), static_cast<double>(CharsIn));
	Out->SetNumberField(TEXT("chars_out"), static_cast<double>(CharsOut));
	TSharedPtr<FJsonObject> ScopeObj = MakeShared<FJsonObject>();
	for (const auto& Pair : ByScope)
	{
		ScopeObj->SetNumberField(Pair.Key, Pair.Value);
	}
	Out->SetObjectField(TEXT("by_scope"), ScopeObj);
	return Out;
}
