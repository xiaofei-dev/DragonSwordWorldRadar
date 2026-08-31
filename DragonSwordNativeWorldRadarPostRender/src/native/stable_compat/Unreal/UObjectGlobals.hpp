#pragma once

// RE-UE4SS v3.0.1's public UObjectGlobals header defines one-argument
// NewObject helpers but only exposes a two-argument parameters constructor.
// This source-compatible view preserves the v3.0.1 ABI and supplies the
// missing default Outer argument. It is used only for the StableRoot build.

#include <array>
#include <functional>
#include <vector>

#include <Constructs/Loop.hpp>
#include <Function/Function.hpp>
#include <Unreal/Common.hpp>
#include <Unreal/Core/HAL/Platform.hpp>
#include <Unreal/Function.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UnrealFlags.hpp>

namespace RC::Unreal
{
    class UObject;
    struct ObjectSearcher;

    struct FObjectInstancingGraph {};
    struct FFeedbackContext {};

    template<typename SupposedUObject>
    concept UObjectPointerDerivative = std::is_convertible_v<SupposedUObject, UObject*>;

    template<typename SupposedUObject>
    concept UObjectDerivative = std::is_convertible_v<SupposedUObject, UObject>;

    template<typename SupposedUClass>
    concept UClassDerivative = std::is_convertible_v<SupposedUClass, class UClass>;

    template<typename T>
    concept UObjectPointerDerivativeOrAnyNonUObject = !UObjectDerivative<T> || UObjectPointerDerivative<T>;

    template<UObjectDerivative CastResultType>
    auto Cast(UObject* Object) -> CastResultType*;

#define StaticConstructObject_Internal_Params_Deprecated \
    const UClass* InClass_,\
    UObject* InOuter_,\
    FName InName_,\
    EObjectFlags InFlags_,\
    EInternalObjectFlags InternalSetFlags_,\
    UObject* InTemplate_,\
    bool bCopyTransientsFromClassDefaults_,\
    FObjectInstancingGraph* InInstanceGraph_,\
    bool bAssumeTemplateIsArchetype_,\
    void* ExternalPackage_

    struct RC_UE_API FStaticConstructObjectParameters
    {
        const class UClass* Class;
        UObject* Outer;
        FName Name;
        EObjectFlags SetFlags = RF_NoFlags;
        EInternalObjectFlags InternalSetFlags = EInternalObjectFlags::None;
        bool bCopyTransientsFromClassDefaults = false;
        bool bAssumeTemplateIsArchetype = false;
        UObject* Template = nullptr;
        struct FObjectInstancingGraph* InstanceGraph = nullptr;
        class UPackage* ExternalPackage = nullptr;

    private:
        TFunction<void()> PropertyInitCallback{};
        void* SubobjectOverrides = nullptr;

    public:
        FStaticConstructObjectParameters(const class UClass* InClass, UObject* InOuter = nullptr)
            : Class(InClass), Outer(InOuter)
        {
        }
    };
}

namespace RC::Unreal::UObjectGlobals
{
    static inline UPackage* ANY_PACKAGE{reinterpret_cast<UPackage*>(-1)};

    struct GlobalState
    {
        RC_UE_API static Function<UObject*(StaticConstructObject_Internal_Params_Deprecated)> StaticConstructObjectInternalDeprecated;
        RC_UE_API static Function<UObject*(const FStaticConstructObjectParameters&)> StaticConstructObjectInternal;
    };

    RC_UE_API auto SetupStaticConstructObjectInternalAddress(void* FunctionAddress) -> void;
    RC_UE_API auto ForEachUObject(const std::function<LoopAction(UObject*, int32, int32)>& RawObject) -> void;
    RC_UE_API auto ForEachUObjectInChunk(int32_t ChunkIndex, const std::function<LoopAction(UObject*, int32)>& Callable) -> void;
    RC_UE_API auto ForEachUObjectInRange(int32_t Start, int32_t End, const std::function<LoopAction(UObject*, int32, int32)>& Callable) -> void;
    RC_UE_API auto VersionIsAtMost(uint32_t Major, uint32_t Minor) -> bool;
    RC_UE_API auto StaticConstructObject(const FStaticConstructObjectParameters& Params) -> UObject*;

