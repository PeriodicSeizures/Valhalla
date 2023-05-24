#pragma once

#include "VUtils.h"
#include "VUtilsTraits.h"

template<typename T, size_t ...COUNTS>
    requires std::is_integral_v<T> && std::is_unsigned_v<T>
class BitPack {
    //static_assert((COUNTS + ...) > 0, "Counts summed together must be greater than 0");
    //static_assert((COUNTS + ...) <= sizeof(T) * 8, "Counts summed together must fit within mask");
    
    static_assert((COUNTS + ...) == sizeof(T) * 8, "Exactly all bits must be utilized in mask");

public:
    using type = T;

    template<size_t index>
    using count = VUtils::Traits::variadic_value_at_index<index, COUNTS...>;

    template<size_t index>
    using offset = VUtils::Traits::variadic_accumulate_values_until_index<index, COUNTS...>;

    template<size_t index>
    using capacity = std::integral_constant<size_t, (1 << count<index>::value) - 1>;

private:
    T m_data{};

private:
    /*
    template<uint8_t index, uint8_t A, uint8_t... Bs>
    constexpr uint8_t OffsetImpl() const {
        if constexpr (index == (sizeof...(Bs)) || sizeof...(Bs) == 0) {
            return A;
        }
        else {
            return A + OffsetImpl<index, Bs...>();
        }
    }

    template<uint8_t index, uint8_t A, uint8_t... Bs>
    constexpr T CountImpl() const {
        if constexpr (index == (sizeof...(Bs)) || sizeof...(Bs) == 0) {
            return static_cast<T>(A);
        }
        else {
            return CountImpl<index, Bs...>();
        }
    }*/

public:
    constexpr BitPack() {}
    constexpr BitPack(T data) : m_data(data) {}

    void operator=(const BitPack<T, COUNTS...>& other) {
        this->m_data = other.m_data;
    }

    constexpr bool operator==(const BitPack<T, COUNTS...>& other) const {
        return m_data == other.m_data;
    }

    constexpr bool operator!=(const BitPack<T, COUNTS...>& other) const {
        return !(*this == other);
    }

    constexpr operator bool() const {
        return static_cast<bool>(m_data);
    }

    constexpr operator T() const {
        return m_data;
    }

    /*
    // Get the width of a specified member at index
    template<uint8_t index>
        requires (index < sizeof...(COUNTS))
    constexpr T GetCount() const {
        return CountImpl<sizeof...(COUNTS) - index - 1, COUNTS...>();
    }

    // Get the offset of a specified member at index
    template<uint8_t index>
        requires (index < sizeof...(COUNTS))
    constexpr T GetOffset() const {
        if constexpr (index == 0) {
            return 0;
        }
        else {
            return OffsetImpl<sizeof...(COUNTS) - index, COUNTS...>();
        }
    }*/

    // Get the value of a specified member at index
    template<uint8_t index>
        requires (index < sizeof...(COUNTS))
    constexpr type Get() const {
        //return (m_data >> GetOffset<index>()) & ((1 << GetCount<index>()) - 1);
        //return m_data >> offset<index>::value
        return (m_data >> offset<index>::value) & ((1 << count<index>::value) - 1);
    }

    // Set the value of a specified member at index to 0
    template<uint8_t index>
    constexpr void Clear() {
        //m_data &= ~(((1 << GetCount<index>()) - 1) << GetOffset<index>());
        m_data &= ~(((1 << count<index>::value) - 1) << offset<index>::value);
    }

    // Set the value of a specified member at index
    template<uint8_t index>
        requires (index < sizeof...(COUNTS))
    constexpr void Set(type value) {
        Clear<index>();
        Merge<index>(value);
    }

    // Merge the bits of a specified index with another value
    template<uint8_t index>
    constexpr void Merge(type value) {
        //m_data |= (value & ((1 << GetCount<index>()) - 1)) << GetOffset<index>();
        m_data |= (value & ((1 << count<index>::value) - 1)) << offset<index>::value;
    }
};

