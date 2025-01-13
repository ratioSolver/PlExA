#include "z3executor.hpp"

namespace ratio::executor
{
    z3executor::z3executor(std::string_view name, const utils::rational &units_per_tick) : z3solver(name), executor(units_per_tick) {}
} // namespace ratio::executor