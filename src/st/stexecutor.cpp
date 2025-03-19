#include "stexecutor.hpp"

namespace ratio::executor
{
    stexecutor::stexecutor(const utils::rational &units_per_tick) noexcept : executor(units_per_tick) {}
} // namespace ratio::executor
