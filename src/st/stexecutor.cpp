#include "stexecutor.hpp"
#include "la_theory.hpp"
#include "logging.hpp"
#include <cassert>

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
            assert(atm->get_state() == riddle::atom_state::active);
            auto tp = std::dynamic_pointer_cast<riddle::arith_term>(atm->get_core().get_predicate(riddle::impulse_kw).is_assignable_from(atm->get_type()) ? atm->get(riddle::at_kw) : atm->get(riddle::start_kw));
            get_linear_arithmetic_theory().new_lt(static_cast<riddle::arith_item &>(*tp).get_lin(), utils::lin(arith_value(*tp).get_rational() + delay), static_cast<riddle::atom &>(*atm).get_sigma());
        }
    }

    void executor::dont_end_yet(const std::unordered_map<riddle::atom_term *, utils::rational> &atoms)
    {
        for (const auto &[atm, delay] : atoms)
        {
            assert(atm->get_state() == riddle::atom_state::active);
            auto tp = std::dynamic_pointer_cast<riddle::arith_term>(atm->get(riddle::end_kw));
            get_linear_arithmetic_theory().new_lt(static_cast<riddle::arith_item &>(*tp).get_lin(), utils::lin(arith_value(*tp).get_rational() + delay), static_cast<riddle::atom &>(*atm).get_sigma());
        }
    }

    void executor::freeze_start(const std::vector<std::reference_wrapper<riddle::atom_term>> &atoms)
    {
        for (const auto &atm : atoms)
        {
            assert(atm.get().get_state() == riddle::atom_state::active);
            for (auto &[name, par] : atm.get().get_items())
                if (name != riddle::duration_kw && name != riddle::end_kw)
                {
                    if (auto b_par = dynamic_cast<riddle::bool_item *>(par.get()))
                    {
                        auto b_val = bool_value(*b_par);
                    }
                    else if (auto a_par = dynamic_cast<riddle::arith_item *>(par.get()))
                    {
                        auto a_val = arith_value(*a_par);
                    }
                    else if (auto s_par = dynamic_cast<riddle::string_item *>(par.get()))
                    {
                        auto s_val = string_value(*s_par);
                    }
                    else if (auto e_par = dynamic_cast<riddle::enum_item *>(par.get()))
                    {
                        auto e_val = enum_value(*e_par);
                    }
                }
        }
    }
    void executor::freeze_end(const std::vector<std::reference_wrapper<riddle::atom_term>> &atoms)
    {
        for (const auto &atm : atoms)
        {
            assert(atm.get().get_state() == riddle::atom_state::active);
            if (get_type(riddle::interval_kw).is_assignable_from(atm.get().get_type()))
            {
                auto a_par = std::dynamic_pointer_cast<riddle::arith_term>(atm.get().get(riddle::end_kw));
                auto a_val = arith_value(*a_par);
            }
        }
    }
} // namespace ratio::executor
