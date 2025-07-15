#include "stexecutor.hpp"
#include "la_theory.hpp"
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

    void executor::delay(riddle::arith_expr tp, const utils::rational &d) { get_linear_arithmetic_theory().new_lt(static_cast<riddle::arith_item &>(*tp).get_lin(), utils::lin(arith_value(*tp).get_rational() + d)); }
} // namespace ratio::executor
