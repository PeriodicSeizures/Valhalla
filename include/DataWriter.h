#pragma once

#include "VUtils.h"
#include "ZDOID.h"
#include "Vector.h"
#include "Quaternion.h"
#include "UserData.h"
#include "VUtilsTraits.h"
#include "ModManager.h"
#include "DataStream.h"
#include "DataReader.h"

class DataWriter : public virtual DataStream {
private:
    // Write count bytes from the specified buffer
    // Bytes are written in place, making space as necessary
    void WriteSomeBytes(const BYTE_t* buffer, size_t count) {
        std::visit(VUtils::Traits::overload{
            [this, count](std::reference_wrapper<BYTES_t> buf) { 
                if (this->CheckOffset(count))
                    buf.get().resize(this->m_pos + count);
            },
            [this, count](BYTE_VIEW_t buf) { this->AssertOffset(count); }
        }, this->m_data);

        std::copy(buffer,
                  buffer + count, 
                  this->data() + this->m_pos);

        this->m_pos += count;
    }

    // Write count bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto WriteSomeBytes(const BYTES_t& vec, size_t count) {
        return WriteSomeBytes(vec.data(), count);
    }

    // Write all bytes from the specified vector
    // Bytes are written in place, making space as necessary
    auto WriteSomeBytes(const BYTES_t& vec) {
        return WriteSomeBytes(vec, vec.size());
    }

    void Write7BitEncodedInt(int32_t value) {
        auto num = static_cast<uint32_t>(value);
        for (; num >= 128U; num >>= 7)
            Write((BYTE_t)(num | 128U));

        Write((BYTE_t)num);
    }

public:
    explicit DataWriter(BYTE_VIEW_t buf) : DataStream(buf) {}
    explicit DataWriter(BYTES_t &buf) : DataStream(buf) {}
    explicit DataWriter(BYTE_VIEW_t buf, size_t pos) : DataStream(buf, pos) {}
    explicit DataWriter(BYTES_t &buf, size_t pos) : DataStream(buf, pos) {}

    /*
    // Clears the underlying container and resets position
    // TODO standardize by renaming to 'clear'
    void Clear() {
        if (this->owned()) {
            this->m_pos = 0;
            this->m_ownedBuf.clear();
        }
        else {
            throw std::runtime_error("tried calling Clear() on unownedBuf DataWriter");
        }
    }*/

    // Sets the length of this stream
    //void SetLength(uint32_t length) {
    //    m_provider.get().resize(length);
    //}

