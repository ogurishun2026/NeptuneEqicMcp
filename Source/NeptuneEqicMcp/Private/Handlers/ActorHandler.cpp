#include "Handlers/ActorHandler.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"
#include "Components/ActorComponent.h"
#include "JsonObjectConverter.h"

TSharedPtr<FJsonObject> UActorHandler::Handle(
    const FString& Command,
    const TSharedPtr<FJsonObject>& Params,
    FString& OutError)
{
    if (Command == TEXT("create")) return HandleCreate(Params, OutError);
    if (Command == TEXT("delete")) return HandleDelete(Params, OutError);
    if (Command == TEXT("find")) return HandleFind(Params, OutError);
    if (Command == TEXT("list")) return HandleList(Params, OutError);
    if (Command == TEXT("get_transform")) return HandleGetTransform(Params, OutError);
    if (Command == TEXT("set_transform")) return HandleSetTransform(Params, OutError);
    if (Command == TEXT("get_property")) return HandleGetProperty(Params, OutError);
    if (Command == TEXT("set_property")) return HandleSetProperty(Params, OutError);
    if (Command == TEXT("add_component")) return HandleAddComponent(Params, OutError);
    if (Command == TEXT("remove_component")) return HandleRemoveComponent(Params, OutError);
    if (Command == TEXT("get_components")) return HandleGetComponents(Params, OutError);

    OutError = FString::Printf(TEXT("Unknown command: %s"), *Command);
    return nullptr;
}

