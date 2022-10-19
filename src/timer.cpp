#include "timer.h"
#include <chrono>

namespace ratio::time
{
    timer::timer(const size_t &tick_dur, std::function<void(void)> f) : tick_duration(tick_dur), fun(f) {}

    void timer::start()
    {
        if (executing.load(std::memory_order_acquire))
            stop();
        executing.store(true, std::memory_order_release);
        tick_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(tick_duration);
        th = std::thread([this]()
                         {
            while (executing.load(std::memory_order_acquire)) {
                fun();
                std::this_thread::sleep_until(tick_time);
                tick_time += std::chrono::milliseconds(tick_duration);
            } });
    }

    void timer::stop()
    {
        executing.store(false, std::memory_order_release);
        if (th.joinable())
            th.join();
    }
} // namespace ratio::time