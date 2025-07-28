#include "stexecutor.hpp"
#include "la_theory.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio::executor
{
    executor::executor(std::string_view name, const utils::rational &units_per_tick) noexcept : solver(name), theory(static_cast<smt::semitone &>(*this)), plexa(units_per_tick) {}

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
            auto v = variable(static_cast<riddle::atom &>(atm.get()).get_sigma());
            bind(v); // we bind the sigma variable to the theory..
            for (auto &[name, par] : atm.get().get_items())
                if (name != riddle::duration_kw && name != riddle::end_kw)
                {
                    if (auto b_par = dynamic_cast<riddle::bool_item *>(par.get()))
                        adaptations[v].emplace_back(std::make_unique<bool_adaptation>(static_cast<riddle::atom &>(atm.get()), *b_par, bool_value(*b_par)));
                    else if (auto a_par = dynamic_cast<riddle::arith_item *>(par.get()))
                    {
                        auto a_val = arith_value(*a_par);
                        adaptations[v].emplace_back(std::make_unique<lb_adaption>(static_cast<riddle::atom &>(atm.get()), *a_par, a_val));
                        adaptations[v].emplace_back(std::make_unique<ub_adaption>(static_cast<riddle::atom &>(atm.get()), *a_par, a_val));
                    }
                    else if (auto e_par = dynamic_cast<riddle::enum_item *>(par.get()))
                    {
                        auto e_val = enum_value(*e_par);
                        assert(!e_val.empty());
                        assert(e_val.size() == 1);
                        adaptations[v].emplace_back(std::make_unique<var_adaptation>(static_cast<riddle::atom &>(atm.get()), *e_par, e_val.front().get()));
                    }
                }
        }
    }
    void executor::freeze_end(const std::vector<std::reference_wrapper<riddle::atom_term>> &atoms)
    {
        for (const auto &atm : atoms)
        {
            assert(atm.get().get_state() == riddle::atom_state::active);
            auto v = variable(static_cast<riddle::atom &>(atm.get()).get_sigma());
            bind(v); // we bind the sigma variable to the theory..
            if (get_type(riddle::interval_kw).is_assignable_from(atm.get().get_type()))
            {
                auto a_par = static_cast<riddle::arith_item *>(atm.get().get(riddle::end_kw).get());
                auto a_val = arith_value(*a_par);
                adaptations[v].emplace_back(std::make_unique<lb_adaption>(static_cast<riddle::atom &>(atm.get()), *a_par, a_val));
                adaptations[v].emplace_back(std::make_unique<ub_adaption>(static_cast<riddle::atom &>(atm.get()), *a_par, a_val));
            }
        }
    }

    bool executor::propagate(const utils::lit &p) noexcept
    {
        if (value(p) == utils::True)
            for (const auto &adapt : adaptations[variable(p)])
            {
                if (auto ba = dynamic_cast<bool_adaptation *>(adapt.get()))
                {
                    if (bool_value(ba->bi) != ba->val) // if the value of the boolean item is not the same as the value of the adaptation we record the adaptation
                        smt::theory::record({!ba->atm.get_sigma(), ba->val == utils::True ? ba->bi.get_lit() : !ba->bi.get_lit()});
                }
                else if (auto la = dynamic_cast<lb_adaption *>(adapt.get())) // if the adaptation is a lower bound adaptation we record the adaptation
                    add_le(utils::lin(la->lb.get_rational()), la->ai.get_lin(), la->atm.get_sigma());
                else if (auto ua = dynamic_cast<ub_adaption *>(adapt.get())) // if the adaptation is an upper bound adaptation we record the adaptation
                    add_ge(utils::lin(ua->ub.get_rational()), ua->ai.get_lin(), ua->atm.get_sigma());
                else if (auto va = dynamic_cast<var_adaptation *>(adapt.get()))
                { // if the adaptation is a variable adaptation we record the adaptation
                    for (const auto &val : va->ei.get_values())
                        if (&val.get() != &va->val && value(va->ei.get_lit(val.get())) != utils::False)
                            smt::theory::record({!va->atm.get_sigma(), !va->ei.get_lit(val.get())});
                }
            }
        return true;
    }
    bool executor::check() noexcept { return true; }
    void executor::push() noexcept {}
    void executor::pop() noexcept {}
} // namespace ratio::executor
