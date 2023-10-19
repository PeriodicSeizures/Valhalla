#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "ModManager.h"
#include "DataStream.h"

class DataReader : public DataStream {
private:
    void read_some_bytes(BYTE_t* buffer, size_t count) {
        this->AssertOffset(count);

        // read into 'buffer'
        std::copy(this->data() + this->m_pos,
            this->data() + this->m_pos + count,
            buffer);

        this->m_pos += count;
    }

    uint32_t read_encoded_int() {
        uint32_t out = 0;
        uint32_t num2 = 0;
        while (num2 != 35) {
            auto b = read<uint8_t>();
            out |= static_cast<decltype(out)>(b & 127) << num2;
            num2 += 7;
            if ((b & 128) == 0)
            {
                return out;
            }
        }
        throw std::runtime_error("bad encoded int");
    }

public:
    explicit DataReader(BYTE_VIEW_t buf) : DataStream(buf) {}
    explicit DataReader(BYTES_t &buf) : DataStream(buf) {}
    explicit DataReader(BYTE_VIEW_t buf, size_t pos) : DataStream(buf, pos) {}
    explicit DataReader(BYTES_t &buf, size_t pos) : DataStream(buf, pos) {}

public:
    template<typename T>
        requires 
            (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>
                || std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
    decltype(auto) read() {
        auto count = (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t>) 
            ? read<uint32_t>()
            : read_encoded_int();

        this->AssertOffset(count);

        auto result = T(this->data() + this->m_pos,
            this->data() + this->m_pos + count);

        this->SetPos(this->m_pos + count);

        return result;
    }

    
    // Reads a byte array as a Reader (more efficient than read_bytes())
    //  int32_t:   size
    //  BYTES_t:    data
    template<typename T>
        requires std::is_same_v<T, DataReader>
    decltype(auto) read() {
        return T(read<BYTE_VIEW_t>());
    }



    //  Reads a primitive type
    template<typename T> 
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    decltype(auto) read() {
        T out{};
        read_some_bytes(reinterpret_cast<BYTE_t*>(&out), sizeof(T));
        return out;
    }

    // Reads a container of supported types
    //  int32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
            && !std::is_arithmetic_v<typename Iterable::value_type>)
    decltype(auto) read() {
        using Type = Iterable::value_type;

        const auto count = read<int32_t>();

        Iterable out{};

        // TODO why is this here? should always reserve regardless
        //if constexpr (std::is_same_v<Type, std::string>)
        out.reserve(count);

        for (int32_t i=0; i < count; i++) {
            auto type = read<Type>();
            out.insert(out.end(), type);
        }

        return out;
    }
    
    // Reads a container<T> and runs a runction on each element
    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 1)
    void for_each(F func) {
        using Type = std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>;

        const auto count = read<int32_t>();

        for (int32_t i = 0; i < count; i++) {
            func(read<Type>());
        }
    }

    // Reads a ZDOID
    //  12 bytes total are read:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    template<typename T> requires std::same_as<T, ZDOID>
    decltype(auto) read() {
        auto a(read<int64_t>());
        auto b(read<uint32_t>());
        return ZDOID(a, b);
    }

    // Reads a Vector3f
    //  12 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    template<typename T> requires std::same_as<T, Vector3f>
    decltype(auto) read() {
        auto a(read<float>());
        auto b(read<float>());
        auto c(read<float>());
        return Vector3f(a, b, c);
    }

    // Reads a Vector2s
    //  8 bytes total are read:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    template<typename T> requires std::same_as<T, Vector2i>
    decltype(auto) read() {
        auto a(read<int32_t>());
        auto b(read<int32_t>());
        return Vector2i(a, b);
    }


    // Reads a Vector2s
    //  4 bytes total are read:
    //  int16_t: x (2 bytes)
    //  int16_t: y (2 bytes)
    template<typename T> requires std::same_as<T, Vector2s>
    decltype(auto) read() {
        auto a(read<int16_t>());
        auto b(read<int16_t>());
        return Vector2s(a, b);
    }



    // Reads a Quaternion
    //  16 bytes total are read:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    template<typename T> requires std::same_as<T, Quaternion>
    decltype(auto) read() {
        auto a(read<float>());
        auto b(read<float>());
        auto c(read<float>());
        auto d(read<float>());
        return Quaternion{ a, b, c, d };
    }

    // Reads an enum type
    //  - Bytes read depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    decltype(auto) read() {
        return static_cast<Enum>(read<std::underlying_type_t<Enum>>());
    }

    // Read a UTF-8 encoded C# char
    //  Will advance 1 -> 3 bytes, depending on size of first char
    template<typename T> requires std::is_same_v<T, char16_t>
    decltype(auto) read() {
        auto b1 = read<uint8_t>();

        // 3 byte
        if (b1 >= 0xE0) {
            auto b2 = read<uint8_t>() & 0x3F;
            auto b3 = read<uint8_t>() & 0x3F;
            return ((b1 & 0xF) << 12) | (b2 << 6) | b3;
        }
        // 2 byte
        else if (b1 >= 0xC0) {
            auto b2 = read<uint8_t>() & 0x3F;
            return ((b1 & 0x1F) << 6) | b2;
        }
        // 1 byte
        else {
            return b1 & 0x7F;
        }
    }

    template<typename T> requires std::is_same_v<T, UserProfile>
    decltype(auto) read() {
        auto name = read<std::string>();
        auto gamerTag = read<std::string>();
        auto networkUserId = read<std::string>();

        return UserProfile(std::move(name), std::move(gamerTag), std::move(networkUserId));
    }



    // verbose extension methods
    //  I want these to actually all be in lua
    //  templates in c, wrappers for lua in modman

    decltype(auto) read_bool() { return read<bool>(); }

    decltype(auto) read_string() { return read<std::string>(); }
    decltype(auto) read_strings() { return read<std::vector<std::string>>(); }

    decltype(auto) read_bytes() { return read<BYTES_t>(); }

    decltype(auto) read_zdoid() { return read<ZDOID>(); }
    decltype(auto) read_vec3f() { return read<Vector3f>(); }
    decltype(auto) read_vec2i() { return read<Vector2i>(); }
    decltype(auto) read_quat() { return read<Quaternion>(); }
    decltype(auto) read_profile() { return read<UserProfile>(); }

    decltype(auto) read_int8() { return read<int8_t>(); }
    decltype(auto) read_int16() { return read<int16_t>(); }
    decltype(auto) read_int32() { return read<int32_t>(); }
    decltype(auto) read_int64() { return read<int64_t>(); }
    decltype(auto) read_int64_wrapper() { return (Int64Wrapper) read<int64_t>(); }

    decltype(auto) read_uint8() { return read<uint8_t>(); }
    decltype(auto) read_uint16() { return read<uint16_t>(); }
    decltype(auto) read_uint32() { return read<uint32_t>(); }
    decltype(auto) read_uint64() { return read<uint64_t>(); }
    decltype(auto) read_uint64_wrapper() { return (UInt64Wrapper) read<uint64_t>(); }

    decltype(auto) read_float() { return read<float>(); }
    decltype(auto) read_double() { return read<double>(); }
    
    decltype(auto) read_char() { return read<char16_t>(); }



    // Deserialize a reader to a tuple of types
    template<class...Ts, class RD>
    static std::tuple<Ts...> deserialize(RD& reader) {
        return { reader.template read<Ts>()... };
    }


