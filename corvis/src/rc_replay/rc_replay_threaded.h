#pragma once
//  Copyright (c) 2015 RealityCap. All rights reserved.
#include <pthread.h>
#include <thread>
#include <atomic>
#include <chrono>
#include "rc_replay.h"

namespace rc {

class replay_threaded : public replay
{
    pthread_t thread;
    std::atomic<bool> done {true};
    std::chrono::steady_clock::duration initial_offset {0};
    bool sleep_until(uint64_t time_us) override {
        auto current_offset = std::chrono::steady_clock::now() - std::chrono::steady_clock::time_point(std::chrono::microseconds(time_us));
        if (initial_offset == std::chrono::seconds(0))
            initial_offset = current_offset;
        auto delta = initial_offset - current_offset;
        if (delta > std::chrono::seconds(1))
            initial_offset = current_offset;
        else if (delta > std::chrono::seconds(0))
            std::this_thread::sleep_for(delta);
        return !done;
    };
public:
    bool open(const char *file) {
        if (!done)
            close();
        return replay::open(file) && 0 == pthread_create(&thread, NULL, [](void*arg) {
                ((replay_threaded*)arg)->done = false;
                return (void*)((replay_threaded*)arg)->run();
            }, (void*)this);
    }
    bool close() {
        if (done)
            return false;
        done = true;
        void *result;
        return 0 == pthread_join(thread, &result) && (bool)result;
    }
};

}
