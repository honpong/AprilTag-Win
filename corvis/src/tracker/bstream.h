#pragma once

#include <array>
#include <tuple>
#include <vector>
#include <memory>
#include <algorithm>
#include <string.h>
#include "rc_tracker.h"

#define STREAM_BUFFER_SIZE 10240 //a reasonable compromise of file i/o vs. memory consumption
class host_stream;

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
    template <typename T, std::size_t N>
    bstream_writer& operator << (const T (&c)[N]) { return write_array((const char*)&c[0], N * sizeof(T)); }
    template <typename T>
    bstream_writer& operator << (const std::basic_string<T> &c) { *this << (uint32_t)c.size(); return write_array(c.data(),c.size()); }
    template <typename T>
    bstream_writer& operator << (const std::unique_ptr<T> &c) { *this << uint8_t(c?1:0); if (c) *this << *c; return *this; }

    template <template <class, class, class, class...> class TMap, class Key, class T, class Comp, class... TArgs>
    bstream_writer& operator << (const TMap<Key, T, Comp, TArgs...> &c) {
        *this << (uint64_t)c.size();
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

    void flush_stream() {
        if (out_func && offset > 0) {
            out_func(handle, buffer.get(), offset);
            offset = 0;
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

    template<typename T>
    bstream_writer& write(const T& data) { return write((const char *)&data, sizeof(T)); }

    template<typename T, typename... Args>
    bstream_writer& write(T &first, Args &... args) { return write(first).write(args...);}
    template<typename T>
    bstream_writer& write_container(T from_itr, T to_itr) {
        *this << (uint64_t)std::distance(from_itr, to_itr);
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
    template <typename T, std::size_t N>
    bstream_reader& operator >> (T(&c)[N]) { return read_array((char*)&c[0], N * sizeof(T)); }
    template <typename T>
    bstream_reader& operator >> (std::basic_string<T> &c) { uint32_t s=0; *this >> s; c.resize(s); return read_array((char*)c.data(), s * sizeof(T)); }
    template <typename T>
    bstream_reader& operator >> (std::unique_ptr<T> &c) { uint8_t p=0; *this >> p; if (p) { c = std::make_unique<T>(); *this >> *c; } return *this; }

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
        for (auto itr = std::begin(c); is_good && itr != std::end(c); itr++) { *this >> *itr; }
        return *this;
    }

    bstream_reader& read_part(char* data, size_t length) {
        size_t remain = max_offset - offset;
        if (remain < length) { //need to update buffer
            if (remain > 0) memmove(buffer.get(), buffer.get() + offset, remain); //move unread content to start
            offset = 0;
            max_offset = remain;
            for (size_t bytes_read = 0; max_offset < length; max_offset += bytes_read) //continue to get sufficient data
                if (!(bytes_read = in_func(handle, buffer.get() + max_offset, STREAM_BUFFER_SIZE - max_offset)))
                    break;
            is_good = is_good && (length <= max_offset);
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

size_t mem_load_callback(void * handle, void *buffer, size_t length);

class pose_feature_output {
private:
    // ---- C O N F I G U R A T I O N---------------
    // upper limits for transferring out of TM2 in order to keep USB message size constant.
    //----------------------------------------------

    const uint32_t max_reloc_pose = 10; ///max number of relocalization candidates out of TM2
    const uint32_t max_features = 70; /// max number of image features out of TM2
public:
    const rc_PoseTime &rc_getPose(rc_DataPath path, rc_PoseVelocity *v = nullptr,
        rc_PoseAcceleration *a = nullptr) const;
    void rc_setPose(rc_DataPath path, const rc_PoseTime &&pt, const rc_PoseVelocity *v = nullptr,
        const rc_PoseAcceleration *a = nullptr);
    /// reuse the memory pointed to by parameter feat, hence memory must remain valid.
    void rc_setFeatures(rc_Feature *feat, uint32_t num_feat);
    void rc_resetFeatures();
    /// if replay_output object is brought up from a stream, features are available up to max_save_features.
    uint32_t rc_getFeatures(rc_Feature **feat) const;
    typedef enum class output_mode {
        POSE_ONLY = 0, POSE_FEATURE, POSE_MAP, POSE_FEATURE_MAP
    } output_mode;
    bool set_data(bstream_reader &cur_stream);
    size_t get_data(bstream_writer &cur_stream) const;
    size_t get_feature_size() const;

    /// reuse the memory pointed to by parameter pose, hence memory must remain valid.
    void rc_setRelocPoses(rc_Pose *reloc_pose, uint32_t num_poses);
    void rc_resetRelocPoses();
    /// if replay_output object is brought up from a stream, poses are available up to max_reloc_pose.
    uint32_t rc_getRelocPoses(rc_Pose **reloc_pose) const;
    size_t get_reloc_poses_size() const;

    virtual ~pose_feature_output() {};
protected:
    output_mode output_type{ output_mode::POSE_ONLY };
    struct {
        struct rc_PoseOutput {
            rc_PoseTime pose_time;
            rc_PoseVelocity veloc;
            rc_PoseAcceleration accel;
        } fast, slow;
    } poses;
private:
    /// re-use pointer to features by rc_tracker, is mutually exclusive with features.
    rc_Feature * reuse_features_ptr{ nullptr };
    /// allocated pointer to hold features, mutually exclusive with reuse_features_ptr.
    std::unique_ptr<rc_Feature[]> features;
    uint32_t num_features{ 0 };
    /// re-use pointer to relocalization poses by rc_tracker, is mutually exclusive with reloc_poses.
    rc_Pose *reuse_reloc_poses_ptr{ nullptr };
    /// allocated pointer to hold features, mutually exclusive with reuse_features_ptr.
    std::unique_ptr<rc_Pose[]> reloc_poses;
    uint32_t num_reloc_poses{ 0 };
};

/// holds tracking output that includes camera poses, pose velocity, pose acceleration, and confidence,
/// (traveled) path length and timestamp, type, data path of the sensor data associated with the pose.
/// It optionally includes features detected on the camera input.
class replay_output: public pose_feature_output {
public:
    host_stream *host{ nullptr }; /// variable used if device and host are in the same memory system.
    rc_Tracker *tracker{ nullptr };  /// variable used if device and host are in the same memory system.

    /// function that takes a pointer to a SINGLE replay output object
    void(*on_track_output)(const replay_output *output, const rc_Data *data) { nullptr };

    replay_output();
    size_t get_buffer_size() const;
    /// if enable output of features, a maximum of max_save_features is saved and loaded.
    bool get(void **data, size_t *size) const;
    bool set(void *data, size_t size);
    void set_pose_output(const replay_output &other);
    output_mode get_output_type() const { return pose_feature_output::output_type; }
    void set_output_type(output_mode type);

    rc_Timestamp sensor_time_us{ 0 };
    rc_SensorType sensor_type{ rc_SENSOR_TYPE_GYROSCOPE };
    rc_Sensor sensor_id{ 0 };
    float path_length{ 0 };
    rc_DataPath data_path{ rc_DATA_PATH_SLOW };
    int8_t confidence{ 0 };
    void print_pose(uint8_t data_path) const;
    class bstream_buffer {
    public:
        bstream_buffer(size_t pos_, size_t len_, void *data) : position(pos_), length(len_), content(data) {};
        bstream_buffer(size_t pos_, size_t len_, std::unique_ptr<void, decltype(&free)> &data) : position(pos_), length(len_),
            unique_content(&data) {
            content = unique_content->get();
        };

        size_t copy_to(void *buffer, size_t length_) {
            size_t copy_size = std::min(length_, length - position);
            memcpy(buffer, (char *)content + position, copy_size);
            position += copy_size;
            return copy_size;
        }

        void copy_from(const void *buffer, size_t length_) {
            if (length_ > length - position)
                resize((size_t)(std::max(position + length_ * 1.2, length * 1.2)));
            memcpy((char *)content + position, buffer, length_);
            position += length_;
        }

        size_t get_length() { return length; }
    private:
        size_t position;
        size_t length;
        void *content{ nullptr };
        std::unique_ptr<void, decltype(&free)> *unique_content{ nullptr };
        bstream_buffer() = delete;
        void resize(size_t new_length) {
            length = new_length;
            content = realloc(content, new_length);
            if (unique_content) {
                unique_content->release();
                *unique_content = { content, free };
            }
        }
    };

private:
    mutable size_t buffer_size{ 0 }; /// size of buffer to hold persistence data
    mutable std::unique_ptr<void, decltype(&free)> write_buffer{ nullptr, free }; /// buffer used for writing out to memory
    std::unique_ptr<char, decltype(&free)> packet_header; /// dummy header to reserve space in streaming packet

    bool set_data(bstream_buffer &&pos);
    size_t get_data(bstream_buffer &&pos) const;
    size_t get_data(bstream_buffer &pos) const { return get_data(std::move(pos)); }
    replay_output& operator=(const replay_output &other) = delete;
};