#if VH_IS_ON(VH_USE_MODS)
    sol::object single_deserialize_lua(sol::state_view state, IModManager::Type type) {
        switch (type) {
        case IModManager::Type::BYTES:
            // Will be interpreted as sol container type
            // see https://sol2.readthedocs.io/en/latest/containers.html
            return sol::make_object(state, read_bytes());
        case IModManager::Type::STRING:
            // Primitive: string
            return sol::make_object(state, read_string());
        case IModManager::Type::ZDOID:
            // Userdata: ZDOID
            return sol::make_object(state, read_zdoid());
        case IModManager::Type::VECTOR3f:
            // Userdata: Vector3f
            return sol::make_object(state, read_vec3f());
        case IModManager::Type::VECTOR2i:
            // Userdata: Vector2i
            return sol::make_object(state, read_vec2i());
        case IModManager::Type::QUATERNION:
            // Userdata: Quaternion
            return sol::make_object(state, read_quat());
        case IModManager::Type::STRINGS:
            // Container type of Primitive: string
            return sol::make_object(state, read_strings());
        case IModManager::Type::BOOL:
            // Primitive: boolean
            return sol::make_object(state, read_bool());
        case IModManager::Type::INT8:
            // Primitive: number
            return sol::make_object(state, read_int8());
        case IModManager::Type::INT16:
            // Primitive: number
            return sol::make_object(state, read_int16());
        case IModManager::Type::INT32:
            // Primitive: number
            return sol::make_object(state, read_int32());
        case IModManager::Type::INT64:
            // Userdata: Int64Wrapper
            return sol::make_object(state, read_int64());
        case IModManager::Type::UINT8:
            // Primitive: number
            return sol::make_object(state, read_uint8());
        case IModManager::Type::UINT16:
            // Primitive: number
            return sol::make_object(state, read_uint16());
        case IModManager::Type::UINT32:
            // Primitive: number
            return sol::make_object(state, read_uint32());
        case IModManager::Type::UINT64:
            // Userdata: UInt64Wrapper
            return sol::make_object(state, read_uint64_wrapper());
        case IModManager::Type::FLOAT:
            // Primitive: number
            return sol::make_object(state, read_float());
        case IModManager::Type::DOUBLE:
            // Primitive: number
            return sol::make_object(state, read_double());
        default:
            throw std::runtime_error("invalid mod DataReader type");
        }
    }

    sol::variadic_results deserialize_lua(sol::state_view state, const IModManager::Types& types) {
        sol::variadic_results results;

        for (auto&& type : types) {
            results.push_back(single_deserialize_lua(state, type));
        }

        return results;
    }
#endif
};