TSharedPtr<FJsonObject> UActorHandler::HandleCreate(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ClassName;
    if (!Params->TryGetStringField(TEXT("class"), ClassName))
    {
        OutError = TEXT("Missing required parameter: class");
        return nullptr;
    }

    FString ActorName;
    Params->TryGetStringField(TEXT("name"), ActorName);

    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    FVector Scale = FVector::OneVector;

    const TSharedPtr<FJsonObject>* LocationObj;
    if (Params->TryGetObjectField(TEXT("location"), LocationObj))
    {
        Location.X = (*LocationObj)->GetNumberField(TEXT("x"));
        Location.Y = (*LocationObj)->GetNumberField(TEXT("y"));
        Location.Z = (*LocationObj)->GetNumberField(TEXT("z"));
    }

    const TSharedPtr<FJsonObject>* RotationObj;
    if (Params->TryGetObjectField(TEXT("rotation"), RotationObj))
    {
        Rotation.Pitch = (*RotationObj)->GetNumberField(TEXT("pitch"));
        Rotation.Yaw = (*RotationObj)->GetNumberField(TEXT("yaw"));
        Rotation.Roll = (*RotationObj)->GetNumberField(TEXT("roll"));
    }

    const TSharedPtr<FJsonObject>* ScaleObj;
    if (Params->TryGetObjectField(TEXT("scale"), ScaleObj))
    {
        Scale.X = (*ScaleObj)->GetNumberField(TEXT("x"));
        Scale.Y = (*ScaleObj)->GetNumberField(TEXT("y"));
        Scale.Z = (*ScaleObj)->GetNumberField(TEXT("z"));
    }

    UClass* ActorClass = nullptr;

    if (ClassName.StartsWith(TEXT("/")))
    {
        ActorClass = LoadObject<UClass>(nullptr, *ClassName);
    }
    else
    {
        ActorClass = FindObject<UClass>(nullptr, *ClassName);
    }

    if (!ActorClass)
    {
        OutError = FString::Printf(TEXT("Could not find class: %s"), *ClassName);
        return nullptr;
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        OutError = TEXT("No world available");
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    if (!ActorName.IsEmpty())
    {
        SpawnParams.Name = FName(*ActorName);
    }
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* NewActor = World->SpawnActor<AActor>(ActorClass, Location, Rotation, SpawnParams);
    if (!NewActor)
    {
        OutError = TEXT("Failed to spawn actor");
        return nullptr;
    }

    NewActor->SetActorScale3D(Scale);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField(TEXT("id"), GetActorId(NewActor));
    Result->SetStringField(TEXT("name"), NewActor->GetName());
    Result->SetStringField(TEXT("class"), NewActor->GetClass()->GetName());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleDelete(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ActorName;

    if (!Params->TryGetStringField(TEXT("id"), ActorId))
    {
        Params->TryGetStringField(TEXT("name"), ActorName);
    }

    AActor* Actor = nullptr;
    if (!ActorId.IsEmpty())
    {
        Actor = FindActorById(ActorId);
    }
    else if (!ActorName.IsEmpty())
    {
        Actor = FindActorByName(ActorName);
    }
    else
    {
        OutError = TEXT("Missing required parameter: id or name");
        return nullptr;
    }

    if (!Actor)
    {
        OutError = TEXT("Actor not found");
        return nullptr;
    }

    FString DeletedId = GetActorId(Actor);
    FString DeletedName = Actor->GetName();

    Actor->Destroy();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField(TEXT("id"), DeletedId);
    Result->SetStringField(TEXT("name"), DeletedName);
    Result->SetBoolField(TEXT("deleted"), true);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleFind(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ActorName, ClassName;
    Params->TryGetStringField(TEXT("id"), ActorId);
    Params->TryGetStringField(TEXT("name"), ActorName);
    Params->TryGetStringField(TEXT("class"), ClassName);

    TArray<AActor*> FoundActors;

    if (!ActorId.IsEmpty())
    {
        AActor* Actor = FindActorById(ActorId);
        if (Actor) FoundActors.Add(Actor);
    }
    else if (!ActorName.IsEmpty())
    {
        AActor* Actor = FindActorByName(ActorName);
        if (Actor) FoundActors.Add(Actor);
    }
    else if (!ClassName.IsEmpty())
    {
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (World)
        {
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                if ((*It)->GetClass()->GetName().Contains(ClassName) ||
                    (*It)->GetClass()->GetPathName().Contains(ClassName))
                {
                    FoundActors.Add(*It);
                }
            }
        }
    }
    else
    {
        OutError = TEXT("Missing search criteria: id, name, or class");
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> ActorsArray;
    for (AActor* Actor : FoundActors)
    {
        TSharedPtr<FJsonObject> ActorObj = MakeShareable(new FJsonObject);
        ActorObj->SetStringField(TEXT("id"), GetActorId(Actor));
        ActorObj->SetStringField(TEXT("name"), Actor->GetName());
        ActorObj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
        ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetArrayField(TEXT("actors"), ActorsArray);
    Result->SetNumberField(TEXT("count"), ActorsArray.Num());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleList(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ClassName;
    Params->TryGetStringField(TEXT("class"), ClassName);

    bool bIncludeHidden = false;
    Params->TryGetBoolField(TEXT("include_hidden"), bIncludeHidden);

    int32 Limit = 100;
    Params->TryGetNumberField(TEXT("limit"), Limit);

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        OutError = TEXT("No world available");
        return nullptr;
    }

    TArray<TSharedPtr<FJsonValue>> ActorsArray;
    int32 Count = 0;

    for (TActorIterator<AActor> It(World); It && Count < Limit; ++It)
    {
        AActor* Actor = *It;

        if (!bIncludeHidden && Actor->IsHidden())
        {
            continue;
        }

        if (!ClassName.IsEmpty())
        {
            if (!Actor->GetClass()->GetName().Contains(ClassName) &&
                !Actor->GetClass()->GetPathName().Contains(ClassName))
            {
                continue;
            }
        }

        TSharedPtr<FJsonObject> ActorObj = MakeShareable(new FJsonObject);
        ActorObj->SetStringField(TEXT("id"), GetActorId(Actor));
        ActorObj->SetStringField(TEXT("name"), Actor->GetName());
        ActorObj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
        ActorObj->SetBoolField(TEXT("hidden"), Actor->IsHidden());
        ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
        Count++;
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetArrayField(TEXT("actors"), ActorsArray);
    Result->SetNumberField(TEXT("count"), ActorsArray.Num());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetTransform(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId;
    if (!Params->TryGetStringField(TEXT("id"), ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    FTransform Transform = Actor->GetActorTransform();

    TSharedPtr<FJsonObject> LocationObj = MakeShareable(new FJsonObject);
    LocationObj->SetNumberField(TEXT("x"), Transform.GetLocation().X);
    LocationObj->SetNumberField(TEXT("y"), Transform.GetLocation().Y);
    LocationObj->SetNumberField(TEXT("z"), Transform.GetLocation().Z);

    TSharedPtr<FJsonObject> RotationObj = MakeShareable(new FJsonObject);
    FRotator Rotator = Transform.GetRotation().Rotator();
    RotationObj->SetNumberField(TEXT("pitch"), Rotator.Pitch);
    RotationObj->SetNumberField(TEXT("yaw"), Rotator.Yaw);
    RotationObj->SetNumberField(TEXT("roll"), Rotator.Roll);

    TSharedPtr<FJsonObject> ScaleObj = MakeShareable(new FJsonObject);
    ScaleObj->SetNumberField(TEXT("x"), Transform.GetScale3D().X);
    ScaleObj->SetNumberField(TEXT("y"), Transform.GetScale3D().Y);
    ScaleObj->SetNumberField(TEXT("z"), Transform.GetScale3D().Z);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField(TEXT("id"), ActorId);
    Result->SetObjectField(TEXT("location"), LocationObj);
    Result->SetObjectField(TEXT("rotation"), RotationObj);
    Result->SetObjectField(TEXT("scale"), ScaleObj);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleSetTransform(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId;
    if (!Params->TryGetStringField(TEXT("id"), ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    FVector Location = Actor->GetActorLocation();
    FRotator Rotation = Actor->GetActorRotation();
    FVector Scale = Actor->GetActorScale3D();

    const TSharedPtr<FJsonObject>* LocationObj;
    if (Params->TryGetObjectField(TEXT("location"), LocationObj))
    {
        Location.X = (*LocationObj)->GetNumberField(TEXT("x"));
        Location.Y = (*LocationObj)->GetNumberField(TEXT("y"));
        Location.Z = (*LocationObj)->GetNumberField(TEXT("z"));
    }

    const TSharedPtr<FJsonObject>* RotationObj;
    if (Params->TryGetObjectField(TEXT("rotation"), RotationObj))
    {
        Rotation.Pitch = (*RotationObj)->GetNumberField(TEXT("pitch"));
        Rotation.Yaw = (*RotationObj)->GetNumberField(TEXT("yaw"));
        Rotation.Roll = (*RotationObj)->GetNumberField(TEXT("roll"));
    }

    const TSharedPtr<FJsonObject>* ScaleObj;
    if (Params->TryGetObjectField(TEXT("scale"), ScaleObj))
    {
        Scale.X = (*ScaleObj)->GetNumberField(TEXT("x"));
        Scale.Y = (*ScaleObj)->GetNumberField(TEXT("y"));
        Scale.Z = (*ScaleObj)->GetNumberField(TEXT("z"));
    }

    Actor->SetActorLocation(Location);
    Actor->SetActorRotation(Rotation);
    Actor->SetActorScale3D(Scale);

    return HandleGetTransform(Params, OutError);
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetProperty(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, PropertyName;
    if (!Params->TryGetStringField(TEXT("id"), ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }
    if (!Params->TryGetStringField(TEXT("property"), PropertyName))
    {
        OutError = TEXT("Missing required parameter: property");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    UClass* Class = Actor->GetClass();
    FProperty* Property = Class->FindPropertyByName(FName(*PropertyName));

    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return nullptr;
    }

    FString ValueStr;
    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);

    if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        ValueStr = StrProperty->GetPropertyValue(ValuePtr);
    }
    else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        ValueStr = FString::FromInt(IntProperty->GetPropertyValue(ValuePtr));
    }
    else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        ValueStr = FString::SanitizeFloat(FloatProperty->GetPropertyValue(ValuePtr));
    }
    else if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        ValueStr = BoolProperty->GetPropertyValue(ValuePtr) ? TEXT("true") : TEXT("false");
    }
    else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        ValueStr = NameProperty->GetPropertyValue(ValuePtr).ToString();
    }
    else
    {
        Property->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, Actor, PPF_None);
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField(TEXT("id"), ActorId);
    Result->SetStringField(TEXT("property"), PropertyName);
    Result->SetStringField(TEXT("value"), ValueStr);
    Result->SetStringField(TEXT("type"), Property->GetClass()->GetName());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleSetProperty(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, PropertyName, ValueStr;
    if (!Params->TryGetStringField(TEXT("id"), ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }
    if (!Params->TryGetStringField(TEXT("property"), PropertyName))
    {
        OutError = TEXT("Missing required parameter: property");
        return nullptr;
    }
    if (!Params->TryGetStringField(TEXT("value"), ValueStr))
    {
        OutError = TEXT("Missing required parameter: value");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    UClass* Class = Actor->GetClass();
    FProperty* Property = Class->FindPropertyByName(FName(*PropertyName));

    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return nullptr;
    }

    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Actor);

    if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        StrProperty->SetPropertyValue(ValuePtr, ValueStr);
    }
    else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        IntProperty->SetPropertyValue(ValuePtr, FCString::Atoi(*ValueStr));
    }
    else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        FloatProperty->SetPropertyValue(ValuePtr, FCString::Atof(*ValueStr));
    }
    else if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        BoolProperty->SetPropertyValue(ValuePtr, ValueStr.ToBool());
    }
    else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        NameProperty->SetPropertyValue(ValuePtr, FName(*ValueStr));
    }
    else
    {
        Property->ImportText_Direct(*ValueStr, ValuePtr, Actor, PPF_None);
    }

    return HandleGetProperty(Params, OutError);
}

TSharedPtr<FJsonObject> UActorHandler::HandleAddComponent(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ComponentClassName, ComponentName;
    if (!Params->TryGetStringField(TEXT("id"), ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }
    if (!Params->TryGetStringField(TEXT("class"), ComponentClassName))
    {
        OutError = TEXT("Missing required parameter: class");
        return nullptr;
    }

    Params->TryGetStringField(TEXT("name"), ComponentName);

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    UClass* ComponentClass = FindObject<UClass>(nullptr, *ComponentClassName);
    if (!ComponentClass)
    {
        OutError = FString::Printf(TEXT("Component class not found: %s"), *ComponentClassName);
        return nullptr;
    }

    UActorComponent* NewComponent = NewObject<UActorComponent>(Actor, ComponentClass,
        ComponentName.IsEmpty() ? NAME_None : FName(*ComponentName));

    if (!NewComponent)
    {
        OutError = TEXT("Failed to create component");
        return nullptr;
    }

    NewComponent->RegisterComponent();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField(TEXT("actor_id"), ActorId);
    Result->SetStringField(TEXT("component_name"), NewComponent->GetName());
    Result->SetStringField(TEXT("component_class"), NewComponent->GetClass()->GetName());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleRemoveComponent(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ComponentName;
    if (!Params->TryGetStringField(TEXT("id"), ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }
    if (!Params->TryGetStringField(TEXT("name"), ComponentName))
    {
        OutError = TEXT("Missing required parameter: name");
        return nullptr;
    }

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);

    UActorComponent* TargetComponent = nullptr;
    for (UActorComponent* Comp : Components)
    {
        if (Comp->GetName() == ComponentName)
        {
            TargetComponent = Comp;
            break;
        }
    }

    if (!TargetComponent)
    {
        OutError = FString::Printf(TEXT("Component not found: %s"), *ComponentName);
        return nullptr;
    }

    TargetComponent->DestroyComponent();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField(TEXT("actor_id"), ActorId);
    Result->SetStringField(TEXT("component_name"), ComponentName);
    Result->SetBoolField(TEXT("removed"), true);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetComponents(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ComponentClass;
    if (!Params->TryGetStringField(TEXT("id"), ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }

    Params->TryGetStringField(TEXT("class"), ComponentClass);

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    TArray<UActorComponent*> Components;
    if (!ComponentClass.IsEmpty())
    {
        UClass* Class = FindObject<UClass>(nullptr, *ComponentClass);
        if (Class)
        {
            Actor->GetComponents(Class, Components);
        }
    }
    else
    {
        Actor->GetComponents(Components);
    }

    TArray<TSharedPtr<FJsonValue>> ComponentsArray;
    for (UActorComponent* Component : Components)
    {
        TSharedPtr<FJsonObject> CompObj = MakeShareable(new FJsonObject);
        CompObj->SetStringField(TEXT("name"), Component->GetName());
        CompObj->SetStringField(TEXT("class"), Component->GetClass()->GetName());
        ComponentsArray.Add(MakeShareable(new FJsonValueObject(CompObj)));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField(TEXT("actor_id"), ActorId);
    Result->SetArrayField(TEXT("components"), ComponentsArray);
    Result->SetNumberField(TEXT("count"), ComponentsArray.Num());

    return Result;
}

AActor* UActorHandler::FindActorById(const FString& ActorId)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return nullptr;

    FString ActorName = ActorId;
    int32 UnderscoreIndex;
    if (ActorId.FindLastChar('_', UnderscoreIndex))
    {
        ActorName = ActorId.Left(UnderscoreIndex);
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (GetActorId(*It) == ActorId || (*It)->GetName() == ActorName)
        {
            return *It;
        }
    }

    return nullptr;
}

AActor* UActorHandler::FindActorByName(const FString& Name)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if ((*It)->GetName() == Name
#if WITH_EDITOR
            || (*It)->GetActorLabel() == Name
#endif
        )
        {
            return *It;
        }
    }

    return nullptr;
}

FString UActorHandler::GetActorId(AActor* Actor)
{
    if (!Actor) return TEXT("");

    FString LevelName = Actor->GetLevel() ? Actor->GetLevel()->GetPathName() : TEXT("Unknown");
    return FString::Printf(TEXT("%s_%s"), *Actor->GetName(), *LevelName);
}
