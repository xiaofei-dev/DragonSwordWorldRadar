#pragma once

#include <bit>
#include <cstdint>

#include <Unreal/Common.hpp>

namespace RC::Unreal
{
    // Target-local view of the exact UE4SS v3.0.1 ABI. That release keeps
    // these identity fields and Reset private; newer UE4SS exposes them.
    struct RC_UE_API FWeakObjectPtr
    {
        int32_t ObjectIndex{};
        int32_t ObjectSerialNumber{};

      private:
        auto InternalGetObjectItem() const -> struct FUObjectItem*;
        auto InternalGet(bool bEvenIfPendingKill) const -> class UObject*;
        auto Reset() -> void;

      public:
        FWeakObjectPtr() = default;

        auto operator=(class UObject*) -> void;
        auto SerialNumbersMatch(struct FUObjectItem*) const -> bool;
        auto Get() const -> class UObject*;
    };

    template<class T=UObject, class TWeakObjectPtrBase=FWeakObjectPtr>
    struct TWeakObjectPtr;

    template<class T, class TWeakObjectPtrBase>
    struct TWeakObjectPtr : private TWeakObjectPtrBase
    {
        T* Get()
        {
            return std::bit_cast<T*>(TWeakObjectPtrBase::Get());
        }
    };
}
