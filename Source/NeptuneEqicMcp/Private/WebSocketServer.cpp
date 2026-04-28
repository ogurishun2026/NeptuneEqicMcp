#include "WebSocketServer.h"
#include "JsonObjectConverter.h"
#include "Misc/Guid.h"
#include "IWebSocketNetworkingModule.h"

UWebSocketServer::UWebSocketServer() {}
UWebSocketServer::~UWebSocketServer()
{
    Stop();
}

bool UWebSocketServer::Start(int32 Port)
{
    if (bIsRunning) return true;

    ServerPort = Port;

    IWebSocketNetworkingModule& Module = FModuleManager::LoadModuleChecked<IWebSocketNetworkingModule>(TEXT("WebSocketNetworking"));
    Server = Module.CreateServer();

    if (!Server.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create WebSocket server"));
        return false;
    }

    FWebSocketClientConnectedCallBack ConnectedCallback = FWebSocketClientConnectedCallBack::CreateLambda(
        [this](INetworkingWebSocket* Socket)
        {
            ConnectedClient = MakeShareable(Socket);

            Socket->SetReceiveCallBack(FWebSocketPacketReceivedCallBack::CreateLambda(
                [this](void* Data, int32 DataSize)
                {
                    UE_LOG(LogTemp, Log, TEXT("Received %d bytes"), DataSize);

                    // Convert data to FString with correct length
                    FString Message;
                    Message.AppendChars(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Data)), DataSize);

                    UE_LOG(LogTemp, Log, TEXT("Message: %s"), *Message);
                    HandleMessage(Message);
                }
            ));

            Socket->SetSocketClosedCallBack(FWebSocketInfoCallBack::CreateLambda([this]()
            {
                UE_LOG(LogTemp, Log, TEXT("Client disconnected"));
                ConnectedClient.Reset();
            }));

            FString ClientEndpoint = Socket->RemoteEndPoint(true);
            UE_LOG(LogTemp, Log, TEXT("Client connected: %s"), *ClientEndpoint);
            OnClientConnected.ExecuteIfBound(ClientEndpoint);
        }
    );

    FString BindAddress = TEXT("");
    if (!Server->Init(Port, ConnectedCallback, BindAddress))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start WebSocket server on port %d"), Port);
        Server.Reset();
        return false;
    }

    // Register ticker to tick the server
    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UWebSocketServer::OnTick),
        0.01f
    );

    bIsRunning = true;
    UE_LOG(LogTemp, Log, TEXT("WebSocket server started on port %d"), Port);
    return true;
}

void UWebSocketServer::Stop()
{
    if (!bIsRunning) return;

    if (TickerHandle.IsValid())
    {
        FTSTicker::RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }

    ConnectedClient.Reset();

    if (Server.IsValid())
    {
        Server.Reset();
    }

    bIsRunning = false;
    UE_LOG(LogTemp, Log, TEXT("WebSocket server stopped"));
}

bool UWebSocketServer::OnTick(float DeltaTime)
{
    if (Server.IsValid())
    {
        Server->Tick();
    }
    return true;
}

void UWebSocketServer::SendResponse(const FString& Id, bool bSuccess, const TSharedPtr<FJsonObject>& Data)
{
    if (!ConnectedClient.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("No client connected"));
        return;
    }

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetStringField(TEXT("id"), Id);
    Response->SetBoolField(TEXT("success"), bSuccess);

    if (bSuccess && Data.IsValid())
    {
        Response->SetObjectField(TEXT("data"), Data);
    }
    else if (!bSuccess && Data.IsValid())
    {
        Response->SetObjectField(TEXT("error"), Data);
    }

    FString JsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

    FTCHARToUTF8 Utf8Converter(*JsonString);
    ConnectedClient->Send(reinterpret_cast<const uint8*>(Utf8Converter.Get()), Utf8Converter.Length());
}

void UWebSocketServer::SendEvent(const FString& EventType, const TSharedPtr<FJsonObject>& Data)
{
    if (!ConnectedClient.IsValid()) return;

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

    FTCHARToUTF8 Utf8Converter(*JsonString);
    ConnectedClient->Send(reinterpret_cast<const uint8*>(Utf8Converter.Get()), Utf8Converter.Length());
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
