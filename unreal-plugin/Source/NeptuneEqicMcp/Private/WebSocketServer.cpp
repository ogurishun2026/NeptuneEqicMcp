#include "WebSocketServer.h"
#include "JsonObjectConverter.h"
#include "Misc/Guid.h"

UWebSocketServer::UWebSocketServer() {}

bool UWebSocketServer::Start(int32 Port)
{
    if (bIsRunning) return true;

    ServerPort = Port;
    FWebSocketServerConfig Config;
    Config.Port = Port;

    Server = IWebSocketServer::Create(Config);
    if (!Server.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create WebSocket server"));
        return false;
    }

    Server->OnClientConnected().BindLambda([this](TSharedPtr<IWebSocket> Client)
    {
        ConnectedClient = Client;

        Client->OnMessage().BindLambda([this](const FString& Message) { HandleMessage(Message); });

        Client->OnClosed().BindLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean)
        {
            UE_LOG(LogTemp, Log, TEXT("Client disconnected: %s"), *Reason);
            ConnectedClient.Reset();
        });

        UE_LOG(LogTemp, Log, TEXT("Client connected"));
        OnClientConnected.ExecuteIfBound(Client->GetRemoteEndpoint());
    });

    if (!Server->Start())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start WebSocket server on port %d"), Port);
        return false;
    }

    bIsRunning = true;
    UE_LOG(LogTemp, Log, TEXT("WebSocketServer started on port %d"), Port);
    return true;
}

void UWebSocketServer::Stop()
{
    if (!bIsRunning) return;

    if (ConnectedClient.IsValid())
    {
        ConnectedClient->Close();
        ConnectedClient.Reset();
    }

    if (Server.IsValid())
    {
        Server->Stop();
        Server.Reset();
    }

    bIsRunning = false;
    UE_LOG(LogTemp, Log, TEXT("WebSocketServer stopped"));
}

void UWebSocketServer::SendResponse(const FString& Id, bool bSuccess, const TSharedPtr<FJsonObject>& Data)
{
    if (!ConnectedClient.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("No client connected"));
        return;
    }

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetStringField("id", Id);
    Response->SetBoolField("success", bSuccess);

    if (bSuccess && Data.IsValid()) Response->SetObjectField("data", Data);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    ConnectedClient->Send(JsonString);
}

void UWebSocketServer::SendEvent(const FString& EventType, const TSharedPtr<FJsonObject>& Data)
{
    if (!ConnectedClient.IsValid()) return;

    TSharedPtr<FJsonObject> Event = MakeShareable(new FJsonObject);
    Event->SetStringField("event", EventType);
    Event->SetStringField("timestamp", FDateTime::UtcNow().ToIso8601());

    if (Data.IsValid()) Event->SetObjectField("data", Data);

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Event.ToSharedRef(), Writer);

    ConnectedClient->Send(JsonString);
}

void UWebSocketServer::HandleMessage(const FString& Message)
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

FString UWebSocketServer::GenerateUUID()
{
    return FGuid::NewGuid().ToString(EGuidFormats::DigitsLower);
}
