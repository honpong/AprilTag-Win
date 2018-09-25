//
//  sensor_fusion_queue.cpp
//
//  Created by Eagle Jones on 1/6/15.
//  Copyright (c) 2015 Realitycap. All rights reserved.
//

#include "rc_tracker.h"
#include "sensor_fusion_queue.h"
#include <cassert>
#include <memory>
#include "Trace.h"

template <typename Stream, int size>
inline Stream &operator <<(Stream &s, const sorted_ring_buffer<sensor_data, size> &q) {
    for(auto &d: q)
        s << std::chrono::duration_cast<std::chrono::microseconds>(d.timestamp-q.front().time_us).count() << ":" << std::hex << d.id*16+d.type << std::dec << " ";
    return s;
}

template<typename T, int N>
class sorted_ring_iterator : std::iterator<std::forward_iterator_tag, T>
{
public:
    sorted_ring_iterator(sorted_ring_buffer<T, N> &_buf, uint64_t _index): buf(_buf), index(_index){}
    
    T &operator*() const
    {
        return buf[index];
    }
    
    T *operator->() const
    {
        return &(buf[index]);
    }
    
    bool operator!=(const sorted_ring_iterator<T, N> &other) const
    {
        return index != other.index;
    }
    
    sorted_ring_iterator<T, N> operator++()
    {
        ++index;
        return *this;
    }
    
private:
    sorted_ring_buffer<T, N> &buf;
    uint64_t index;
};

fusion_queue::fusion_queue(const std::function<void(sensor_data &&)> data_func,
                           latency_strategy s,
                           sensor_clock::duration maximum_latency):
                strategy(s),
                data_receiver(data_func),
                control_func(nullptr),
                active(false),
                singlethreaded(false),
                max_latency(maximum_latency)
{
}

void fusion_queue::require_sensor(rc_SensorType type, rc_Sensor id, sensor_clock::duration max_latency)
{
    uint64_t global_id = sensor_data::get_global_id_by_type_id(type, id);
    required_sensors.push_back(global_id);
    stats.emplace(global_id, sensor_stats{max_latency});
}

void fusion_queue::reset()
{
    stop_immediately();
    wait_until_finished();

    thread = std::thread();
    clear();

    control_func = nullptr;
    active = false;
    last_dispatched = sensor_clock::time_point();
    buffer_time = {};
    buffering = false;
}

fusion_queue::~fusion_queue()
{
    stop_immediately();
    wait_until_finished();
}

std::string id_string(uint64_t global_id)
{
    auto id = sensor_data::get_id_by_global_id(global_id);
    auto type = sensor_data::get_type_by_global_id(global_id);
    std::string type_string = "UNKNOWN";
    switch(type) {
        case rc_SENSOR_TYPE_IMAGE: type_string = "Camera"; break;
        case rc_SENSOR_TYPE_DEPTH: type_string = "Depth"; break;
        case rc_SENSOR_TYPE_STEREO: type_string = "Stereo"; break;
        case rc_SENSOR_TYPE_ACCELEROMETER: type_string = "Accel"; break;
        case rc_SENSOR_TYPE_GYROSCOPE: type_string = "Gyro"; break;
        case rc_SENSOR_TYPE_THERMOMETER: type_string = "Temp"; break;
        case rc_SENSOR_TYPE_DEBUG: type_string = "Debug"; break;
        case rc_SENSOR_TYPE_VELOCIMETER: type_string = "Velo"; break;
    }
    return type_string + std::to_string(id);
}

