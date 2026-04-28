#include "WebSocketClient.h"
#include "JsonObjectConverter.h"

UWebSocketClient::UWebSocketClient() {}

bool UWebSocketClient::Connect(const FString& Host, int32 Port)
{
    if (bIsConnected)
    {
        return true;
    }

    ServerHost = Host;
    ServerPort = Port;

    const FString ServerURL = FString::Printf(TEXT("ws://%s:%d"), *Host, Port);

    TMap<FString, FString> Headers;
    Headers.Add(TEXT("User-Agent"), TEXT("NeptuneEqicMcp-Unreal-Client/1.0"));

    WebSocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL, TEXT(""), Headers);

    if (!WebSocket.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create WebSocket client"));
        return false;
    }

    WebSocket->OnConnected().AddLambda([this]()
    {
        HandleConnection();
    });

    WebSocket->OnConnectionError().AddLambda([this](const FString& Error)
    {
        UE_LOG(LogTemp, Error, TEXT("WebSocket connection error: %s"), *Error);
        bIsConnected = false;
        OnConnected.Execute(false);
    });

    WebSocket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean)
    {
        HandleDisconnection();
    });

    WebSocket->OnMessage().AddLambda([this](const FString& Message)
    {
        HandleMessage(Message);
    });

    WebSocket->Connect();

    UE_LOG(LogTemp, Log, TEXT("Connecting to cloud MCP server at %s"), *ServerURL);
    return true;
}

void UWebSocketClient::Disconnect()
{
    if (WebSocket.IsValid())
    {
        WebSocket->Close();
        WebSocket.Reset();
    }

    bIsConnected = false;
    UE_LOG(LogTemp, Log, TEXT("WebSocket client disconnected"));
}

void UWebSocketClient::SendResponse(const FString& Id, bool bSuccess, const TSharedPtr<FJsonObject>& Data)
{
    if (!IsConnected())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot send response: not connected"));
        return;
    }

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetStringField(TEXT("id"), Id);
    Response->SetBoolField(TEXT("success"), bSuccess);

    if (bSuccess && Data.IsValid())
    {
        Response->SetObjectField(TEXT("data"), Data);
    }

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    WebSocket->Send(JsonString);
}

void UWebSocketClient::SendEvent(const FString& EventType, const TSharedPtr<FJsonObject>& Data)
{
    if (!IsConnected()) return;

    TSharedPtr<FJsonObject> Event = MakeShareable(new FJsonObject);
    Event->SetStringField(TEXT("event"), EventType);
    Event->SetStringField(TEXT("timestamp"), FDateTime::UtcNow().ToIso8601());

    if (Data.IsValid())
    {
        Event->SetObjectField(TEXT("data"), Data);
    }

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Event.ToSharedRef(), Writer);

    WebSocket->Send(JsonString);
}

void UWebSocketClient::HandleMessage(const FString& Message)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to parse JSON message: %s"), *Message);
        return;
    }

    OnCommandReceived.ExecuteIfBound(JsonObject);
}

void UWebSocketClient::HandleConnection()
{
    bIsConnected = true;
    UE_LOG(LogTemp, Log, TEXT("Connected to cloud MCP server"));
    OnConnected.Execute(true);
}

void UWebSocketClient::HandleDisconnection()
{
    bIsConnected = false;
    UE_LOG(LogTemp, Log, TEXT("Disconnected from cloud MCP server"));
}
