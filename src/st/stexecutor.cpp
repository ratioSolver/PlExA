#include "stexecutor.hpp"

namespace ratio::executor
{
    executor::executor(std::string_view name, const utils::rational &units_per_tick) noexcept : ratio::solver(name), plexa(units_per_tick) {}

    void executor::adapt(const std::string &script)
    {
    }

    void executor::failure(const std::unordered_set<const riddle::atom_term *> &atoms)
    {
    }
} // namespace ratio::executor
