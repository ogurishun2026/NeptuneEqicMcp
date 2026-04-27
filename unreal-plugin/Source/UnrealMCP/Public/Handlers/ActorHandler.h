#pragma once

#include "CoreMinimal.h"
#include "IHandler.h"
#include "ActorHandler.generated.h"

UCLASS()
class UNREALMCP_API UActorHandler : public UObject, public IHandler
{
    GENERATED_BODY()

public:
    virtual FString GetPrefix() const override { return TEXT("actor"); }
    virtual TSharedPtr<FJsonObject> Handle(
        const FString& Command,
        const TSharedPtr<FJsonObject>& Params,
        FString& OutError) override;

private:
    TSharedPtr<FJsonObject> HandleCreate(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleDelete(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleFind(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleList(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleGetTransform(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleSetTransform(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleGetProperty(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleSetProperty(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleAddComponent(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleRemoveComponent(const TSharedPtr<FJsonObject>& Params, FString& OutError);
    TSharedPtr<FJsonObject> HandleGetComponents(const TSharedPtr<FJsonObject>& Params, FString& OutError);

    AActor* FindActorById(const FString& ActorId);
    AActor* FindActorByName(const FString& Name);
    FString GetActorId(AActor* Actor);
};
