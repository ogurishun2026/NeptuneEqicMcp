#include "ActorHandler.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
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
    if (!Params->TryGetStringField("class", ClassName))
    {
        OutError = TEXT("Missing required parameter: class");
        return nullptr;
    }

    FString ActorName;
    Params->TryGetStringField("name", ActorName);

    // Get location, rotation, scale from params
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    FVector Scale = FVector::OneVector;

    const TSharedPtr<FJsonObject>* LocationObj;
    if (Params->TryGetObjectField("location", LocationObj))
    {
        Location.X = (*LocationObj)->GetNumberField("x");
        Location.Y = (*LocationObj)->GetNumberField("y");
        Location.Z = (*LocationObj)->GetNumberField("z");
    }

    const TSharedPtr<FJsonObject>* RotationObj;
    if (Params->TryGetObjectField("rotation", RotationObj))
    {
        Rotation.Pitch = (*RotationObj)->GetNumberField("pitch");
        Rotation.Yaw = (*RotationObj)->GetNumberField("yaw");
        Rotation.Roll = (*RotationObj)->GetNumberField("roll");
    }

    const TSharedPtr<FJsonObject>* ScaleObj;
    if (Params->TryGetObjectField("scale", ScaleObj))
    {
        Scale.X = (*ScaleObj)->GetNumberField("x");
        Scale.Y = (*ScaleObj)->GetNumberField("y");
        Scale.Z = (*ScaleObj)->GetNumberField("z");
    }

    // Find the class
    UClass* ActorClass = nullptr;

    // Try to find by path first
    if (ClassName.StartsWith(TEXT("/")))
    {
        ActorClass = LoadObject<UClass>(nullptr, *ClassName);
    }
    else
    {
        // Try to find by short name
        ActorClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
    }

    if (!ActorClass)
    {
        OutError = FString::Printf(TEXT("Could not find class: %s"), *ClassName);
        return nullptr;
    }

    // Spawn the actor
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
    Result->SetStringField("id", GetActorId(NewActor));
    Result->SetStringField("name", NewActor->GetName());
    Result->SetStringField("class", NewActor->GetClass()->GetName());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleDelete(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ActorName;

    if (!Params->TryGetStringField("id", ActorId))
    {
        Params->TryGetStringField("name", ActorName);
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

    // Store info before destroying
    FString DeletedId = GetActorId(Actor);
    FString DeletedName = Actor->GetName();

    Actor->Destroy();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField("id", DeletedId);
    Result->SetStringField("name", DeletedName);
    Result->SetBoolField("deleted", true);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleFind(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ActorName, ClassName;
    Params->TryGetStringField("id", ActorId);
    Params->TryGetStringField("name", ActorName);
    Params->TryGetStringField("class", ClassName);

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
        // Find all actors of class
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
        ActorObj->SetStringField("id", GetActorId(Actor));
        ActorObj->SetStringField("name", Actor->GetName());
        ActorObj->SetStringField("class", Actor->GetClass()->GetName());
        ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetArrayField("actors", ActorsArray);
    Result->SetNumberField("count", ActorsArray.Num());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleList(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ClassName;
    Params->TryGetStringField("class", ClassName);

    bool bIncludeHidden = false;
    Params->TryGetBoolField("include_hidden", bIncludeHidden);

    int32 Limit = 100;
    Params->TryGetNumberField("limit", Limit);

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

        // Skip hidden actors unless requested
        if (!bIncludeHidden && Actor->IsHidden())
        {
            continue;
        }

        // Filter by class if specified
        if (!ClassName.IsEmpty())
        {
            if (!Actor->GetClass()->GetName().Contains(ClassName) &&
                !Actor->GetClass()->GetPathName().Contains(ClassName))
            {
                continue;
            }
        }

        TSharedPtr<FJsonObject> ActorObj = MakeShareable(new FJsonObject);
        ActorObj->SetStringField("id", GetActorId(Actor));
        ActorObj->SetStringField("name", Actor->GetName());
        ActorObj->SetStringField("class", Actor->GetClass()->GetName());
        ActorObj->SetBoolField("hidden", Actor->IsHidden());
        ActorsArray.Add(MakeShareable(new FJsonValueObject(ActorObj)));
        Count++;
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetArrayField("actors", ActorsArray);
    Result->SetNumberField("count", ActorsArray.Num());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetTransform(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId;
    if (!Params->TryGetStringField("id", ActorId))
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
    LocationObj->SetNumberField("x", Transform.GetLocation().X);
    LocationObj->SetNumberField("y", Transform.GetLocation().Y);
    LocationObj->SetNumberField("z", Transform.GetLocation().Z);

    TSharedPtr<FJsonObject> RotationObj = MakeShareable(new FJsonObject);
    FRotator Rotator = Transform.GetRotation().Rotator();
    RotationObj->SetNumberField("pitch", Rotator.Pitch);
    RotationObj->SetNumberField("yaw", Rotator.Yaw);
    RotationObj->SetNumberField("roll", Rotator.Roll);

    TSharedPtr<FJsonObject> ScaleObj = MakeShareable(new FJsonObject);
    ScaleObj->SetNumberField("x", Transform.GetScale3D().X);
    ScaleObj->SetNumberField("y", Transform.GetScale3D().Y);
    ScaleObj->SetNumberField("z", Transform.GetScale3D().Z);

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField("id", ActorId);
    Result->SetObjectField("location", LocationObj);
    Result->SetObjectField("rotation", RotationObj);
    Result->SetObjectField("scale", ScaleObj);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleSetTransform(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId;
    if (!Params->TryGetStringField("id", ActorId))
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

    // Get current transform
    FVector Location = Actor->GetActorLocation();
    FRotator Rotation = Actor->GetActorRotation();
    FVector Scale = Actor->GetActorScale3D();

    // Update from params
    const TSharedPtr<FJsonObject>* LocationObj;
    if (Params->TryGetObjectField("location", LocationObj))
    {
        Location.X = (*LocationObj)->GetNumberField("x");
        Location.Y = (*LocationObj)->GetNumberField("y");
        Location.Z = (*LocationObj)->GetNumberField("z");
    }

    const TSharedPtr<FJsonObject>* RotationObj;
    if (Params->TryGetObjectField("rotation", RotationObj))
    {
        Rotation.Pitch = (*RotationObj)->GetNumberField("pitch");
        Rotation.Yaw = (*RotationObj)->GetNumberField("yaw");
        Rotation.Roll = (*RotationObj)->GetNumberField("roll");
    }

    const TSharedPtr<FJsonObject>* ScaleObj;
    if (Params->TryGetObjectField("scale", ScaleObj))
    {
        Scale.X = (*ScaleObj)->GetNumberField("x");
        Scale.Y = (*ScaleObj)->GetNumberField("y");
        Scale.Z = (*ScaleObj)->GetNumberField("z");
    }

    Actor->SetActorLocation(Location);
    Actor->SetActorRotation(Rotation);
    Actor->SetActorScale3D(Scale);

    // Return updated transform
    return HandleGetTransform(Params, OutError);
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetProperty(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, PropertyName;
    if (!Params->TryGetStringField("id", ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }
    if (!Params->TryGetStringField("property", PropertyName))
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

    // Find property using reflection
    UClass* Class = Actor->GetClass();
    FProperty* Property = Class->FindPropertyByName(FName(*PropertyName));

    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return nullptr;
    }

    // Get property value as string
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
    else if (FVectorProperty* VectorProperty = CastField<FVectorProperty>(Property))
    {
        FVector Vec = VectorProperty->GetPropertyValue(ValuePtr);
        ValueStr = FString::Printf(TEXT("(%f, %f, %f)"), Vec.X, Vec.Y, Vec.Z);
    }
    else if (FRotatorProperty* RotatorProperty = CastField<FRotatorProperty>(Property))
    {
        FRotator Rot = RotatorProperty->GetPropertyValue(ValuePtr);
        ValueStr = FString::Printf(TEXT("(P=%f, Y=%f, R=%f)"), Rot.Pitch, Rot.Yaw, Rot.Roll);
    }
    else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        ValueStr = NameProperty->GetPropertyValue(ValuePtr).ToString();
    }
    else
    {
        // Generic fallback
        Property->ExportTextItem(ValueStr, ValuePtr, nullptr, Actor, PPF_None);
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField("id", ActorId);
    Result->SetStringField("property", PropertyName);
    Result->SetStringField("value", ValueStr);
    Result->SetStringField("type", Property->GetClass()->GetName());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleSetProperty(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, PropertyName, ValueStr;
    if (!Params->TryGetStringField("id", ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }
    if (!Params->TryGetStringField("property", PropertyName))
    {
        OutError = TEXT("Missing required parameter: property");
        return nullptr;
    }
    if (!Params->TryGetStringField("value", ValueStr))
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

    // Find property using reflection
    UClass* Class = Actor->GetClass();
    FProperty* Property = Class->FindPropertyByName(FName(*PropertyName));

    if (!Property)
    {
        OutError = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return nullptr;
    }

    // Set property value
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
        // Generic fallback using ImportText
        Property->ImportText(*ValueStr, ValuePtr, PPF_None, Actor, nullptr);
    }

    // Return updated property
    return HandleGetProperty(Params, OutError);
}

TSharedPtr<FJsonObject> UActorHandler::HandleAddComponent(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ComponentClassName, ComponentName;
    if (!Params->TryGetStringField("id", ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }
    if (!Params->TryGetStringField("class", ComponentClassName))
    {
        OutError = TEXT("Missing required parameter: class");
        return nullptr;
    }

    Params->TryGetStringField("name", ComponentName);

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    // Find component class
    UClass* ComponentClass = FindObject<UClass>(ANY_PACKAGE, *ComponentClassName);
    if (!ComponentClass)
    {
        OutError = FString::Printf(TEXT("Component class not found: %s"), *ComponentClassName);
        return nullptr;
    }

    // Create component
    UActorComponent* NewComponent = NewObject<UActorComponent>(Actor, ComponentClass,
        ComponentName.IsEmpty() ? NAME_None : FName(*ComponentName));

    if (!NewComponent)
    {
        OutError = TEXT("Failed to create component");
        return nullptr;
    }

    NewComponent->RegisterComponent();

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField("actor_id", ActorId);
    Result->SetStringField("component_name", NewComponent->GetName());
    Result->SetStringField("component_class", NewComponent->GetClass()->GetName());

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleRemoveComponent(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ComponentName;
    if (!Params->TryGetStringField("id", ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }
    if (!Params->TryGetStringField("name", ComponentName))
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

    // Find component
    UActorComponent* Component = Actor->FindComponentByClass<UActorComponent>();
    TArray<UActorComponent*> Components = Actor->GetComponentsByClass(UActorComponent::StaticClass());

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
    Result->SetStringField("actor_id", ActorId);
    Result->SetStringField("component_name", ComponentName);
    Result->SetBoolField("removed", true);

    return Result;
}

TSharedPtr<FJsonObject> UActorHandler::HandleGetComponents(const TSharedPtr<FJsonObject>& Params, FString& OutError)
{
    FString ActorId, ComponentClass;
    if (!Params->TryGetStringField("id", ActorId))
    {
        OutError = TEXT("Missing required parameter: id");
        return nullptr;
    }

    Params->TryGetStringField("class", ComponentClass);

    AActor* Actor = FindActorById(ActorId);
    if (!Actor)
    {
        OutError = FString::Printf(TEXT("Actor not found: %s"), *ActorId);
        return nullptr;
    }

    TArray<UActorComponent*> Components;
    if (!ComponentClass.IsEmpty())
    {
        UClass* Class = FindObject<UClass>(ANY_PACKAGE, *ComponentClass);
        if (Class)
        {
            Components = Actor->GetComponentsByClass(Class);
        }
    }
    else
    {
        Components = Actor->GetComponentsByClass(UActorComponent::StaticClass());
    }

    TArray<TSharedPtr<FJsonValue>> ComponentsArray;
    for (UActorComponent* Component : Components)
    {
        TSharedPtr<FJsonObject> CompObj = MakeShareable(new FJsonObject);
        CompObj->SetStringField("name", Component->GetName());
        CompObj->SetStringField("class", Component->GetClass()->GetName());
        ComponentsArray.Add(MakeShareable(new FJsonValueObject(CompObj)));
    }

    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
    Result->SetStringField("actor_id", ActorId);
    Result->SetArrayField("components", ComponentsArray);
    Result->SetNumberField("count", ComponentsArray.Num());

    return Result;
}

AActor* UActorHandler::FindActorById(const FString& ActorId)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return nullptr;

    // Parse actor ID (format: "Name_LevelPath" or just "Name")
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
        if ((*It)->GetName() == Name || (*It)->GetActorLabel() == Name)
        {
            return *It;
        }
    }

    return nullptr;
}

FString UActorHandler::GetActorId(AActor* Actor)
{
    if (!Actor) return TEXT("");

    // Create a unique ID combining name and level path
    FString LevelName = Actor->GetLevel() ? Actor->GetLevel()->GetPathName() : TEXT("Unknown");
    return FString::Printf(TEXT("%s_%s"), *Actor->GetName(), *LevelName);
}
