#pragma once

#include "CoreMinimal.h"
#include "WebSocketsModule.h"
#include "IWebSocketServer.h"
#include "WebSocketServer.generated.h"

DECLARE_DELEGATE_OneParam(FOnCommandReceived, const TSharedPtr<FJsonObject>&);
DECLARE_DELEGATE_OneParam(FOnClientConnected, const FString&);

UCLASS()
class UNREALMCP_API UWebSocketServer : public UObject
{
    GENERATED_BODY()

public:
    UWebSocketServer();

    bool Start(int32 Port = 8080);
    void Stop();

    void SendResponse(const FString& Id, bool bSuccess, const TSharedPtr<FJsonObject>& Data);
    void SendEvent(const FString& EventType, const TSharedPtr<FJsonObject>& Data);

    FOnCommandReceived OnCommandReceived;
    FOnClientConnected OnClientConnected;

    bool IsRunning() const { return bIsRunning; }

private:
    TSharedPtr<IWebSocketServer> Server;
    TSharedPtr<IWebSocket> ConnectedClient;
    bool bIsRunning = false;
    int32 ServerPort = 8080;

    void HandleMessage(const FString& Message);
    FString GenerateUUID();
};
