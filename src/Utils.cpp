#include <random>
#include <limits>
#include "Utils.hpp"

namespace Utils {

    // https://stackoverflow.com/questions/12398377/is-it-possible-to-have-zlib-read-from-and-write-to-the-same-memory-buffer
    int Compress(z_stream* strm, unsigned char* buf, unsigned len,
        unsigned* max)
    {
        int ret;                    /* return code from deflate functions */
        unsigned have;              /* number of bytes in temp[] */
        unsigned char* hold;        /* allocated buffer to hold input data */
        unsigned char temp[11];     /* must be large enough to hold zlib or gzip
                                       header (if any) and one more byte -- 11
                                       works for the worst case here, but if gzip
                                       encoding is used and a deflateSetHeader()
                                       call is inserted in this code after the
                                       deflateReset(), then the 11 needs to be
                                       increased to accomodate the resulting gzip
                                       header size plus one */

                                       /* initialize deflate stream and point to the input data */
        ret = deflateReset(strm);
        if (ret != Z_OK)
            return ret;
        strm->next_in = buf;
        strm->avail_in = len;

        /* kick start the process with a temporary output buffer -- this allows
           deflate to consume a large chunk of input data in order to make room for
           output data there */
        if (*max < len)
            *max = len;
        strm->next_out = temp;
        strm->avail_out = sizeof(temp) > *max ? *max : sizeof(temp);
        ret = deflate(strm, Z_FINISH);
        if (ret == Z_STREAM_ERROR)
            return ret;

        /* if we can, copy the temporary output data to the consumed portion of the
           input buffer, and then continue to write up to the start of the consumed
           input for as long as possible */
        have = strm->next_out - temp;
        if (have <= (strm->avail_in ? len - strm->avail_in : *max)) {
            memcpy(buf, temp, have);
            strm->next_out = buf + have;
            have = 0;
            while (ret == Z_OK) {
                strm->avail_out = strm->avail_in ? strm->next_in - strm->next_out :
                    (buf + *max) - strm->next_out;
                ret = deflate(strm, Z_FINISH);
            }
            if (ret != Z_BUF_ERROR || strm->avail_in == 0) {
                *max = strm->next_out - buf;
                return ret == Z_STREAM_END ? Z_OK : ret;
            }
        }

        /* the output caught up with the input due to insufficiently compressible
           data -- copy the remaining input data into an allocated buffer and
           complete the compression from there to the now empty input buffer (this
           will only occur for long incompressible streams, more than ~20 MB for
           the default deflate memLevel of 8, or when *max is too small and less
           than the length of the header plus one byte) */
        hold = (unsigned char*)strm->zalloc(strm->opaque, strm->avail_in, 1);
        if (hold == Z_NULL)
            return Z_MEM_ERROR;
        memcpy(hold, strm->next_in, strm->avail_in);
        strm->next_in = hold;
        if (have) {
            memcpy(buf, temp, have);
            strm->next_out = buf + have;
        }
        strm->avail_out = (buf + *max) - strm->next_out;
        ret = deflate(strm, Z_FINISH);
        strm->zfree(strm->opaque, hold);
        *max = strm->next_out - buf;
        return ret == Z_OK ? Z_BUF_ERROR : (ret == Z_STREAM_END ? Z_OK : ret);
    }

