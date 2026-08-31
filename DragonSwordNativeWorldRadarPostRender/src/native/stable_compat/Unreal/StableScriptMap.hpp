#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <Unreal/ContainerAllocationPolicies.hpp>
#include <Unreal/FScriptArray.hpp>
#include <Unreal/Property/FMapProperty.hpp>

namespace RC::Unreal
{
    struct FScriptSparseArrayLayout
    {
        int32 Alignment{};
        int32 Size{};
    };

    struct FScriptSetLayout
    {
        int32 HashNextIdOffset{};
        int32 HashIndexOffset{};
        int32 Size{};
        FScriptSparseArrayLayout SparseArrayLayout{};
    };

    struct FScriptMapLayout
    {
        int32 ValueOffset{};
        FScriptSetLayout SetLayout{};
    };

    namespace StableCompat
    {
        [[nodiscard]] constexpr auto align_value(int32 value, int32 alignment) noexcept -> int32
        {
            return alignment > 0
                ? (value + alignment - 1) & ~(alignment - 1)
                : value;
        }

        class ScriptBitArrayView
        {
          public:
            [[nodiscard]] auto IsValidIndex(int32 index) const noexcept -> bool
            {
                return index >= 0 && index < m_num_bits;
            }

            [[nodiscard]] auto IsSet(int32 index) const noexcept -> bool
            {
                if (!IsValidIndex(index)) {
                    return false;
                }
                const auto* words = m_allocator.GetAllocation();
                return words && (words[index >> 5] & (std::uint32_t{1} << (index & 31))) != 0;
            }

          private:
            TInlineAllocator<4>::ForElementType<std::uint32_t> m_allocator;
            int32 m_num_bits{};
            int32 m_max_bits{};
        };

        class ScriptSparseArrayView
        {
          public:
            [[nodiscard]] auto IsValidIndex(int32 index) const noexcept -> bool
            {
                return m_allocation_flags.IsSet(index);
            }

            [[nodiscard]] auto Num() const noexcept -> int32
            {
                return m_data.Num() - m_num_free_indices;
            }

            [[nodiscard]] auto GetMaxIndex() const noexcept -> int32
            {
                return m_data.Num();
            }

            [[nodiscard]] auto GetData(
                int32 index, const FScriptSparseArrayLayout& layout) noexcept -> void*
            {
                auto* data = static_cast<std::byte*>(m_data.GetData());
                return data && index >= 0
                    ? data + static_cast<std::size_t>(layout.Size)
                        * static_cast<std::size_t>(index)
                    : nullptr;
            }

          private:
            FScriptArray m_data;
            ScriptBitArrayView m_allocation_flags;
            int32 m_first_free_index{};
            int32 m_num_free_indices{};
        };

        [[nodiscard]] inline auto make_map_layout(FMapProperty* property) noexcept
            -> FScriptMapLayout
        {
            FScriptMapLayout result{};
            if (!property || !property->GetKeyProp() || !property->GetValueProp()) {
                return result;
            }

            FProperty* key = property->GetKeyProp();
            FProperty* value = property->GetValueProp();
            const int32 key_alignment = std::max<int32>(1, key->GetMinAlignment());
            const int32 value_alignment = std::max<int32>(1, value->GetMinAlignment());
            const int32 pair_alignment = std::max(key_alignment, value_alignment);
            result.ValueOffset = align_value(key->GetSize(), value_alignment);
            const int32 pair_size = align_value(
                result.ValueOffset + value->GetSize(), pair_alignment);

            result.SetLayout.HashNextIdOffset = align_value(pair_size, alignof(int32));
            result.SetLayout.HashIndexOffset = result.SetLayout.HashNextIdOffset + sizeof(int32);
            const int32 set_alignment = std::max<int32>(pair_alignment, alignof(int32));
            result.SetLayout.Size = align_value(
                result.SetLayout.HashIndexOffset + sizeof(int32), set_alignment);
            result.SetLayout.SparseArrayLayout.Alignment =
                std::max<int32>(set_alignment, alignof(int32));
            result.SetLayout.SparseArrayLayout.Size = align_value(
                std::max<int32>(result.SetLayout.Size, sizeof(int32) * 2),
                result.SetLayout.SparseArrayLayout.Alignment);
            return result;
        }
    }

    class FScriptMap
    {
      public:
        [[nodiscard]] auto IsValidIndex(int32 index) const noexcept -> bool
        {
            return m_elements.IsValidIndex(index);
        }

        [[nodiscard]] auto Num() const noexcept -> int32
        {
            return m_elements.Num();
        }

        [[nodiscard]] auto GetMaxIndex() const noexcept -> int32
        {
            return m_elements.GetMaxIndex();
        }

        [[nodiscard]] auto GetData(
            int32 index, const FScriptMapLayout& layout) noexcept -> void*
        {
            return m_elements.GetData(index, layout.SetLayout.SparseArrayLayout);
        }

      private:
        StableCompat::ScriptSparseArrayView m_elements;
    };
}
