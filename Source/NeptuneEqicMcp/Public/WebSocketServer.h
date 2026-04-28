#pragma once

#include "CoreMinimal.h"
#include "IWebSocketNetworkingModule.h"
#include "IWebSocketServer.h"
#include "INetworkingWebSocket.h"
#include "Containers/Ticker.h"
#include "WebSocketServer.generated.h"

DECLARE_DELEGATE_OneParam(FOnCommandReceived, const TSharedPtr<FJsonObject>&);
DECLARE_DELEGATE_OneParam(FOnClientConnected, const FString&);

UCLASS()
class NEPTUNEEQICMCP_API UWebSocketServer : public UObject
{
    GENERATED_BODY()

public:
    UWebSocketServer();
    ~UWebSocketServer();

    bool Start(int32 Port = 8080);
    void Stop();

    void SendResponse(const FString& Id, bool bSuccess, const TSharedPtr<FJsonObject>& Data);
    void SendEvent(const FString& EventType, const TSharedPtr<FJsonObject>& Data);

    FOnCommandReceived OnCommandReceived;
    FOnClientConnected OnClientConnected;

    bool IsRunning() const { return bIsRunning; }

private:
    TUniquePtr<IWebSocketServer> Server;
    TSharedPtr<INetworkingWebSocket> ConnectedClient;
    bool bIsRunning = false;
    int32 ServerPort = 8080;
    FTSTicker::FDelegateHandle TickerHandle;

    void HandleMessage(const FString& Message);
    FString GenerateUUID();
    bool OnTick(float DeltaTime);
};
