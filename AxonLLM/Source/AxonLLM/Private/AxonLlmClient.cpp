#include "AxonLlmClient.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

namespace AxonLlmClientPrivate
{
	FString NormalizeBaseUrl(const FString& In)
	{
		FString Url = In.TrimStartAndEnd();
		while (Url.EndsWith(TEXT("/")))
		{
			Url.LeftChopInline(1);
		}
		return Url;
	}
}

FString FAxonLlmClient::BuildChatBody(
	const FAxonLlmWorkerProfile& Worker,
	const FString& SystemPrompt,
	const FString& UserPrompt)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), Worker.Model);
	Root->SetBoolField(TEXT("stream"), false);

	TArray<TSharedPtr<FJsonValue>> Messages;
	if (!SystemPrompt.IsEmpty())
	{
		TSharedPtr<FJsonObject> Sys = MakeShared<FJsonObject>();
		Sys->SetStringField(TEXT("role"), TEXT("system"));
		Sys->SetStringField(TEXT("content"), SystemPrompt);
		Messages.Add(MakeShared<FJsonValueObject>(Sys));
	}
	{
		TSharedPtr<FJsonObject> User = MakeShared<FJsonObject>();
		User->SetStringField(TEXT("role"), TEXT("user"));
		User->SetStringField(TEXT("content"), UserPrompt);
		Messages.Add(MakeShared<FJsonValueObject>(User));
	}
	Root->SetArrayField(TEXT("messages"), Messages);

	TSharedPtr<FJsonObject> Options = MakeShared<FJsonObject>();
	Options->SetNumberField(TEXT("num_predict"), Worker.MaxOutputTokens);
	Root->SetObjectField(TEXT("options"), Options);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	return Body;
}

FAxonLlmChatResult FAxonLlmClient::ParseChatResponse(const FString& ResponseBody, int32 CharsIn, double LatencyMs)
{
	FAxonLlmChatResult Result;
	Result.CharsIn = CharsIn;
	Result.LatencyMs = LatencyMs;

	TSharedPtr<FJsonObject> Resp;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, Resp) || !Resp.IsValid())
	{
		Result.Error = TEXT("Failed to parse /api/chat JSON");
		return Result;
	}

	FString Content;
	const TSharedPtr<FJsonObject>* MessageObj = nullptr;
	if (Resp->TryGetObjectField(TEXT("message"), MessageObj) && MessageObj)
	{
		(*MessageObj)->TryGetStringField(TEXT("content"), Content);
	}
	if (Content.IsEmpty())
	{
		Resp->TryGetStringField(TEXT("response"), Content);
	}

	Content = StripThinking(Content);
	if (Content.IsEmpty())
	{
		Result.Error = TEXT("Model returned empty content");
		return Result;
	}

	Result.bOk = true;
	Result.Content = MoveTemp(Content);
	Result.CharsOut = Result.Content.Len();
	return Result;
}

bool FAxonLlmClient::HttpExchange(
	const FString& Verb,
	const FString& Url,
	const FString& ContentType,
	const FString& Body,
	int32 TimeoutSec,
	int32& OutHttpCode,
	FString& OutBody,
	FString& OutError,
	double& OutLatencyMs)
{
	OutHttpCode = 0;
	OutBody.Reset();
	OutError.Reset();
	OutLatencyMs = 0.0;

	const double Start = FPlatformTime::Seconds();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(Verb);
	Request->SetTimeout(static_cast<float>(FMath::Max(5, TimeoutSec)));
	if (!ContentType.IsEmpty())
	{
		Request->SetHeader(TEXT("Content-Type"), ContentType);
	}
	if (!Body.IsEmpty())
	{
		Request->SetContentAsString(Body);
	}

	struct FHttpWaitState
	{
		bool bDone = false;
		int32 Code = 0;
		FString Body;
		FString Error;
	};
	const TSharedRef<FHttpWaitState> State = MakeShared<FHttpWaitState>();

	Request->OnProcessRequestComplete().BindLambda(
		[State](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				State->Error = TEXT("HTTP connection failed");
			}
			else
			{
				State->Code = Response->GetResponseCode();
				State->Body = Response->GetContentAsString();
				if (State->Code < 200 || State->Code >= 300)
				{
					State->Error = FString::Printf(TEXT("HTTP %d: %s"), State->Code,
						*State->Body.Left(400));
				}
			}
			State->bDone = true;
		});

	if (!Request->ProcessRequest())
	{
		OutError = TEXT("Failed to start HTTP request");
		return false;
	}

	const double Deadline = Start + static_cast<double>(FMath::Max(5, TimeoutSec));
	while (!State->bDone && FPlatformTime::Seconds() < Deadline)
	{
		FHttpModule::Get().GetHttpManager().Tick(0.f);
		FPlatformProcess::Sleep(0.01f);
	}

	OutLatencyMs = (FPlatformTime::Seconds() - Start) * 1000.0;

	if (!State->bDone)
	{
		Request->CancelRequest();
		OutError = FString::Printf(TEXT("HTTP timed out after %d s"), TimeoutSec);
		return false;
	}

	OutHttpCode = State->Code;
	OutBody = MoveTemp(State->Body);
	OutError = MoveTemp(State->Error);
	return OutError.IsEmpty();
}

