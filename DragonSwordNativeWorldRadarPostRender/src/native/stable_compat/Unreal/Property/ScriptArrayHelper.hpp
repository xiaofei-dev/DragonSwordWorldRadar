#pragma once

#include <cstddef>

#include <Unreal/FScriptArray.hpp>
#include <Unreal/Property/FArrayProperty.hpp>

namespace RC::Unreal
{
    // Read-only subset used by the radar. The official v3.0.1 helper shipped
    // with stale field names and stale include paths, so it cannot be consumed
    // by an external C++ mod as published.
    class FScriptArrayHelper
    {
      public:
        FScriptArrayHelper(FArrayProperty* property, const void* array)
            : m_array(static_cast<const FScriptArray*>(array))
            , m_element_size(property && property->GetInner()
                  ? property->GetInner()->GetElementSize()
                  : 0)
        {
        }

        [[nodiscard]] auto Num() const -> int32
        {
            return m_array ? m_array->Num() : 0;
        }

        [[nodiscard]] auto GetRawPtr(int32 index = 0) const -> uint8*
        {
            if (!m_array || m_element_size <= 0 || !m_array->IsValidIndex(index)) {
                return nullptr;
            }
            return static_cast<uint8*>(const_cast<void*>(m_array->GetData()))
                + static_cast<std::size_t>(index) * static_cast<std::size_t>(m_element_size);
        }

      private:
        const FScriptArray* m_array{};
        int32 m_element_size{};
    };

    class FScriptArrayHelper_InContainer : public FScriptArrayHelper
    {
      public:
        FScriptArrayHelper_InContainer(
            FArrayProperty* property, const void* container,
            int32 fixed_array_index = 0)
            : FScriptArrayHelper(
                  property,
                  property && container
                      ? property->ContainerPtrToValuePtr<void>(
                            const_cast<void*>(container), fixed_array_index)
                      : nullptr)
        {
        }

        FScriptArrayHelper_InContainer(
            FArrayProperty* property, const UObject* container,
            int32 fixed_array_index = 0)
            : FScriptArrayHelper_InContainer(
                  property, static_cast<const void*>(container), fixed_array_index)
        {
        }
    };
}
