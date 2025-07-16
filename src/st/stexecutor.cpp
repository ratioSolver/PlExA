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

    void executor::dont_start_yet(const std::unordered_map<riddle::atom_term *, utils::rational> &atoms)
    {
        for (const auto &[atm, delay] : atoms)
        {
            auto tp = std::dynamic_pointer_cast<riddle::arith_term>(atm->get_core().get_predicate(riddle::impulse_kw).is_assignable_from(atm->get_type()) ? atm->get("at") : atm->get("start"));
            get_linear_arithmetic_theory().new_lt(static_cast<riddle::arith_item &>(*tp).get_lin(), utils::lin(arith_value(*tp).get_rational() + delay), static_cast<riddle::atom &>(*atm).get_sigma());
        }
    }

    void executor::dont_end_yet(const std::unordered_map<riddle::atom_term *, utils::rational> &atoms)
    {
        for (const auto &[atm, delay] : atoms)
        {
            auto tp = std::dynamic_pointer_cast<riddle::arith_term>(atm->get("end"));
            get_linear_arithmetic_theory().new_lt(static_cast<riddle::arith_item &>(*tp).get_lin(), utils::lin(arith_value(*tp).get_rational() + delay), static_cast<riddle::atom &>(*atm).get_sigma());
        }
    }
} // namespace ratio::executor
