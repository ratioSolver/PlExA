#include "plexa.hpp"

namespace ratio::executor
{
    plexa::plexa(const utils::rational &units_per_tick) : units_per_tick(units_per_tick) {}

    void plexa::start()
    {
        running = true;
        executor_state_changed(state = executor_state::Executing);
    }

    void plexa::pause()
    {
        running = false;
        executor_state_changed(state = executor_state::Idle);
    }
} // namespace ratio::executor