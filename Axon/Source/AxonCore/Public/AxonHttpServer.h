#pragma once

#include "CoreMinimal.h"
#include "HttpRouteHandle.h"
#include "IHttpRouter.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "IPAddress.h"

class FJsonObject;
class FJsonValue;
class FAxonToolRegistry;

/** Visual / logical connection state for the status bar indicator. */
enum class EAxonMcpStatus : uint8
{
	/** Server not listening (disabled, failed bind, or stopped). */
	Off = 0,
	/** Listening, waiting for an MCP client handshake. */
	Listening,
	/** At least one MCP client has successfully talked to the server recently. */
	Connected,
};

/**
 * Embedded MCP HTTP server.
 * Implements Streamable HTTP transport with JSON-RPC 2.0 dispatch.
 */
class AXONCORE_API FAxonHttpServer
{
public:
	FAxonHttpServer();
	~FAxonHttpServer();

	/** Start the HTTP server on the configured port */
	bool Start(int32 Port);

	/** Stop the server and unbind all routes */
	void Stop();

	/** Stop then Start — useful after a silent bind failure */
	bool Restart(int32 Port);

	/** Is the server currently running? */
	bool IsRunning() const { return bIsRunning; }

	/** Get the port the server is listening on */
	int32 GetPort() const { return BoundPort; }

	/**
	 * Status for the editor status-bar indicator.
	 * Off = not listening; Listening = up but no recent client;
	 * Connected = client activity within the inactivity window.
	 */
	EAxonMcpStatus GetMcpStatus() const;

	/** Seconds since last client MCP request; -1 if never. */
	double GetSecondsSinceLastClientActivity() const;

	/** Inactivity window after which Connected falls back to Listening (default 120s). */
	static constexpr double ClientActivityTimeoutSeconds = 120.0;

private:
	bool HandlePostMcp(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleGetMcp(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleDeleteMcp(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleOptions(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleHealthCheck(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

	TSharedPtr<FJsonObject> ProcessJsonRpcRequest(const TSharedPtr<FJsonObject>& Request);
	TSharedPtr<FJsonObject> HandleInitialize(const TSharedPtr<FJsonValue>& Id, const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleToolsList(const TSharedPtr<FJsonValue>& Id, const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleToolsCall(const TSharedPtr<FJsonValue>& Id, const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandlePing(const TSharedPtr<FJsonValue>& Id);

	TUniquePtr<FHttpServerResponse> MakeJsonResponse(const FString& JsonBody, EHttpServerResponseCodes Code = EHttpServerResponseCodes::Ok);
	TUniquePtr<FHttpServerResponse> MakeSseResponse(const TArray<TSharedPtr<FJsonObject>>& Messages);
	void AddCorsHeaders(FHttpServerResponse& Response, const FHttpServerRequest& Request);

	void BindRoutes();
	static bool ProbePort(int32 Port);

	/** Mark that an MCP client successfully reached us (initialize / tools / ping). */
	void NoteClientActivity();

	TSharedPtr<IHttpRouter> HttpRouter;
	TArray<FHttpRouteHandle> RouteHandles;
	int32 BoundPort = 0;
	bool bIsRunning = false;
	FDateTime StartTime;
	FDateTime LastClientActivity;
	bool bHasSeenClient = false;
};