    // https://zlib.net/zpipe.c
    std::vector<byte> Compress(const byte* uncompressedBytes, int count, int level) {
        std::vector<byte> ret(count);

        z_stream zs;
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.avail_in = (uInt)count;
        zs.next_in = (Bytef*)uncompressedBytes;
        zs.avail_out = (uInt)ret.size(); // HERE
        zs.next_out = (Bytef*)ret.data();

        if (deflateInit2(&zs, level, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
            throw std::runtime_error("unable to compress");

        deflate(&zs, Z_FINISH);
        if (deflateEnd(&zs) != Z_OK)
            throw std::runtime_error("compression error");

        ret.resize(zs.total_out);

        return ret;
    }

    std::vector<byte> Decompress(const byte* compressedBytes, int count) {
        std::vector<byte> ret;

        if (count == 0)
            return ret;

        unsigned full_length = count;
        unsigned half_length = count / 2;

        unsigned uncompLength = full_length;
        std::unique_ptr<byte> uncomp = std::unique_ptr<byte>(new byte[uncompLength]);

        z_stream stream;
        stream.next_in = (Bytef*)compressedBytes;
        stream.avail_in = count;
        stream.total_out = 0;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;

        bool done = false;

        if (inflateInit2(&stream, (16 + MAX_WBITS)) != Z_OK)
            throw std::runtime_error("unable to decompress gzip stream");

        while (!done) {
            // If our output buffer is too small  
            if (stream.total_out >= uncompLength) {
                // Increase size of output buffer  
                auto old = std::move(uncomp);
                uncomp = std::unique_ptr<byte>(new byte[uncompLength + half_length]);
                memcpy(uncomp.get(), old.get(), uncompLength);
                uncompLength += half_length;
            }

            stream.next_out = (Bytef*)(uncomp.get() + stream.total_out);
            stream.avail_out = uncompLength - stream.total_out;

            // Inflate another chunk.  
            int err = inflate(&stream, Z_SYNC_FLUSH);
            if (err == Z_STREAM_END) done = true;
            else if (err != Z_OK) {
                throw std::runtime_error("decompression error " + std::to_string(err));
            }
        }

        if (inflateEnd(&stream) != Z_OK)
            throw std::runtime_error("unable to end decompressing gzip stream");

        ret.insert(ret.begin(), uncomp.get(), uncomp.get() + stream.total_out);

        return ret;
    }

    std::random_device rd;     //Get a random seed from the OS entropy device, or whatever
    std::mt19937_64 eng(rd()); //Use the 64-bit Mersenne Twister 19937 generator
                               //and seed it with entropy.

    UID_t GenerateUID() {
        //Define the distribution, by default it goes from 0 to MAX(unsigned long long)
        //or what have you.
        std::uniform_int_distribution<int64_t> distr;
        return distr(eng);
    }

    int32_t GetStableHashCode(const char* str) {
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

    int32_t GetUnicode8Count(const char* p) {
        int count = 0;
        for (p; *p != 0; ++p)
            count += ((*p & 0xc0) != 0x80);

        return count;
    }

    UID_t StringToUID(std::string_view sv) {
        std::string s(sv);
        std::stringstream ss(s);
        UID_t uid;
        ss >> uid;
        return uid;
    }

    bool IsAddress(std::string_view s) {
        //address make_address(const char* str,
        //    asio::error_code & ec) ASIO_NOEXCEPT
        asio::error_code ec;
        asio::ip::make_address(s, ec);
        return ec ? false : true;
    }

    std::string Join(std::vector<std::string_view>& strings) {
        std::string result;
        for (auto& s : strings) {
            result += s;
        }
        return result;
    }

    std::vector<std::string_view> Split(std::string_view s, char ch) {
        //std::string s = "scott>=tiger>=mushroom";

        // split in Java appears to be a recursive decay function (for the pattern)

        int off = 0;
        int next = 0;
        std::vector<std::string_view> list;
        while ((next = s.find(ch, off)) != std::string::npos) {
            list.push_back(s.substr(off, next));
            off = next + 1;
        }
        // If no match was found, return this
        if (off == 0)
            return { s };

        // Add remaining segment
        list.push_back(s.substr(off));

        // Construct result
        int resultSize = list.size();
        while (resultSize > 0 && list[resultSize - 1].empty()) {
            resultSize--;
        }

        return std::vector<std::string_view>(list.begin(), list.begin() + resultSize);






        //std::vector<std::string_view> res;
        //
        //size_t pos = 0;
        //while ((pos = s.find(delimiter)) != std::string::npos) {
        //    //std::cout << token << std::endl;
        //    res.push_back(s.substr(0, pos));
        //    //s.erase(0, pos + delimiter.length());
        //    if (pos + delimiter.length() == s.length())
        //        s = s.substr(pos + delimiter.length());
        //    else s = s.substr(pos + delimiter.length());
        //}
        //if (res.empty())
        //    res.push_back(s);
        ////std::cout << s << std::endl;
        //return res;
    }
}
