#pragma once

#include "CoreMinimal.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "WebSocketClient.generated.h"

DECLARE_DELEGATE_OneParam(FOnClientConnected, bool);
DECLARE_DELEGATE_OneParam(FOnClientCommandReceived, const TSharedPtr<FJsonObject>&);

/**
 * WebSocket 客户端 - 用于云端模式
 * 主动连接云端 MCP Server
 */
UCLASS()
class NEPTUNEEQICMCP_API UWebSocketClient : public UObject
{
    GENERATED_BODY()

public:
    UWebSocketClient();

    // 连接到云端服务器
    bool Connect(const FString& Host, int32 Port);
    void Disconnect();

    // 发送命令响应
    void SendResponse(const FString& Id, bool bSuccess, const TSharedPtr<FJsonObject>& Data);

    // 发送事件
    void SendEvent(const FString& EventType, const TSharedPtr<FJsonObject>& Data);

    bool IsConnected() const { return bIsConnected && WebSocket.IsValid() && WebSocket->GetState() == EWebSocketState::Connected; }

    FOnClientConnected OnConnected;
    FOnClientCommandReceived OnCommandReceived;

private:
    TSharedPtr<IWebSocket> WebSocket;
    bool bIsConnected = false;
    FString ServerHost;
    int32 ServerPort = 18765;

    void HandleMessage(const FString& Message);
    void HandleConnection();
    void HandleDisconnection();
};