public:
    /*
    DataReader ToReader() {
        if constexpr (std::is_same_v<T, BYTE_VIEW_t>)
            return DataReader(this->m_buf, this->m_pos);
        else
            return DataReader(this->m_buf.get(), this->m_pos);
    }*/

    template<typename F>
        requires (std::tuple_size<typename VUtils::Traits::func_traits<F>::args_type>{} == 1)
    void SubWrite(F func) {
        const auto start = this->Position();
        int32_t count = 0;
        Write(count);

        // call func...
        func(std::ref(*this));

        const auto end = this->Position();
        this->SetPos(start);
        count = end - start - sizeof(count);
        assert(count >= 0);
        Write(count);
        this->SetPos(end);
    }

    void Write(const BYTE_t* in, size_t length) {
        Write<int32_t>(length);
        WriteSomeBytes(in, length);
    }

    template<typename T>
        requires (std::is_same_v<T, BYTES_t> || std::is_same_v<T, BYTE_VIEW_t> || std::is_same_v<T, DataReader>)
    void Write(const T& in) {
        Write(in.data(), in.size());
        //Write<int32_t>(in.size());
        //WriteSomeBytes(in.data(), in.size());
    }

    // Writes a string
    void Write(std::string_view in) {
        auto length = in.length();

        auto byteCount = static_cast<int32_t>(length);

        Write7BitEncodedInt(byteCount);
        if (byteCount == 0)
            return;

        WriteSomeBytes(reinterpret_cast<const BYTE_t*>(in.data()), byteCount);
    }

    // Writes a ZDOID
    //  12 bytes total are written:
    //  int64_t:    owner (8 bytes)
    //  uint32_t:   uid (4 bytes)
    void Write(ZDOID in) {
        Write(in.GetOwner());
        Write(in.GetUID());
    }

    // Writes a Vector3f
    //  12 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    void Write(Vector3f in) {
        Write(in.x);
        Write(in.y);
        Write(in.z);
    }

    // Writes a Vector2i
    //  8 bytes total are written:
    //  int32_t: x (4 bytes)
    //  int32_t: y (4 bytes)
    void Write(Vector2i in) {
        Write(in.x);
        Write(in.y);
    }

    // Writes a Quaternion
    //  16 bytes total are written:
    //  float: x (4 bytes)
    //  float: y (4 bytes)
    //  float: z (4 bytes)
    //  float: w (4 bytes)
    void Write(Quaternion in) {
        Write(in.x);
        Write(in.y);
        Write(in.z);
        Write(in.w);
    }

    // Writes a container of supported types
    //  uint32_t:   size
    //  T...:       value_type
    template<typename Iterable> 
        requires (VUtils::Traits::is_iterable_v<Iterable> 
                && !std::is_arithmetic_v<typename Iterable::value_type>)
    void Write(const Iterable& in) {
        size_t size = in.size();
        Write(static_cast<int32_t>(size));
        for (auto&& v : in) {
            Write(v);
        }
    }

    // Writes a primitive type
    template<typename T> 
        requires (std::is_arithmetic_v<T> && !std::is_same_v<T, char16_t>)
    void Write(T in) { WriteSomeBytes(reinterpret_cast<const BYTE_t*>(&in), sizeof(T)); }

    // Writes an enum
    //  Bytes written depend on the underlying value
    template<typename Enum> requires std::is_enum_v<Enum>
    void Write(Enum value) {
        Write(std::to_underlying(value));
    }

    //template<typename T> requires std::is_same_v<T, char16_t>
    //void Write(T in) {
    void Write(char16_t in) {
        if (in < 0x80) {
            Write<BYTE_t>(in);
        }
        else if (in < 0x0800) {
            Write<BYTE_t>(((in >> 6) & 0x1F) | 0xC0);
            Write<BYTE_t>(((in >> 0) & 0x3F) | 0x80);
        }
        else { // if (i < 0x010000) {
            Write<BYTE_t>(((in >> 12) & 0x0F) | 0xE0);
            Write<BYTE_t>(((in >> 6) & 0x3F) | 0x80);
            Write<BYTE_t>(((in >> 0) & 0x3F) | 0x80);
        }
    }

    void Write(const UserProfile &in) {
        Write(std::string_view(in.m_name));
        Write(std::string_view(in.m_gamerTag));
        Write(std::string_view(in.m_networkUserId));
    }

    void Write(UInt64Wrapper in) {
        Write((uint64_t)in);
    }

    void Write(Int64Wrapper in) {
        Write((int64_t)in);
    }



    // Empty template
    static void SerializeImpl(DataWriter& pkg) {}

    // Writes variadic parameters into a package
    template <typename T, typename... Types>
    static decltype(auto) SerializeImpl(DataWriter& pkg, const T &var1, const Types&... var2) {
        pkg.Write(var1);

        return SerializeImpl(pkg, var2...);
    }

    // Serialize variadic types to an array
    template <typename T, typename... Types>
    static decltype(auto) Serialize(const T &var1, const Types&... var2) {
        BYTES_t bytes;
        DataWriter writer(bytes);

        SerializeImpl(writer, var1, var2...);
        return bytes;
    }

    // empty full template
    static decltype(auto) Serialize() {
        return BYTES_t{};
    }



    void SerializeOneLua(IModManager::Type type, sol::object arg) {
        switch (type) {
            // TODO add recent unsigned types
        case IModManager::Type::UINT8:
            Write(arg.as<uint8_t>());
            break;
        case IModManager::Type::UINT16:
            Write(arg.as<uint16_t>());
            break;
        case IModManager::Type::UINT32:
            Write(arg.as<uint32_t>());
            break;
        case IModManager::Type::UINT64:
            Write(arg.as<uint64_t>());
            break;
        case IModManager::Type::INT8:
            Write(arg.as<int8_t>());
            break;
        case IModManager::Type::INT16:
            Write(arg.as<int16_t>());
            break;
        case IModManager::Type::INT32:
            Write(arg.as<int32_t>());
            break;
        case IModManager::Type::INT64:
            Write(arg.as<int64_t>());
            break;
        case IModManager::Type::FLOAT:
            Write(arg.as<float>());
            break;
        case IModManager::Type::DOUBLE:
            Write(arg.as<double>());
            break;
        case IModManager::Type::STRING:
            Write(arg.as<std::string>());
            break;
        case IModManager::Type::BOOL:
            Write(arg.as<bool>());
            break;
        case IModManager::Type::BYTES:
            Write(arg.as<BYTES_t>());
            break;
        case IModManager::Type::ZDOID:
            Write(arg.as<ZDOID>());
            break;
        case IModManager::Type::VECTOR3f:
            Write(arg.as<Vector3f>());
            break;
        case IModManager::Type::VECTOR2i:
            Write(arg.as<Vector2i>());
            break;
        case IModManager::Type::QUATERNION:
            Write(arg.as<Quaternion>());
            break;
        default:
            throw std::runtime_error("Invalid data type");
        }
    }

    void SerializeLua(const IModManager::Types& types, const sol::variadic_results& results) {
        for (int i = 0; i < results.size(); i++) {
            SerializeOneLua(types[i], results[i]);
        }
    }

    static decltype(auto) SerializeExtLua(const IModManager::Types& types, const sol::variadic_results& results) {
        BYTES_t bytes;
        DataWriter params(bytes);

        params.SerializeLua(types, results);

        return bytes;
    }
};
