/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2017-2018 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#include <queue>
#include <unordered_set>
#include <unordered_map>
#include "gtest/gtest.h"
#include "bstream.h"

using namespace std;

static void save_map_callback(void *handle, const void *buffer, size_t length)
{
    ostringstream * out = (ostringstream *)handle;
    out->write((const char *)buffer, length);
}

static size_t load_map_callback(void *handle, void *buffer, size_t length)
{
    istringstream * out = (istringstream *)handle;
    out->read((char *)buffer, length);
    return (*out) ? length : out->gcount();
}

/// element for use with standard containers, covering basic types/values
class basic_type {
public:
    ///constructor generates member values based on an initial seeding number
    basic_type(int seed) : ui8_m(seed + 1), ui16_m(seed + 2), ui32_m(seed + 3),
        ui64_m(seed + 4), f_m((float)seed + 5.f), inf_m(seed % 2 == 0 ? -INFINITY : INFINITY),
        nan_m(seed % 2 == 0 ? NAN : 0.1f), d_m(seed + 6.0), b_m(seed % 2 == 0) {};

    basic_type() = default;

    bool operator==(const basic_type &other) const {
        return (ui8_m == other.ui8_m) && (ui16_m == other.ui16_m) && (ui32_m == other.ui32_m) &&
            (ui64_m == other.ui64_m) && (f_m == other.f_m) && (inf_m == other.inf_m) && (b_m == other.b_m)
            && ((isnan(nan_m) && isnan(other.nan_m)) || (!isnan(nan_m) && !isnan(other.nan_m)));
    }
    bstream_writer &operator >> (bstream_writer &content) const {
        content << ui8_m << ui16_m << ui32_m << ui64_m;
        return content << f_m << inf_m << nan_m << d_m << b_m;
    }
    bstream_reader &operator << (bstream_reader &content) {
        content >> ui8_m >> ui16_m >> ui32_m >> ui64_m;
        return content >> f_m >> inf_m >> nan_m >> d_m >> b_m;
    }
private:
    uint8_t ui8_m{ 0 };
    uint16_t ui16_m{ 0 };
    uint32_t ui32_m{ 0 };
    uint64_t ui64_m{ 0 };
    float f_m{ 0 };
    float inf_m{ 0 };
    float nan_m{ 0 };
    double d_m{ 0 };
    bool b_m{ false };
};

static bstream_writer &operator << (bstream_writer &content, const basic_type &data) { return data >> content; }
static bstream_reader &operator >> (bstream_reader &content, basic_type &data) { return data << content; }

/// element for use with standard containers that use ordering of elements.
class unique_basic {
public:
    unique_basic(int seed) : id(class_id++), base(seed){};
    unique_basic() :id(class_id++), base() {};

    bool operator < (const unique_basic &other) const {
        return id < other.id;
    }
    bool operator==(const unique_basic &other) const {
        return (id == other.id) && (base == other.base);
    }
    bstream_writer &operator >> (bstream_writer &content) const { return content << id << base; }
    bstream_reader &operator << (bstream_reader &content) { return content >> id >> base; }
    uint32_t get_id() const { return id; };
private:
    static uint32_t class_id;
    uint32_t id;
    basic_type base;
};

uint32_t unique_basic::class_id = 0;

namespace std
{
    /// define hash function for element of set.
    template<>
    struct hash<unique_basic> {
        size_t operator()(const unique_basic &ele) const {
            return ele.get_id();
        }
    };
}

static bstream_writer &operator << (bstream_writer &content, const unique_basic &data) { return data >> content; }
static bstream_reader &operator >> (bstream_reader &content, unique_basic &data) { return data << content; }

TEST(Bstream, Stream_Std_containers)
{
    const int num_ele = 10;
    int seed = 0;
    // populate test data
    array<basic_type, num_ele> save_array = { basic_type(seed+=2) }, load_array = { basic_type(seed+1) };
    vector<basic_type> save_vector(num_ele, basic_type(seed+=2)), load_vector(num_ele, basic_type(seed+1));
    deque<basic_type> save_deque(num_ele, basic_type(seed+=2)), load_deque(num_ele, basic_type(seed+1));
    list<basic_type> save_list(num_ele, basic_type(seed+=2)), load_list(num_ele, basic_type(seed+1));
    map<int64_t, basic_type> save_map, load_map;
    unordered_map<int64_t, basic_type> save_uo_map, load_uo_map;
    set<unique_basic> save_set, load_set;
    unordered_set<unique_basic> save_uo_set, load_uo_set;
    for (uint64_t i = 0; i < num_ele; i++) {
        save_map.emplace(i, basic_type(seed++));
        save_uo_map.emplace(i, basic_type(seed++));
        save_set.insert(unique_basic(seed++));
        save_uo_set.insert(unique_basic(seed++));
    }
    string stream_content;

    // write succeeds
    {
        ostringstream write_ss;
        bstream_writer write_stream(save_map_callback, &write_ss);
        write_stream << save_array;
        write_stream.end_stream();
        EXPECT_TRUE(write_stream.good());
        stream_content = write_ss.str();
    }

    // read succeeds
    {
        array<basic_type, num_ele> load_array2;
        istringstream read_ss(stream_content, ios_base::in);
        bstream_reader read_stream(load_map_callback, &read_ss);
        read_stream >> load_array2;
        EXPECT_TRUE(read_stream.good());
    }

    // reading from incomplete content should fail
    {
        ostringstream write_ss;
        bstream_writer write_stream(save_map_callback, &write_ss);
        write_stream << save_array; // no end_stream is called, writing is not complete

        array<basic_type, num_ele> load_array2;
        istringstream read_ss(write_ss.str(), ios_base::in);
        bstream_reader read_stream(load_map_callback, &read_ss);
        read_stream >> load_array2;
        EXPECT_FALSE(read_stream.good());
    }

    // reading in wrong order of data items should fail
    {
        ostringstream write_ss;
        bstream_writer write_stream(save_map_callback, &write_ss);
        write_stream << save_array << save_map;
        write_stream.end_stream();

        array<basic_type, num_ele> load_array2 = load_array;
        map<int64_t, basic_type> load_map2 = load_map;
        istringstream read_ss(write_ss.str(), ios_base::in);
        bstream_reader read_stream(load_map_callback, &read_ss);
        read_stream >> load_map2 >> load_array2;
        EXPECT_NE(save_array, load_array2);
        EXPECT_NE(save_map, load_map2);
    }

    //test saving and loading of many container types.
    ostringstream write_ss;
    bstream_writer write_stream(save_map_callback, &write_ss);
    write_stream << save_array << save_vector << save_deque << save_list << save_map << save_uo_map;
    write_stream << save_set << save_uo_set;
    write_stream.end_stream();
    EXPECT_TRUE(write_stream.good());

    istringstream read_ss(write_ss.str(), ios_base::in);
    bstream_reader read_stream(load_map_callback, &read_ss);
    read_stream >> load_array >> load_vector >> load_deque >> load_list >> load_map >> load_uo_map;
    read_stream >> load_set >> load_uo_set;
    EXPECT_TRUE(read_stream.good());
    
    EXPECT_EQ(save_array, load_array);
    EXPECT_EQ(save_vector, load_vector);
    EXPECT_EQ(save_deque, load_deque);
    EXPECT_EQ(save_list, load_list);
    EXPECT_EQ(save_map, load_map);
    EXPECT_EQ(save_uo_map, load_uo_map);
    EXPECT_EQ(save_set, load_set);
    EXPECT_EQ(save_uo_set, load_uo_set);
}