#pragma once

#include <functional>
#include <atomic>
#include <thread>

namespace ratio::time
{
  class timer final
  {
  public:
    timer(const size_t &tick_dur, std::function<void(void)> f);

    /**
     * @brief Starts the timer.
     */
    void start();
    /**
     * @brief Stops the timer.
     */
    void stop();

  private:
    const size_t tick_duration; // the duration of each tick in milliseconds..
    std::function<void(void)> fun;
    std::chrono::steady_clock::time_point tick_time;
    std::atomic<bool> executing;
    std::thread th;
  };
} // namespace ratio::time
