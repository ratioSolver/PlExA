#include "stexecutor.hpp"
#include "logging.hpp"

namespace ratio::executor
{
    executor::executor(std::string_view name, const utils::rational &units_per_tick) noexcept : ratio::solver(name), plexa(units_per_tick) {}

    void executor::adapt(const std::string &script)
    {
        const std::lock_guard<std::mutex> lock(mtx);
        while (decision_level() > 0) // we go at root level..
            smt::semitone::pop();
        read(script);                       // we read the script..
        reset_executable_predicates(*this); // reset the executable predicates..
        pending_requirements = true;        // we have pending requirements to be solved..
    }

    void executor::failure(const std::unordered_set<const riddle::atom_term *> &atoms)
    {
    }

    void executor::adapt() { solve(); }
} // namespace ratio::executor
