#pragma once

// A callback-based stream interface extracted from rapidjson::FileReadStream and rapidjson::FileWriteStream

struct load_stream {
    load_stream(size_t (*read_)(void *handle, void *buffer, size_t length), void *handle_) : read(read_), handle(handle_) {
        Read();
    }
    load_stream(const load_stream&) = delete;
    load_stream &operator=(const load_stream&) = delete;

    typedef char Ch;
    Ch Peek() const { return *current; }
    Ch Take() { Ch c = *current; Read(); return c; }
    size_t Tell() const { return count + static_cast<size_t>(current - buffer); }
    const Ch* Peek4() const { // For encoding detection only.
        return (current + 4 <= end) ? current : nullptr;
    }

    void Put(Ch) {}; void Flush(); Ch* PutBegin(); size_t PutEnd(Ch*); // Unimplemented

private:
    size_t (*read)(void *handle, void *buffer, size_t length);
    void *handle;
    char buffer[1024], *current = buffer, *end = nullptr;
    size_t count = 0, read_count = 0;
    bool eof = false;

    void Read() {
        if (current < end)
            ++current;
        else if (!eof) {
            count += read_count;
            read_count = read(handle, buffer, sizeof(buffer));
            end = buffer + read_count - 1;
            current = buffer;
            if (read_count < sizeof(buffer)) {
                buffer[read_count] = '\0';
                ++end;
                eof = true;
            }
        }
    }
};

#include <cstring>

struct save_stream {
    save_stream(void (*write_)(void *handle, const void *buffer, size_t length), void *handle_) : write(write_), handle(handle_) {}
    save_stream(const save_stream&) = delete;
    save_stream &operator=(const save_stream&) = delete;

    typedef char Ch;
    void Put(char c) {
        if (current >= end)
            Flush();
        *current++ = c;
    }

    void PutN(char c, size_t n) {
        size_t avail = static_cast<size_t>(end - current);
        while (n > avail) {
            std::memset(current, c, avail);
            current += avail;
            Flush();
            n -= avail;
            avail = static_cast<size_t>(end - current);
        }

        if (n > 0) {
            std::memset(current, c, n);
            current += n;
        }
    }

    void Flush() {
        if (current != buffer) {
            write(handle, buffer, static_cast<size_t>(current - buffer));
            current = buffer;
        }
    }

    Ch Peek(); Ch Take(); size_t Tell() const; Ch* PutBegin(); size_t PutEnd(Ch*); // Unimplemented

private:
    void (*write)(void *handle, const void *buffer, size_t length);
    void *handle;
    char buffer[1024], *current = buffer, *end = buffer + sizeof(buffer);
};

#include "rapidjson/rapidjson.h"

namespace rapidjson {
    //! Implement specialized version of PutN() with memset() for better performance.
    template<>
    inline void PutN(save_stream& stream, char c, size_t n) {
        stream.PutN(c, n);
    }
}
