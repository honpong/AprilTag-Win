/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2016-2017 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#pragma once

#include <array>
#include <tuple>
#include <vector>
#include <memory>
#include <string.h>
#include <rc_tracker.h>
#include "transformation.h"

#define STREAM_BUFFER_SIZE 10240 //a reasonable compromise of file i/o vs. memory consumption

template <typename T>
struct is_std_pair : std::false_type { };

template <typename Key, typename T>
struct is_std_pair<std::pair<Key, T>> : std::true_type { };

/**
converts values of a given data structure into char[] representation before streaming.
*/
class bstream_writer {
public:
    bstream_writer(rc_SaveCallback func_, void *handle_) : out_func(func_), handle(handle_), max_offset(STREAM_BUFFER_SIZE) {};

    template <typename T> 
    typename std::enable_if<std::is_fundamental<T>::value, bstream_writer&>::type operator << (const T& data) { return write((const char*)&data, sizeof(T)); }
    template <typename T> 
    typename std::enable_if<std::is_fundamental<T>::value, bstream_writer&>::type operator << (const T&& data) { return write((const char*)&data, sizeof(T)); }
    template <template <class, class> class TContainer, class T, class Alloc>
    bstream_writer& operator << (const TContainer<T, Alloc> &c) {return write_container(c.begin(), c.end());}
    template <class T, class Alloc>
    bstream_writer& operator << (const std::vector<T, Alloc>& c) { return operator << <std::vector, T, Alloc>(c); }
    template <typename T, std::size_t N>
    bstream_writer& operator << (const std::array<T, N> &c) { return write_array((const char*)c.data(), N * sizeof(T)); }

    template <template <class, class, class, class...> class TMap, class Key, class T, class Comp, class... TArgs>
    bstream_writer& operator << (const TMap<Key, T, Comp, TArgs...> &c) {
        *this << (uint64_t)c.size();
        total_io_bytes += sizeof(uint64_t);
        if (!save_sorted)
            for (const auto &ele : c) *this << ele;
        else { // sort iterators before saving
            typedef typename TMap<Key, T, Comp, TArgs...>::const_iterator cont_itr;
            std::vector<cont_itr> sorted_ele;
            uint32_t idx = 0;
            for (auto itr = c.begin(); itr != c.end(); itr++, idx++) sorted_ele.push_back(itr);
            std::sort(sorted_ele.begin(), sorted_ele.end(), [](const cont_itr &e1, const cont_itr &e2)->bool {
                return (get_key<typename TMap<Key, T, Comp, TArgs...>::value_type, Key>(*e1) <
                        get_key<typename TMap<Key, T, Comp, TArgs...>::value_type, Key>(*e2)); });
            for (const auto &ele : sorted_ele) *this << *ele;
        }
        return *this;
    }

    template<typename T>
    bstream_writer& write_array(const T *data, size_t length) {
        *this << (uint64_t)length;
        total_io_bytes += sizeof(uint64_t);
        write(data, length);
        return *this;
    }

    bstream_writer& write(const char* data, size_t length) {
        for (size_t written = 0, chunk = 0; written < length; written += chunk)
            write_part(data + written, chunk = std::min(length - written, max_offset));
        return *this;
    }

    /// flush writing and generate an end stream callback (e.g. writing a zero length buffer).
    /// Note: required call to complete the current writing in multi-sessions writing
    void end_stream() {
        flush_stream();
        if (out_func) {
            out_func(handle, buffer.get(), 0); //signal end of data
            out_func = nullptr;
        }
    }

    ~bstream_writer() {
        end_stream();
    }

    void set_failed() { is_good = false; }
    bool good() { flush_stream(); return is_good; }

    uint64_t total_io_bytes{ 0 }; /// total bytes read or written, for debugging.
    static bool save_sorted; /// whether to save elements of unordered_map/unordered_set in sorted order.
private:
    rc_SaveCallback out_func{ nullptr };
    bool is_good{ true };
    void *handle{ nullptr };
    std::unique_ptr<char[]> buffer{ new char[STREAM_BUFFER_SIZE] };
    size_t offset{ 0 }, max_offset{ 0 }; //streaming offset from the start of buffer

    void flush_stream() {
        if (out_func && offset > 0) {
            out_func(handle, buffer.get(), offset);
            offset = 0;
        }
    }

    template<typename T>
    bstream_writer& write(const T& data) { return write((const char *)&data, sizeof(T)); }

    template<typename T, typename... Args>
    bstream_writer& write(T &first, Args &... args) { return write(first).write(args...);}
    template<typename T>
    bstream_writer& write_container(T from_itr, T to_itr) {
        *this << (uint64_t)std::distance(from_itr, to_itr);
        total_io_bytes += sizeof(uint64_t);
        for (auto itr = from_itr; itr != to_itr; itr++) { *this << *itr; }
        return *this;
    }
    void write_part(const char* data, size_t length) {
        if (offset + length > max_offset) { //need to flush buffer
            out_func(handle, buffer.get(), offset);
            offset = 0;
        }
        memcpy(buffer.get() + offset, (const char*)data, length);
        offset += length;
        total_io_bytes += length;
    }
    template <class Key, class T>
    bstream_writer& operator << (const std::pair<const Key, T> &ele) { return *this << ele.first << ele.second; }

