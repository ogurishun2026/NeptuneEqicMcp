#include "NeptuneEqicMcp.h"
#include "WebSocketServer.h"
#include "CommandDispatcher.h"
#include "Handlers/ActorHandler.h"

#define LOCTEXT_NAMESPACE "FNeptuneEqicMcpModule"

static UWebSocketServer* GWebSocketServer = nullptr;
static UCommandDispatcher* GCommandDispatcher = nullptr;

void FNeptuneEqicMcpModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("NeptuneEqicMcp Module starting up..."));

    GCommandDispatcher = NewObject<UCommandDispatcher>(GetTransientPackage());
    GCommandDispatcher->AddToRoot();

    UActorHandler* ActorHandlerObj = NewObject<UActorHandler>(GetTransientPackage());
    ActorHandlerObj->AddToRoot();
    TSharedPtr<IHandler> ActorHandler(ActorHandlerObj);
    GCommandDispatcher->RegisterHandler(ActorHandler);

    GWebSocketServer = NewObject<UWebSocketServer>(GetTransientPackage());
    GWebSocketServer->AddToRoot();

    GWebSocketServer->OnCommandReceived.BindLambda([](const TSharedPtr<FJsonObject>& Command)
    {
        FString Id, Cmd;
        Command->TryGetStringField(TEXT("id"), Id);
        Command->TryGetStringField(TEXT("cmd"), Cmd);

        const TSharedPtr<FJsonObject>* Params;
        Command->TryGetObjectField(TEXT("params"), Params);

        FString Error;
        TSharedPtr<FJsonObject> Result = GCommandDispatcher->Dispatch(Cmd, Params ? *Params : MakeShareable(new FJsonObject), Error);

        if (Result.IsValid())
        {
            GWebSocketServer->SendResponse(Id, true, Result);
        }
        else
        {
            TSharedPtr<FJsonObject> ErrorObj = MakeShareable(new FJsonObject);
            ErrorObj->SetStringField(TEXT("code"), TEXT("COMMAND_FAILED"));
            ErrorObj->SetStringField(TEXT("type"), TEXT("runtime_error"));
            ErrorObj->SetStringField(TEXT("message"), Error);
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

    if (GCommandDispatcher)
    {
        GCommandDispatcher->RemoveFromRoot();
        GCommandDispatcher = nullptr;
    }

    bInitialized = false;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNeptuneEqicMcpModule, NeptuneEqicMcp)