std::string fusion_queue::get_stats()
{
    std::ostringstream statstr;

    std::vector<uint64_t> keys;
    for(auto kv : stats) {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    for(auto k : keys) {
        statstr << id_string(k) << "\t" + stats.find(k)->second.to_string() << "\n";
    }
    
    statstr << "Catchup time(us): " << catchup_stats << "\n\n";
    statstr << "Fast path time since catchup(us): " << time_since_catchup_stats << "\n\n";

    statstr << "Queue latency: " << queue_latency << "\n\n";

    return statstr.str();
}

void fusion_queue::receive_sensor_data(sensor_data && x)
{
    uint64_t id = x.global_id();
    push_queue(id, std::move(x));
    if(singlethreaded || buffering) dispatch_singlethread(false);
    else cond.notify_one();
}

void fusion_queue::dispatch_async(std::function<void()> fn)
{
    std::unique_lock<std::mutex> lock(control_mutex);
    control_func = fn;
    lock.unlock();
}

void fusion_queue::start_buffering(sensor_clock::duration _buffer_time)
{
    std::unique_lock<std::mutex> lock(control_mutex);
    active = true;
    buffer_time = _buffer_time;
    buffering = true;
    lock.unlock();
}

void fusion_queue::start(bool threaded)
{
    singlethreaded = !threaded;
    if(threaded) {
        if(!thread.joinable()) {
            std::unique_lock<std::mutex> lock(control_mutex);
            thread = std::thread(&fusion_queue::runloop, this);
            while(!active) {
                cond.wait(lock);
            }
        }
    }
    else {
        active = true;
        buffer_time = {};
        dispatch_singlethread(false); //dispatch any waiting data in case we were buffering
   }
}

void fusion_queue::stop_immediately()
{
    std::unique_lock<std::mutex> lock(control_mutex);
    active = false;
    lock.unlock();
    cond.notify_one();
}

void fusion_queue::stop_async()
{
    if(singlethreaded)
    {
        //flush any waiting data
        dispatch_singlethread(true);
#ifdef DEBUG
        //std::cerr << gyro_queue->hist;
#endif
    }
    stop_immediately();
}

void fusion_queue::stop()
{
    stop_async();
    if(!singlethreaded) wait_until_finished();
}

void fusion_queue::wait_until_finished()
{
    if(thread.joinable()) thread.join();
}

bool fusion_queue::run_control()
{
    if(!control_func) return false;
    control_func();
    control_func = nullptr;
    return true;
}

void fusion_queue::clear()
{
    std::lock_guard<std::mutex> data_guard(data_mutex);
    required_sensors.clear();
    queue.clear();
    stats.clear();
    total_in = 0;
    total_out = 0;

    singlethreaded = false;

    queue_latency = {};
    newest_received = {};
    catchup_stats            = stdev<1>();
    time_since_catchup_stats = stdev<1>();
}

void fusion_queue::runloop()
{
    std::unique_lock<std::mutex> lock(control_mutex);
    active = true;
    buffering = false;
    buffer_time = {};
    //If we were launched synchronously, wake up the launcher
    lock.unlock();
    cond.notify_one();
    lock.lock();
    while(active)
    {
        while(active && !run_control() && !dispatch_next(lock, false))
        {
            cond.wait(lock);
        }
        while(run_control() || dispatch_next(lock, false)); //we need to be greedy, since we only get woken on new data arriving
    }
    //flush any remaining data
    while (dispatch_next(lock, true));
#ifdef DEBUG
    std::cerr << get_stats();
#endif
    lock.unlock();
}

void fusion_queue::push_queue(uint64_t global_id, sensor_data && x)
{
    std::lock_guard<std::mutex> data_guard(data_mutex);
    total_in++;
    newest_received = std::max(x.timestamp, newest_received);

#ifdef __cpp_lib_unordered_map_try_emplace
    sensor_stats &s = stats.try_emplace(global_id, sensor_stats{std::chrono::microseconds(0)}).first->second;
#else
    auto i = stats.find(global_id);
    if (i == stats.end())
        i = stats.emplace_hint(i, global_id, sensor_stats{std::chrono::microseconds(0)});
    sensor_stats &s = i->second;
#endif

    s.receive(newest_received, x.timestamp);

    if (x.timestamp < last_dispatched) {
        TRACE_EVENT(SF_DROP_LATE, sensor_clock::tp_to_micros(last_dispatched));
        s.drop_late(newest_received);
        return;
    }
    if (x.timestamp < s.last_in) {
        TRACE_EVENT(SF_DROP_LATE, 1);
        s.drop_out_of_order();
        return;
    }
    if (queue.full()) {
        TRACE_EVENT(SF_DROP_LATE, 2);
        s.drop_full();
        return;
    }

    s.push();
    queue.push(std::move(x));
}

sensor_clock::time_point fusion_queue::next_timestamp()
{
    // the front of the queue has the max heap item (in all latency
    // strategies)
    return queue.front().timestamp;
}

sensor_data fusion_queue::pop_queue()
{
    sensor_data data = queue.pop();
    last_dispatched = data.timestamp;
    return data;
}

bool fusion_queue::ok_to_dispatch()
{
    sensor_clock::time_point next_time = next_timestamp();
    if(newest_received - next_time > max_latency) return true;
    for(const auto & id : required_sensors) {
        const auto stat = stats.find(id);
        if(stat == stats.end()) return false;
        auto & s = stat->second;
        switch(strategy) {
        case latency_strategy::MINIMIZE_LATENCY:
            if(s.expected(next_time) && !s.late_fixed_latency(newest_received))
                return false;
            break;

        case latency_strategy::DYNAMIC_LATENCY:
            if(s.expected(next_time) && !s.late_dynamic_latency(newest_received))
                return false;
            break;

        case latency_strategy::MINIMIZE_DROPS:
            // We are ok to dispatch unless we have seen at least one
            // of these, but none currently in the queue. This
            // eliminates drops, except for the first item of each
            // type, which we do not wait for to allow
            // autoconfiguration
            if(s.in && s.in_queue == 0)
                return false;
            break;
        }
    }
    return true;
}

uint64_t fusion_queue::data_in_queue(rc_SensorType type, rc_Sensor id)
{
    std::unique_lock<std::mutex> data_lock(data_mutex);

    const auto stat = stats.find(sensor_data::get_global_id_by_type_id(type, id));
    if(stat == stats.end()) return 0;

    return stat->second.in_queue;
}

bool fusion_queue::dispatch_next(std::unique_lock<std::mutex> &control_lock, bool force)
{
    std::unique_lock<std::mutex> data_lock(data_mutex);

    if(queue.empty()) return false;

    if(buffering) {
        sensor_clock::time_point next_time = next_timestamp();
        while(newest_received - next_time > buffer_time) {
            sensor_data dropped = pop_queue();
            stats.find(dropped.global_id())->second.drop_buffered();
            next_time = next_timestamp();
        }
        return false;
    }

    if(!force && !ok_to_dispatch()) return false;

    sensor_data data = pop_queue();
    TRACE_EVENT(SF_POP_QUEUE, data.time_us / 1000);
    
    data_lock.unlock();

    uint64_t id = data.global_id();
    stats.find(id)->second.dispatch();
    queue_latency.data({f_t((newest_received - data.timestamp).count())});

    control_lock.unlock();
    
    auto start = std::chrono::steady_clock::now();
    data_receiver(std::move(data));
    auto stop = std::chrono::steady_clock::now();
    stats.find(id)->second.measure.data(v<1>{ static_cast<f_t>(std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count()) });
    
    total_out++;
            
    control_lock.lock();

    return true;
}

void fusion_queue::dispatch_singlethread(bool force)
{
    std::unique_lock<std::mutex> lock(control_mutex);
    run_control();
    while(dispatch_next(lock, force)); //always be greedy - could have multiple pieces of data buffered
    lock.unlock();
}

uint64_t fusion_queue::size()
{
    std::unique_lock<std::mutex> lock(data_mutex);

    return queue.size();
}

void fusion_queue::dispatch_buffered(std::function<void(sensor_data &)> receive_func)
{
    std::unique_lock<std::mutex> lock(data_mutex);

    for (auto i = queue.begin(); i != queue.end(); ++i) {
        sensor_data d(*i, sensor_data::stack_copy());
        lock.unlock();
        receive_func(d);
        lock.lock();
        while (*i < d) ++i; // skip items push()ed during receive_func()
    }
}