    template<UObjectPointerDerivative ObjectType = UObject*>
    auto StaticConstructObject(const FStaticConstructObjectParameters& Params) -> ObjectType
    {
        return static_cast<ObjectType>(StaticConstructObject(Params));
    }

    template<typename ObjectType>
    ObjectType* NewObject(UObject* Outer,
                          const UClass* Class,
                          FName Name = NAME_None,
                          EObjectFlags Flags = RF_NoFlags,
                          UObject* Template = nullptr,
                          bool bCopyTransientsFromClassDefaults = false,
                          FObjectInstancingGraph* InInstanceGraph = nullptr,
                          UPackage* ExternalPackage = nullptr)
    {
        FStaticConstructObjectParameters Params{Class, Outer};
        Params.Name = Name;
        Params.SetFlags = Flags;
        Params.Template = Template;
        Params.bCopyTransientsFromClassDefaults = bCopyTransientsFromClassDefaults;
        Params.InstanceGraph = InInstanceGraph;
        Params.ExternalPackage = ExternalPackage;
        return StaticConstructObject<ObjectType*>(Params);
    }

    template<typename ObjectType>
    ObjectType* NewObject(UObject* Outer,
                          FName Name,
                          EObjectFlags Flags = RF_NoFlags,
                          UObject* Template = nullptr,
                          bool bCopyTransientsFromClassDefaults = false,
                          FObjectInstancingGraph* InInstanceGraph = nullptr)
    {
        FStaticConstructObjectParameters Params{ObjectType::StaticClass(), Outer};
        Params.Name = Name;
        Params.SetFlags = Flags;
        Params.Template = Template;
        Params.bCopyTransientsFromClassDefaults = bCopyTransientsFromClassDefaults;
        Params.InstanceGraph = InInstanceGraph;
        return StaticConstructObject<ObjectType*>(Params);
    }

    RC_UE_API UObject* FindObject(UClass* Class, UObject* InOuter, File::StringViewType InName, bool bExactClass = false, ObjectSearcher* = nullptr);
    RC_UE_API UObject* FindObject(UClass* Class, UObject* InOuter, const TCHAR* InName, bool bExactClass = false, ObjectSearcher* = nullptr);
    RC_UE_API UObject* FindObject(ObjectSearcher&, UClass* Class, UObject* InOuter, File::StringViewType InName, bool bExactClass = false);
    RC_UE_API UObject* FindObject(ObjectSearcher&, UClass* Class, UObject* InOuter, const TCHAR* InName, bool bExactClass = false);

    template<UObjectDerivative ObjectType>
    ObjectType* FindObject(UObject* Outer, const TCHAR* Name, bool ExactClass = false)
    {
        return static_cast<ObjectType*>(FindObject(ObjectType::StaticClass(), Outer, Name, ExactClass));
    }

    RC_UE_API auto StaticFindObject_InternalSlow(UClass* Object, UObject* ChunkIndex, const wchar_t* OrigInName, bool bExactClass = false) -> UObject*;

    template<UObjectPointerDerivative ObjectType = UObject*>
    auto StaticFindObject(UClass* ObjectClass, UObject* InObjectPackage, const wchar_t* OrigInName, bool bExactClass = false) -> ObjectType
    {
        return static_cast<ObjectType>(FindObject(ObjectClass, InObjectPackage, OrigInName, bExactClass));
    }

    template<UObjectPointerDerivative ObjectType = UObject*>
    auto StaticFindObject(UClass* ObjectClass, UObject* InObjectPackage, std::wstring_view OrigInName, bool bExactClass = false) -> ObjectType
    {
        return static_cast<ObjectType>(FindObject(ObjectClass, InObjectPackage, OrigInName.data(), bExactClass));
    }

