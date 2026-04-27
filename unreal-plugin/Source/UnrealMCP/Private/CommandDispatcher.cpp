#include "CommandDispatcher.h"

UCommandDispatcher::UCommandDispatcher() {}

void UCommandDispatcher::RegisterHandler(TSharedPtr<IHandler> Handler)
{
    if (Handler.IsValid())
    {
        Handlers.Add(Handler->GetPrefix(), Handler);
    }
}

TSharedPtr<FJsonObject> UCommandDispatcher::Dispatch(
    const FString& Command,
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    int32 DotIndex;
    if (!Command.FindChar('.', DotIndex))
    {
        OutError = FString::Printf(TEXT("Invalid command format: %s"), *Command);
        return nullptr;
    }

    FString Prefix = Command.Left(DotIndex);
    FString SubCommand = Command.RightChop(DotIndex + 1);

    TSharedPtr<IHandler>* HandlerPtr = Handlers.Find(Prefix);
    if (!HandlerPtr || !HandlerPtr->IsValid())
    {
        OutError = FString::Printf(TEXT("No handler found for prefix: %s"), *Prefix);
        return nullptr;
    }

    return (*HandlerPtr)->Handle(SubCommand, Params, OutError);
}
