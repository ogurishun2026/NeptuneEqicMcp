#include "UnrealMCP.h"
#include "WebSocketServer.h"
#include "CommandDispatcher.h"
#include "Handlers/ActorHandler.h"

#define LOCTEXT_NAMESPACE "FUnrealMCPModule"

static UWebSocketServer* GWebSocketServer = nullptr;
static UCommandDispatcher* GCommandDispatcher = nullptr;

void FUnrealMCPModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("UnrealMCP Module starting up..."));

    GCommandDispatcher = NewObject<UCommandDispatcher>();
    GCommandDispatcher->AddToRoot();

    TSharedPtr<IHandler> ActorHandler = MakeShareable(new UActorHandler());
    GCommandDispatcher->RegisterHandler(ActorHandler);

    GWebSocketServer = NewObject<UWebSocketServer>();
    GWebSocketServer->AddToRoot();

    GWebSocketServer->OnCommandReceived.BindLambda([](const TSharedPtr<FJsonObject>& Command)
    {
        FString Id, Cmd;
        Command->TryGetStringField("id", Id);
        Command->TryGetStringField("cmd", Cmd);

        const TSharedPtr<FJsonObject>* Params;
        Command->TryGetObjectField("params", Params);

        FString Error;
        TSharedPtr<FJsonObject> Result = GCommandDispatcher->Dispatch(Cmd, Params ? *Params : MakeShareable(new FJsonObject), Error);

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
    });

    if (GWebSocketServer->Start(8080))
    {
        UE_LOG(LogTemp, Log, TEXT("WebSocket server started on port 8080"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start WebSocket server"));
    }

    bInitialized = true;
}

void FUnrealMCPModule::ShutdownModule()
{
    if (!bInitialized) return;

    UE_LOG(LogTemp, Log, TEXT("UnrealMCP Module shutting down..."));

    if (GWebSocketServer)
    {
        GWebSocketServer->Stop();
        GWebSocketServer->RemoveFromRoot();
        GWebSocketServer = nullptr;
    }

    if (GCommandDispatcher)
    {
        GCommandDispatcher->RemoveFromRoot();
        GCommandDispatcher = nullptr;
    }

    bInitialized = false;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPModule, UnrealMCP)
