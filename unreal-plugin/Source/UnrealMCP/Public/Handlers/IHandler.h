#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class IHandler
{
public:
    virtual ~IHandler() = default;

    virtual FString GetPrefix() const = 0;
    virtual TSharedPtr<FJsonObject> Handle(
        const FString& Command,
        const TSharedPtr<FJsonObject>& Params,
        FString& OutError) = 0;
};
