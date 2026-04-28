#include "NeptuneEqicMcp.h"
#include "WebSocketServer.h"
#include "WebSocketClient.h"
#include "CommandDispatcher.h"
#include "Handlers/ActorHandler.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"

#define LOCTEXT_NAMESPACE "FNeptuneEqicMcpModule"

// 配置结构
struct FMcpConfig
{
    FString Mode; // "local" or "cloud"
    FString LocalHost;
    int32 LocalPort;
    FString CloudHost;
    int32 CloudPort;
};

static UWebSocketServer* GWebSocketServer = nullptr;
static UWebSocketClient* GWebSocketClient = nullptr;
static UCommandDispatcher* GCommandDispatcher = nullptr;
static FMcpConfig GConfig;

// 读取配置文件
FMcpConfig LoadConfig()
{
    FMcpConfig Config;
    Config.Mode = TEXT("local");
    Config.LocalHost = TEXT("localhost");
    Config.LocalPort = 18765;
    Config.CloudHost = TEXT("localhost");
    Config.CloudPort = 18765;

    // 尝试读取项目根目录的 mcp-config.json
    FString ConfigPath = FPaths::ProjectDir() / TEXT("mcp-config.json");

    if (FPaths::FileExists(ConfigPath))
    {
        FString JsonString;
        if (FFileHelper::LoadFileToString(JsonString, *ConfigPath))
        {
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                JsonObject->TryGetStringField(TEXT("mode"), Config.Mode);

                const TSharedPtr<FJsonObject>* LocalConfig;
                if (JsonObject->TryGetObjectField(TEXT("local"), LocalConfig))
                {
                    (*LocalConfig)->TryGetStringField(TEXT("host"), Config.LocalHost);
                    (*LocalConfig)->TryGetNumberField(TEXT("port"), Config.LocalPort);
                }

                const TSharedPtr<FJsonObject>* CloudConfig;
                if (JsonObject->TryGetObjectField(TEXT("cloud"), CloudConfig))
                {
                    (*CloudConfig)->TryGetStringField(TEXT("host"), Config.CloudHost);
                    (*CloudConfig)->TryGetNumberField(TEXT("port"), Config.CloudPort);
                }

                UE_LOG(LogTemp, Log, TEXT("Loaded mcp-config.json: mode=%s"), *Config.Mode);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("mcp-config.json not found, using default local mode"));
    }

    return Config;
}

void FNeptuneEqicMcpModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("NeptuneEqicMcp Module starting up..."));

    // 加载配置
    GConfig = LoadConfig();

    // 初始化命令分发器
    GCommandDispatcher = NewObject<UCommandDispatcher>();
    GCommandDispatcher->AddToRoot();

    TSharedPtr<IHandler> ActorHandler = MakeShareable(new UActorHandler());
    GCommandDispatcher->RegisterHandler(ActorHandler);

    // 命令处理函数
    auto HandleCommand = [](const TSharedPtr<FJsonObject>& Command)
    {
        FString Id, Cmd;
        Command->TryGetStringField("id", Id);
        Command->TryGetStringField("cmd", Cmd);

        const TSharedPtr<FJsonObject>* Params;
        Command->TryGetObjectField("params", Params);

        FString Error;
        TSharedPtr<FJsonObject> Result = GCommandDispatcher->Dispatch(Cmd, Params ? *Params : MakeShareable(new FJsonObject), Error);

        if (GConfig.Mode == TEXT("local") && GWebSocketServer)
        {
            if (Result.IsValid())
            {
                GWebSocketServer->SendResponse(Id, true, Result);
            }
            else
            {
                TSharedPtr<FJsonObject> ErrorObj = MakeShareable(new FJsonObject);
                ErrorObj->SetStringField("code", "COMMAND_FAILED");
                ErrorObj->SetStringField("type", "runtime_error");
                ErrorObj->SetStringField("message", Error);
                GWebSocketServer->SendResponse(Id, false, ErrorObj);
            }
        }
        else if (GConfig.Mode == TEXT("cloud") && GWebSocketClient)
        {
            if (Result.IsValid())
            {
                GWebSocketClient->SendResponse(Id, true, Result);
            }
            else
            {
                TSharedPtr<FJsonObject> ErrorObj = MakeShareable(new FJsonObject);
                ErrorObj->SetStringField("code", "COMMAND_FAILED");
                ErrorObj->SetStringField("type", "runtime_error");
                ErrorObj->SetStringField("message", Error);
                GWebSocketClient->SendResponse(Id, false, ErrorObj);
            }
        }
    };

    if (GConfig.Mode == TEXT("local"))
    {
        // 本地模式：启动 WebSocket 服务器
        UE_LOG(LogTemp, Log, TEXT("Starting in LOCAL mode"));

        GWebSocketServer = NewObject<UWebSocketServer>();
        GWebSocketServer->AddToRoot();

        GWebSocketServer->OnCommandReceived.BindLambda(HandleCommand);

        if (GWebSocketServer->Start(GConfig.LocalPort))
        {
            UE_LOG(LogTemp, Log, TEXT("WebSocket server started on port %d"), GConfig.LocalPort);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to start WebSocket server"));
        }
    }
    else
    {
        // 云端模式：作为客户端连接云服务器
        UE_LOG(LogTemp, Log, TEXT("Starting in CLOUD mode"));

        GWebSocketClient = NewObject<UWebSocketClient>();
        GWebSocketClient->AddToRoot();

        GWebSocketClient->OnCommandReceived.BindLambda(HandleCommand);

        GWebSocketClient->OnConnected.BindLambda([](bool bSuccess)
        {
            if (bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Connected to cloud MCP server"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to connect to cloud MCP server"));
            }
        });

        if (GWebSocketClient->Connect(GConfig.CloudHost, GConfig.CloudPort))
        {
            UE_LOG(LogTemp, Log, TEXT("Connecting to cloud MCP server at %s:%d"), *GConfig.CloudHost, GConfig.CloudPort);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to initiate connection to cloud MCP server"));
        }
    }

    bInitialized = true;
}

void FNeptuneEqicMcpModule::ShutdownModule()
{
    if (!bInitialized) return;

    UE_LOG(LogTemp, Log, TEXT("NeptuneEqicMcp Module shutting down..."));

    if (GWebSocketServer)
    {
        GWebSocketServer->Stop();
        GWebSocketServer->RemoveFromRoot();
        GWebSocketServer = nullptr;
    }

    if (GWebSocketClient)
    {
        GWebSocketClient->Disconnect();
        GWebSocketClient->RemoveFromRoot();
        GWebSocketClient = nullptr;
    }

    if (GCommandDispatcher)
    {
        GCommandDispatcher->RemoveFromRoot();
        GCommandDispatcher = nullptr;
    }

    bInitialized = false;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNeptuneEqicMcpModule, NeptuneEqicMcp)
