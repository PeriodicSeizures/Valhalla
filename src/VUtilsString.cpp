
#include "VUtils.h"

namespace VUtils::String {
    HASH_t GetStableHashCode(const std::string& str) {
        int num = 5381;
        int num2 = num;
        int num3 = 0;
        while (str[num3] != '\0')
        {
            num = ((num << 5) + num) ^ (int)str[num3];
            if (str[num3 + 1] == '\0')
            {
                break;
            }
            num2 = ((num2 << 5) + num2) ^ (int)str[num3 + 1];
            num3 += 2;
        }
        return num + num2 * 1566083941;
    }

    std::vector<std::string_view> Split(const std::string& s, const std::string &delim) {
        std::string_view remaining(s);
        std::vector<std::string_view> result;
        int pos = 0;
        //ABC DE FGHI JK
        while ((pos = remaining.find(delim)) != std::string::npos) {
            // If the delim was not at idx 0, then add everything from 0 to the pos
            if (pos) result.push_back(remaining.substr(0, pos));
            // Trim everything before pos
            remaining = remaining.substr(pos + 1);
        }
        // add final match to list after delim
        if (!remaining.empty())
            result.push_back(remaining);
        return result;
    }

    void FormatAscii(std::string& in) {
        auto data = reinterpret_cast<BYTE_t*>(in.data());
        for (int i = 0; i < in.size(); i++) {
            if (data[i] > 127)
                data[i] = 63;
        }
    }

    // https://en.wikipedia.org/wiki/UTF-8#Encoding
    int32_t GetUTF8Count(const BYTE_t*p) {
        // leading bits:
        //   0: total 1 byte
        //   110: total 2 bytes (trailing 10xxxxxx)
        //   1110: total 3 bytes (trailing 10xxxxxx)
        //   11110: total 4 bytes (trailing 10xxxxxx)
        int32_t count = 0;
        for (; *p != '\0'; ++p, count++) {
#define CHECK_TRAILING_BYTES(n) \
        { \
            for (p++; /*next byte*/ \
                *p != '\0', i < (n); /*min bounds check*/ \
                ++p, ++i) /*increment*/ \
            { \
                if (((*p) >> 6) != 0b10) { \
                    return -1; \
                } \
            } \
            /* if string ended prematurely, panic */ \
            if (i != (n)) \
                return -1; \
        }

            // 1-byte code point
            if (((*p) >> 7) == 0b0) {
                continue;
            }
            else {
                int i = 0;
                // 2-byte code point
                if (((*p) >> 5) == 0b110) {
                    CHECK_TRAILING_BYTES(1);
                }
                    // 3-byte code point
                else if (((*p) >> 4) == 0b1110) {
                    CHECK_TRAILING_BYTES(2);
                }
                    // 4-byte code point
                else if (((*p) >> 3) == 0b11110) {
                    CHECK_TRAILING_BYTES(3);
                }
                else
                    return -1;
            }
        }
        return count;
    }
}