    template <typename E, class Key, typename std::enable_if<is_std_pair<E>::value, int>::type = 0>
    static Key get_key(const E &ele) { return ele.first; }

    template <typename E, typename..., typename std::enable_if<!is_std_pair<E>::value, int>::type = 0>
    static E get_key(const E &ele) { return ele; }

    bstream_writer() = delete;
};

/**
takes a stream of char[] representation and populates values for given data structures.
*/
class bstream_reader {
public:
    bstream_reader(const rc_LoadCallback func_, void *handle_) : in_func(func_), handle(handle_) {};
    template <typename T>
    typename std::enable_if<std::is_fundamental<T>::value, bstream_reader&>::type operator >> (T& data) { return read(data); }
    template <typename T>
    typename std::enable_if<std::is_fundamental<T>::value, bstream_reader&>::type operator >> (T&& data) { return read(data); }
    template <template <class, class> class TContainer, class T, class Alloc>
    bstream_reader& operator >> (TContainer<T, Alloc> &c) { return read_container(c, [&](uint64_t new_size)->bool{
        c.resize(new_size); //TODO: check against size of pre-allocated memory for map
        return true;
    });}
    template <class T, class Alloc>
    bstream_reader& operator >> (std::vector<T, Alloc> &c) { return operator >> <std::vector, T, Alloc>(c); }
    template <typename T, std::size_t N>
    bstream_reader& operator >> (std::array<T, N> &c) { return read_array((char*)c.data(), N * sizeof(T)); }

    template <template <class, class, class, class...> class TMap, class Key, class T, class Comp, class... TArgs>
    bstream_reader& operator >> (TMap<Key, T, Comp, TArgs...> &c) {
        if (!is_good) return *this;
        uint64_t c_size = 0;
        read(c_size);
        for (size_t i = 0; is_good && i < c_size; i++)
            c.insert(read_ele<typename TMap<Key, T, Comp, TArgs...>::value_type, Key, T>());
        return *this;
    }

    bstream_reader& read_array(char* data, size_t length) {
        if (!is_good) return *this;
        uint64_t c_size = 0;
        read(c_size);
        is_good = is_good && (c_size == length);
        read(data, length);
        return *this;
    }

    bstream_reader& read(char* data, size_t length) {
        for (size_t read_bytes = 0, chunk = 0; read_bytes < length; read_bytes += chunk)
            read_part(data + read_bytes, chunk = std::min(length - read_bytes, (size_t)STREAM_BUFFER_SIZE));
        return *this;
    }

    /// set failure status to stop loading as appropriate.
    void set_failed() { is_good = false; }

    bool good() { return is_good; }

    virtual ~bstream_reader() {};
private:
    const rc_LoadCallback in_func{ nullptr };
    bool is_good{ true };
    void *handle{ nullptr };
    std::unique_ptr<char[]> buffer{ new char[STREAM_BUFFER_SIZE] };
    size_t offset{ 0 }, max_offset{ 0 }; //streaming offset from the start of buffer

    template<typename T>
    bstream_reader& read(T& data) { return read((char *)&data, sizeof(T)); }

    template<typename T, typename... Args>
    bstream_reader& read(T &first, Args &... args) { return read(first).read(args...); }

    template<typename T, typename Func>
    bstream_reader& read_container(T &c, Func sizing) {
        if (!is_good) return *this;
        uint64_t c_size = 0;//size_t is not compatible
        read(c_size);
        is_good = is_good && sizing(c_size);
        for (auto itr = c.begin(); is_good && itr != c.end(); itr++) { *this >> *itr; }
        return *this;
    }

    bstream_reader& read_part(char* data, size_t length) {
        size_t remain = max_offset - offset;
        if (remain < length) { //need to update buffer
            if (remain > 0) memmove(buffer.get(), buffer.get() + offset, remain); //move unread content to start
            int32_t bytes_read = in_func(handle, buffer.get() + remain, STREAM_BUFFER_SIZE - remain);
            max_offset = remain + bytes_read;
            offset = 0;
            is_good = is_good && (bytes_read > 0) && (length <= max_offset);
        }
        if (is_good) {
            memcpy(data, buffer.get() + offset, length);
            offset += length;
        }
        return *this;
    }

    template <typename E, class Key, class T, typename std::enable_if<is_std_pair<E>::value, int>::type = 0>
    E read_ele() {
        std::pair<Key, T> ele;
        *this >> ele.first >> ele.second;
        return ele;
    }

    template <typename E, typename..., typename std::enable_if<!is_std_pair<E>::value, int>::type = 0>
    E read_ele() {
        E ele;
        *this >> ele;
        return ele;
    }

    bstream_reader() = delete;
};

bstream_reader & operator >> (bstream_reader & cur_stream, transformation &transform);
bstream_writer & operator << (bstream_writer & cur_stream, const transformation &transform);
