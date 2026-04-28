#pragma once

#include "CoreMinimal.h"
#include "Handlers/IHandler.h"
#include "CommandDispatcher.generated.h"

UCLASS()
class UNREALMCP_API UCommandDispatcher : public UObject
{
    GENERATED_BODY()

public:
    UCommandDispatcher();

    void RegisterHandler(TSharedPtr<IHandler> Handler);
    TSharedPtr<FJsonObject> Dispatch(
        const FString& Command,
        const TSharedPtr<FJsonObject>& Params,
        FString& OutError);

private:
    TMap<FString, TSharedPtr<IHandler>> Handlers;
};