    template<UObjectPointerDerivative ObjectType = UObject*>
    auto StaticFindObject(UClass* ObjectClass, UObject* InObjectPackage, const std::wstring& OrigInName, bool bExactClass = false) -> ObjectType
    {
        return static_cast<ObjectType>(FindObject(ObjectClass, InObjectPackage, OrigInName.c_str(), bExactClass));
    }

    RC_UE_API auto FindFirstOf(FName Object) -> UObject*;
    RC_UE_API auto FindFirstOf(const wchar_t* ClassName) -> UObject*;
    RC_UE_API auto FindFirstOf(std::wstring_view ClassName) -> UObject*;
    RC_UE_API auto FindFirstOf(const std::wstring& ClassName) -> UObject*;
    RC_UE_API auto FindFirstOf(std::string_view ClassName) -> UObject*;
    RC_UE_API auto FindFirstOf(const std::string& ClassName) -> UObject*;

    RC_UE_API auto FindAllOf(FName SuperStruct, std::vector<UObject*>& ChunkIndex) -> void;
    RC_UE_API auto FindAllOf(const wchar_t* ClassName, std::vector<UObject*>& OutStorage) -> void;
    RC_UE_API auto FindAllOf(std::wstring_view ClassName, std::vector<UObject*>& OutStorage) -> void;
    RC_UE_API auto FindAllOf(const std::wstring& ClassName, std::vector<UObject*>& OutStorage) -> void;
    RC_UE_API auto FindAllOf(std::string_view ClassName, std::vector<UObject*>& OutStorage) -> void;
    RC_UE_API auto FindAllOf(const std::string& ClassName, std::vector<UObject*>& OutStorage) -> void;

    RC_UE_API auto FindObjects(size_t NumObjectsToFind, const FName ClassName, const FName ObjectShortName, std::vector<UObject*>& OutStorage, int32 RequiredFlags = {}, int32 BannedFlags = {}, bool bExactClass = true) -> void;
    RC_UE_API auto FindObjects(size_t NumObjectsToFind, const wchar_t* ClassName, const wchar_t* ObjectShortName, std::vector<UObject*>& OutStorage, int32 RequiredFlags = {}, int32 BannedFlags = {}, bool bExactClass = true) -> void;
    RC_UE_API auto FindObject(const FName ClassName, const FName ObjectShortName, int32 RequiredFlags = {}, int32 BannedFlags = {}) -> UObject*;
    RC_UE_API auto FindObject(const wchar_t* ClassName, const wchar_t* ObjectShortName, int32 RequiredFlags = {}, int32 BannedFlags = {}) -> UObject*;
    RC_UE_API auto FindObjects(const FName ClassName, const FName ObjectShortName, std::vector<UObject*>& OutStorage, int32 RequiredFlags = {}, int32 BannedFlags = {}, bool bExactClass = true) -> void;
    RC_UE_API auto FindObjects(const wchar_t* ClassName, const wchar_t* ObjectShortName, std::vector<UObject*>& OutStorage, int32 RequiredFlags = {}, int32 BannedFlags = {}, bool bExactClass = true) -> void;

    RC_UE_API auto RegisterHook(class UFunction* Function, UnrealScriptFunctionCallable, UnrealScriptFunctionCallable, void*) -> std::pair<int, int>;
    RC_UE_API auto RegisterHook(const StringType& FunctionFullNameNoType, UnrealScriptFunctionCallable, UnrealScriptFunctionCallable, void*) -> std::pair<int, int>;
    RC_UE_API auto UnregisterHook(class UFunction* Function, std::pair<int, int>) -> void;
    RC_UE_API auto UnregisterHook(const StringType& FunctionFullNameNoType, std::pair<int, int>) -> void;
}
