#include "executor.hpp"

namespace ratio::executor
{
    executor::executor(const utils::rational &units_per_tick) : units_per_tick(units_per_tick) {}

    void executor::start()
    {
        running = true;
        executor_state_changed(state = executor_state::Executing);
    }

    void executor::pause()
    {
        running = false;
        executor_state_changed(state = executor_state::Idle);
    }
} // namespace ratio::executor