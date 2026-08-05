#include "AxonKnowledgeRegistry.h"

namespace AxonKnowledgeRegistryPrivate
{
	FCriticalSection& Mutex()
	{
		static FCriticalSection GMutex;
		return GMutex;
	}

	TMap<FString, FAxonKnowledgePackInfo>& Map()
	{
		static TMap<FString, FAxonKnowledgePackInfo> GMap;
		return GMap;
	}
}

void FAxonKnowledgeRegistry::Register(const FString& PluginName, const FString& Namespace)
{
	if (PluginName.IsEmpty() || Namespace.IsEmpty())
	{
		return;
	}
	FScopeLock Lock(&AxonKnowledgeRegistryPrivate::Mutex());
	FAxonKnowledgePackInfo Info;
	Info.PluginName = PluginName;
	Info.Namespace = Namespace;
	AxonKnowledgeRegistryPrivate::Map().Add(PluginName, MoveTemp(Info));
}

void FAxonKnowledgeRegistry::Unregister(const FString& PluginName)
{
	if (PluginName.IsEmpty())
	{
		return;
	}
	FScopeLock Lock(&AxonKnowledgeRegistryPrivate::Mutex());
	AxonKnowledgeRegistryPrivate::Map().Remove(PluginName);
}

TArray<FAxonKnowledgePackInfo> FAxonKnowledgeRegistry::GetRegistered()
{
	FScopeLock Lock(&AxonKnowledgeRegistryPrivate::Mutex());
	TArray<FAxonKnowledgePackInfo> Out;
	AxonKnowledgeRegistryPrivate::Map().GenerateValueArray(Out);
	Out.Sort([](const FAxonKnowledgePackInfo& A, const FAxonKnowledgePackInfo& B)
	{
		return A.PluginName < B.PluginName;
	});
	return Out;
}

TArray<FString> FAxonKnowledgeRegistry::GetRegisteredPluginNames()
{
	TArray<FString> Out;
	for (const FAxonKnowledgePackInfo& Info : GetRegistered())
	{
		Out.Add(Info.PluginName);
	}
	return Out;
}

bool FAxonKnowledgeRegistry::IsRegistered(const FString& PluginName)
{
	FScopeLock Lock(&AxonKnowledgeRegistryPrivate::Mutex());
	return AxonKnowledgeRegistryPrivate::Map().Contains(PluginName);
}
