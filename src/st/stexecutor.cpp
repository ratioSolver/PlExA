#include "stexecutor.hpp"

namespace ratio::executor
{
    executor::executor(const utils::rational &units_per_tick) noexcept : plexa(units_per_tick) {}
} // namespace ratio::executor