FAxonLlmTagsResult FAxonLlmClient::GetTags(const FString& BaseUrl, int32 TimeoutSec)
{
	FAxonLlmTagsResult Result;
	const FString Url = AxonLlmClientPrivate::NormalizeBaseUrl(BaseUrl) + TEXT("/api/tags");

	int32 Code = 0;
	FString Body;
	FString Err;
	double Latency = 0.0;
	if (!HttpExchange(TEXT("GET"), Url, FString(), FString(), TimeoutSec, Code, Body, Err, Latency))
	{
		Result.Error = Err.IsEmpty() ? TEXT("tags request failed") : Err;
		Result.LatencyMs = Latency;
		return Result;
	}

	Result.LatencyMs = Latency;
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		Result.Error = TEXT("Failed to parse /api/tags JSON");
		return Result;
	}

	const TArray<TSharedPtr<FJsonValue>>* Models = nullptr;
	if (Root->TryGetArrayField(TEXT("models"), Models) && Models)
	{
		for (const TSharedPtr<FJsonValue>& V : *Models)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (V.IsValid() && V->TryGetObject(Obj) && Obj)
			{
				FString Name;
				if ((*Obj)->TryGetStringField(TEXT("name"), Name) && !Name.IsEmpty())
				{
					Result.Models.Add(Name);
				}
			}
		}
	}

	Result.bOk = true;
	return Result;
}

FString FAxonLlmClient::StripThinking(const FString& Text)
{
	FString Out = Text;

	auto StripTagPair = [&Out](const TCHAR* OpenTag, const TCHAR* CloseTag)
	{
		for (;;)
		{
			const int32 Open = Out.Find(OpenTag, ESearchCase::IgnoreCase);
			if (Open == INDEX_NONE)
			{
				break;
			}
			const int32 Close = Out.Find(CloseTag, ESearchCase::IgnoreCase, ESearchDir::FromStart, Open);
			if (Close == INDEX_NONE)
			{
				Out.LeftInline(Open);
				break;
			}
			Out.RemoveAt(Open, (Close - Open) + FCString::Strlen(CloseTag));
		}
	};

	StripTagPair(TEXT("<think>"), TEXT("</think>"));
	StripTagPair(TEXT("<thinking>"), TEXT("</thinking>"));

	return Out.TrimStartAndEnd();
}

FAxonLlmChatResult FAxonLlmClient::Chat(
	const FAxonLlmWorkerProfile& Worker,
	const FString& SystemPrompt,
	const FString& UserPrompt)
{
	const int32 CharsIn = SystemPrompt.Len() + UserPrompt.Len();
	const FString Body = BuildChatBody(Worker, SystemPrompt, UserPrompt);
	const FString Url = AxonLlmClientPrivate::NormalizeBaseUrl(Worker.BaseUrl) + TEXT("/api/chat");

	int32 Code = 0;
	FString ResponseBody;
	FString Err;
	double Latency = 0.0;
	if (!HttpExchange(TEXT("POST"), Url, TEXT("application/json"), Body, Worker.TimeoutSec, Code, ResponseBody, Err, Latency))
	{
		FAxonLlmChatResult Result;
		Result.CharsIn = CharsIn;
		Result.Error = Err.IsEmpty() ? TEXT("chat request failed") : Err;
		Result.LatencyMs = Latency;
		return Result;
	}

	return ParseChatResponse(ResponseBody, CharsIn, Latency);
}

void FAxonLlmClient::ChatAsync(
	const FAxonLlmWorkerProfile& Worker,
	const FString& SystemPrompt,
	const FString& UserPrompt,
	FAxonLlmChatComplete OnComplete)
{
	const int32 CharsIn = SystemPrompt.Len() + UserPrompt.Len();
	const FString Body = BuildChatBody(Worker, SystemPrompt, UserPrompt);
	const FString Url = AxonLlmClientPrivate::NormalizeBaseUrl(Worker.BaseUrl) + TEXT("/api/chat");
	const double Start = FPlatformTime::Seconds();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetTimeout(static_cast<float>(FMath::Max(5, Worker.TimeoutSec)));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(Body);

	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete, CharsIn, Start](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedSuccessfully)
		{
			const double Latency = (FPlatformTime::Seconds() - Start) * 1000.0;
			FAxonLlmChatResult Result;
			Result.CharsIn = CharsIn;
			Result.LatencyMs = Latency;

			if (!bConnectedSuccessfully || !Response.IsValid())
			{
				Result.Error = TEXT("HTTP connection failed");
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			const int32 Code = Response->GetResponseCode();
			const FString ResponseBody = Response->GetContentAsString();
			if (Code < 200 || Code >= 300)
			{
				Result.Error = FString::Printf(TEXT("HTTP %d: %s"), Code, *ResponseBody.Left(400));
				OnComplete.ExecuteIfBound(Result);
				return;
			}

			Result = ParseChatResponse(ResponseBody, CharsIn, Latency);
			OnComplete.ExecuteIfBound(Result);
		});

	if (!Request->ProcessRequest())
	{
		FAxonLlmChatResult Result;
		Result.CharsIn = CharsIn;
		Result.Error = TEXT("Failed to start HTTP request");
		OnComplete.ExecuteIfBound(Result);
	}
}
