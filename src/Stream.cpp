#include "Stream.h"

Stream::Stream(uint32_t count) {
    Reserve(count);
}

// Copy constructor
Stream::Stream(const Stream& other)
    : Stream(other.m_alloc)
{
    // Retrieving data from m_alloc bytes location is undefined, 
    // so only bytes from pos 0 to m_length is utilized
    Write(other.Bytes(), other.m_length);
}


#ifdef REALLOC_STREAM
Stream::~Stream() {
    free(m_bytes);
}
#endif


void Stream::Read(byte_t* buffer, uint32_t count) {
    if (m_marker + count > m_length) throw std::range_error("Stream::Read(byte_t* buffer, int count) length exceeded");

    std::memcpy(buffer, 
        Bytes() + m_marker,
        count);
    m_marker += count;
}

byte_t Stream::Read() {
    byte_t b;
    Read(&b, 1);
    return b;
}

void Stream::Read(std::vector<byte_t>& vec, uint32_t count) {
    if (m_marker + count > m_length) throw std::range_error("Stream::Read(std::vector<byte_t>& vec, int count) length exceeded");
    
    vec.insert(vec.end(), 
        Bytes() + m_marker, 
        Bytes() + m_marker + count);
    m_marker += count;
}

void Stream::Read(std::string& s, uint32_t count) {
    if (m_marker + count > m_length) throw std::range_error("Stream::Read(std::string& s, int count) length exceeded");

    s.insert(s.end(),
        Bytes() + m_marker,
        Bytes() + m_marker + count);
    m_marker += count;
}



void Stream::Write(const byte_t* buffer, uint32_t count) {
    EnsureCapacity(m_marker + count);
    std::memcpy(Bytes() + m_marker, buffer, count);
    m_marker += count;
    m_length += count;
}



void Stream::SetLength(uint32_t length) {
    if (length > m_alloc) throw std::runtime_error("length exceeds allocated memory");

    m_length = length;
}




void Stream::SetMarker(uint32_t marker) {
    if (marker > m_length) throw std::runtime_error("marks exceeds remaining length");

    m_marker = marker;
}



void Stream::Reserve(uint32_t count) {
    if (m_alloc < count) {
#ifdef REALLOC_STREAM
        m_bytes = (byte_t*) realloc(m_bytes, sizeof(byte_t) * count);
        if (!m_bytes)
            throw std::runtime_error("Stream failed to realloc; rare exception");
#else
        auto oldPtr = std::move(m_bytes);
        m_bytes = std::unique_ptr<byte_t>(new byte_t[count]);
        if (oldPtr) {
            std::memcpy(m_bytes.get(), oldPtr.get(), static_cast<size_t>(count));
        }
#endif
        m_alloc = count;
    }
}
