#include "executor.h"
#include "executor_listener.h"
#include "item.h"
#include "atom_flaw.h"
#include <chrono>
#include <sstream>
#include <cassert>

namespace ratio::executor
{
    PLEXA_EXPORT executor::executor(ratio::solver &slv, const std::string &name, const utils::rational &units_per_tick) : core_listener(slv), solver_listener(slv), theory(slv.get_sat_core_ptr()), name(name), units_per_tick(units_per_tick), xi(slv.get_sat_core().new_var())
    {
        bind(variable(xi));
        build_timelines();
    }

    PLEXA_EXPORT void executor::start_execution()
    {
        running = true;
        state = executor_state::Executing;
        for (const auto &l : listeners)
            l->executor_state_changed(state);
    }

    PLEXA_EXPORT void executor::pause_execution()
    {
        running = false;
        state = executor_state::Idle;
        for (const auto &l : listeners)
            l->executor_state_changed(state);
    }

    PLEXA_EXPORT void executor::tick()
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        if (pending_requirements)
        { // we solve the problem again..
            slv.solve();
            pending_requirements = false;
        }

        if (!running)
            return;

        LOG("current time: " << to_string(current_time));

    manage_tick:
        while (!pulses.empty() && *pulses.cbegin() <= current_time)
        { // we have something to do..
            if (const auto starting_atms = s_atms.find(*pulses.cbegin()); starting_atms != s_atms.cend())
                // we notify that some atoms might be starting their execution..
                for (const auto &l : listeners)
                    l->starting(starting_atms->second);
            if (const auto ending_atms = e_atms.find(*pulses.cbegin()); ending_atms != e_atms.cend())
                // we notify that some atoms might be ending their execution..
                for (const auto &l : listeners)
                    l->ending(ending_atms->second);

            bool delays = false;
            if (const auto starting_atms = s_atms.find(*pulses.cbegin()); starting_atms != s_atms.cend())
                for (const auto &atm : starting_atms->second)
                    if (const auto at_atm = dont_start.find(atm); at_atm != dont_start.end())
                    { // this starting atom is not ready to be started..
                        auto &xpr = slv.is_impulse(*atm) ? atm->get(RATIO_AT) : atm->get(RATIO_START);
                        if (slv.is_constant(xpr))
                            throw execution_exception(); // we can't delay constants..
                        const auto lb = slv.arith_value(xpr) + (units_per_tick > at_atm->second ? units_per_tick : at_atm->second);
                        auto [it, added] = adaptations.at(atm).bounds.emplace(&*xpr, nullptr);
                        if (added)
                        { // we have to add new bounds..
                            const auto bnds = slv.arith_bounds(xpr);
                            it->second = new atom_adaptation::arith_bounds(lb, bnds.second);
                        }
                        else // we update the lower bound..
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).lb = lb;
                        if (xpr->get_type() == slv.get_real_type())
                        { // we have a real variable..
                            if (!slv.get_lra_theory().set_lb(slv.get_lra_theory().new_var(static_cast<ratio::arith_item &>(*xpr).get_lin()), lb, adaptations.at(atm).sigma_xi))
                            { // setting the lower bound caused a conflict..
                                swap_conflict(slv.get_lra_theory());
                                if (!backtrack_analyze_and_backjump())
                                    throw execution_exception();
                            }
                        }
                        else
                            throw std::runtime_error("not implemented yet");
                        delays = true;
                        dont_start.erase(at_atm);
                    }
            if (const auto ending_atms = e_atms.find(*pulses.cbegin()); ending_atms != e_atms.cend())
                for (const auto &atm : ending_atms->second)
                    if (const auto at_atm = dont_end.find(atm); at_atm != dont_end.end())
                    { // this ending atom is not ready to be ended..
                        auto &xpr = slv.is_impulse(*atm) ? atm->get(RATIO_AT) : atm->get(RATIO_END);
                        if (slv.is_constant(xpr))
                            throw execution_exception(); // we can't delay constants
                        const auto lb = slv.arith_value(xpr) + (units_per_tick > at_atm->second ? units_per_tick : at_atm->second);
                        auto [it, added] = adaptations.at(atm).bounds.emplace(&*xpr, nullptr);
                        if (added)
                        { // we have to add new bounds..
                            const auto bnds = slv.arith_bounds(xpr);
                            it->second = new atom_adaptation::arith_bounds(lb, bnds.second);
                        }
                        else // we update the lower bound..
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).lb = lb;
                        if (xpr->get_type() == slv.get_real_type())
                        { // we have a real variable..
                            if (!slv.get_lra_theory().set_lb(slv.get_lra_theory().new_var(static_cast<ratio::arith_item &>(*xpr).get_lin()), lb, adaptations.at(atm).sigma_xi))
                            { // setting the lower bound caused a conflict..
                                swap_conflict(slv.get_lra_theory());
                                if (!backtrack_analyze_and_backjump())
                                    throw execution_exception();
                            }
                        }
                        else
                            throw std::runtime_error("not implemented yet");
                        delays = true;
                        dont_end.erase(at_atm);
                    }

            if (delays)
            { // we have some delays: we propagate and remove new possible flaws..
                if (!slv.get_sat_core().propagate() || !slv.solve())
                    throw execution_exception();
                goto manage_tick;
            }

            if (const auto starting_atms = s_atms.find(*pulses.cbegin()); starting_atms != s_atms.cend())
            { // we have to freeze the starting atoms..
                for (auto &atm : starting_atms->second)
                    for (const auto &[xpr_name, xpr] : atm->get_vars()) // we freeze the starting atoms' expressions..
                        if (xpr_name != RATIO_AT && xpr_name != RATIO_DURATION && xpr_name != RATIO_END)
                        { // we store the value for propagating it in case of backtracking..
                            auto *itm = &*xpr;
                            if (const auto bi = dynamic_cast<const ratio::bool_item *>(itm))
                            { // we store the propositional value..
                                assert(slv.get_sat_core().value(bi->get_lit()) != utils::Undefined);
                                adaptations.at(atm).bounds.emplace(itm, new atom_adaptation::bool_bounds(slv.get_sat_core().value(bi->get_lit())));
                            }
                            else if (const auto ai = dynamic_cast<const ratio::arith_item *>(itm))
                            { // we store the arithmetic value and, if not a constant, we propagate also the bounds..
                                if (slv.is_constant(xpr))
                                    continue; // we have a constant: nothing to propagate..
                                if (&ai->get_type() == &slv.get_real_type())
                                { // we have a real variable..
                                    const auto val = slv.get_lra_theory().value(ai->get_lin());
                                    adaptations.at(atm).bounds.emplace(itm, new atom_adaptation::arith_bounds(val, val));
                                    // we freeze the arithmetic value..
                                    if (!slv.get_lra_theory().set(slv.get_lra_theory().new_var(ai->get_lin()), val, adaptations.at(atm).sigma_xi))
                                    { // freezing the arithmetic expression caused a conflict..
                                        swap_conflict(slv.get_lra_theory());
                                        if (!backtrack_analyze_and_backjump())
                                            throw execution_exception();
                                    }
                                }
                            }
                            else if (const auto vi = dynamic_cast<const ratio::enum_item *>(itm))
                            { // we store the variable value..
                                const auto vals = slv.get_ov_theory().value(vi->get_var());
                                assert(vals.size() == 1);
                                adaptations.at(atm).bounds.emplace(itm, new atom_adaptation::var_bounds(**vals.begin()));
                            }
                        }
                // we add the starting atoms to the set of atoms executing..
                executing.insert(starting_atms->second.cbegin(), starting_atms->second.cend());
                // we notify that some atoms are starting their execution..
                for (const auto &l : listeners)
                    l->start(starting_atms->second);
            }
            if (const auto ending_atms = e_atms.find(*pulses.cbegin()); ending_atms != e_atms.cend())
            { // we freeze the `at` and the `end` of the ending atoms..
                for (auto &atm : ending_atms->second)
                    if (slv.is_impulse(*atm))
                    { // we have an impulsive atom..
                        auto &at = atm->get(RATIO_AT);
                        if (slv.is_constant(at))
                            continue; // we have a constant: nothing to propagate..
                        const auto val = slv.arith_value(at);
                        auto [it, added] = adaptations.at(atm).bounds.emplace(&*at, nullptr);
                        if (added) // we have to add new bounds..
                            it->second = new atom_adaptation::arith_bounds(val, val);
                        else
                        { // we update the bounds..
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).lb = val;
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).ub = val;
                        }
                        if (at->get_type() == slv.get_real_type())
                        { // we have a real variable..
                            if (!slv.get_lra_theory().set(slv.get_lra_theory().new_var(static_cast<ratio::arith_item &>(*at).get_lin()), val, adaptations.at(atm).sigma_xi))
                            { // freezing the arithmetic expression caused a conflict..
                                swap_conflict(slv.get_lra_theory());
                                if (!backtrack_analyze_and_backjump())
                                    throw execution_exception();
                            }
                        }
                        else
                            throw std::runtime_error("not implemented yet");
                    }
                    else if (slv.is_interval(*atm))
                    { // we have an interval atom..
                        auto &end = atm->get(RATIO_END);
                        if (slv.is_constant(end))
                            continue; // we have a constant: nothing to propagate..
                        const auto val = slv.arith_value(end);
                        auto [it, added] = adaptations.at(atm).bounds.emplace(&*end, nullptr);
                        if (added) // we have to add new bounds..
                            it->second = new atom_adaptation::arith_bounds(val, val);
                        else
                        { // we update the bounds..
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).lb = val;
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).ub = val;
                        }
                        if (end->get_type() == slv.get_real_type())
                        { // we have a real variable..
                            if (!slv.get_lra_theory().set(slv.get_lra_theory().new_var(static_cast<ratio::arith_item &>(*end).get_lin()), val, adaptations.at(atm).sigma_xi))
                            { // freezing the arithmetic expression caused a conflict..
                                swap_conflict(slv.get_lra_theory());
                                if (!backtrack_analyze_and_backjump())
                                    throw execution_exception();
                            }
                        }
                        else
                            throw std::runtime_error("not implemented yet");
                    }
                // we remove the ending atoms from the set of atoms executing..
                for (const auto &atm : ending_atms->second)
                    executing.erase(atm);
                // we notify that some atoms are ending their execution..
                for (const auto &l : listeners)
                    l->end(ending_atms->second);
            }

            pulses.erase(pulses.cbegin());
        }

        if (slv.arith_value(slv.get("horizon")) <= current_time && dont_end.empty())
        { // we have reached the horizon..
            state = executor_state::Finished;
            // we notify that the execution has finished..
            for (const auto &l : listeners)
                l->executor_state_changed(state);
        }

        // we update the current time..
        current_time += units_per_tick;

        // we notify that a tick has arised..
        for (const auto &l : listeners)
            l->tick(current_time);
    }

    PLEXA_EXPORT void executor::adapt(const std::string &script)
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        while (!slv.get_sat_core().root_level()) // we go at root level..
            slv.get_sat_core().pop();
        slv.read(script);
        pending_requirements = true;
    }
    PLEXA_EXPORT void executor::adapt(const std::vector<std::string> &files)
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        while (!slv.get_sat_core().root_level()) // we go at root level..
            slv.get_sat_core().pop();
        slv.read(files);
        pending_requirements = true;
    }

    PLEXA_EXPORT void executor::failure(const std::unordered_set<const ratio::atom *> &atoms)
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        for (const auto &atm : atoms)
            cnfl.push_back(!atm->get_sigma());
        // we backtrack to a level at which we can analyze the conflict..
        if (!backtrack_analyze_and_backjump() || !slv.solve())
            throw execution_exception();
    }

    bool executor::propagate(const semitone::lit &p) noexcept
    {
        if (p == xi)
        { // we propagate the active bounds..
            for (const auto &adapt : adaptations)
                if (slv.get_sat_core().value(adapt.second.sigma_xi) == utils::True)
                    for (const auto &bnds : adapt.second.bounds)
                        if (!propagate_bounds(*bnds.first, *bnds.second, adapt.second.sigma_xi))
                            return false;
        }
        else if (slv.get_sat_core().value(variable(p)) == utils::True)
        { // an atom has been activated..
            const auto atm = all_atoms.at(variable(p));
            const auto &adapt = adaptations.at(atm);
            for (const auto &bnds : adapt.bounds)
                if (!propagate_bounds(*bnds.first, *bnds.second, p))
                    return false;
        }
        return true;
    }

    void executor::started_solving()
    {
        if (state != executor_state::Reasoning)
        {
            state = executor_state::Adapting;
            for (const auto &l : listeners)
                l->executor_state_changed(state);
        }
    }

    void executor::solution_found()
    {
        switch (slv.get_sat_core().value(xi))
        {
        case utils::False: // the plan can't be executed anymore..
            throw execution_exception();
        case utils::Undefined: // we enforce the xi variable..
            slv.take_decision(xi);
            break;
        }
        switch (slv.get_sat_core().value(xi))
        {
        case utils::False: // the plan can't be executed anymore..
            throw execution_exception();
        case utils::Undefined: // we attempt to solve the problem again..
            slv.solve();
            break;
        }
        build_timelines();

        state = running ? executor_state::Executing : executor_state::Idle;
        for (const auto &l : listeners)
            l->executor_state_changed(state);
    }
    void executor::inconsistent_problem()
    {
        s_atms.clear();
        e_atms.clear();
        pulses.clear();

        state = executor_state::Failed;
        for (const auto &l : listeners)
            l->executor_state_changed(state);
    }

    void executor::flaw_created(const ratio::flaw &f)
    {
        if (const auto af = dynamic_cast<const ratio::atom_flaw *>(&f))
        { // we create an adaptation for adapting the atom at execution time..
            auto &atm = static_cast<ratio::atom &>(*af->get_atom());
            // we create a new variable for propagating the execution constraints..
            const auto sigma_xi = slv.get_sat_core().new_var();
            // we bind the sigma variable for propagating the bounds..
            bind(sigma_xi);
            all_atoms.emplace(sigma_xi, &atm);
            // either the atom is not active, or the xi variable is false, or the execution bounds must be enforced..
            [[maybe_unused]] bool nc = slv.get_sat_core().new_clause({!atm.get_sigma(), !xi, semitone::lit(sigma_xi)});
            assert(nc);
            auto [at_adapt, added] = adaptations.emplace(&atm, semitone::lit(sigma_xi));

            if (slv.is_impulse(atm))
            { // we create a new adaptation for the impulse atom..
                auto &xpr = atm.get(RATIO_AT);
                at_adapt->second.bounds.emplace(&*xpr, new atom_adaptation::arith_bounds(utils::inf_rational(current_time), utils::inf_rational(utils::rational::POSITIVE_INFINITY)));
            }
            else if (slv.is_interval(atm))
            { // we create a new adaptation for the interval atom..
                auto &xpr = atm.get(RATIO_START);
                at_adapt->second.bounds.emplace(&*xpr, new atom_adaptation::arith_bounds(utils::inf_rational(current_time), utils::inf_rational(utils::rational::POSITIVE_INFINITY)));
            }
        }
    }

    void executor::build_timelines()
    {
        LOG("building timelines..");
        s_atms.clear();
        e_atms.clear();
        pulses.clear();

        // we collect all the active relevant atoms..
        for (const auto pred : relevant_predicates)
            for (const auto &atm : pred->get_instances())
            {
                auto &c_atm = static_cast<ratio::atom &>(*atm);
                if (slv.get_sat_core().value(c_atm.get_sigma()) == utils::True)
                { // the atom is active..
                    if (slv.is_impulse(c_atm))
                    {
                        auto at = slv.arith_value(c_atm.get(RATIO_AT));
                        if (at < current_time)
                            continue; // this atom is already in the past..
                        s_atms[at].insert(&c_atm);
                        e_atms[at].insert(&c_atm);
                        pulses.insert(at);
                    }
                    else if (slv.is_interval(c_atm))
                    {
                        auto end = slv.arith_value(c_atm.get(RATIO_END));
                        if (end < current_time)
                            continue; // this atom is already in the past..
                        auto start = slv.arith_value(c_atm.get(RATIO_START));
                        if (start >= current_time)
                        {
                            s_atms[start].insert(&c_atm);
                            pulses.insert(start);
                        }
                        e_atms[end].insert(&c_atm);
                        pulses.insert(end);
                    }
                }
            }
    }

    bool executor::propagate_bounds(const riddle::item &itm, const atom_adaptation::item_bounds &bounds, const semitone::lit &reason)
    {
        if (const auto ba = dynamic_cast<const atom_adaptation::bool_bounds *>(&bounds))
        {
            const auto var = static_cast<const ratio::bool_item *>(&itm)->get_lit();
            const auto val = slv.get_sat_core().value(var);
            if (val == utils::Undefined)
                record({var, !reason});
            else if (val != ba->val)
            { // we have a conflict..
                cnfl.push_back(var);
                cnfl.push_back(!reason);
                return false;
            }
        }
        else if (const auto aa = dynamic_cast<const atom_adaptation::arith_bounds *>(&bounds))
        {
            if (static_cast<const ratio::arith_item &>(itm).get_lin().vars.empty())
                return true; // we have a constant: nothing to propagate..
            const auto var = slv.get_lra_theory().new_var(static_cast<const ratio::arith_item &>(itm).get_lin());
            if (itm.get_type() == slv.get_real_type())
            { // we have a real variable..
                if (!slv.get_lra_theory().set_lb(var, aa->lb, reason) || !slv.get_lra_theory().set_ub(var, aa->ub, reason))
                { // setting the bounds caused a conflict..
                    swap_conflict(slv.get_lra_theory());
                    return false;
                }
            }
            else
                throw std::runtime_error("not implemented yet..");
        }
        else if (const auto va = dynamic_cast<const atom_adaptation::var_bounds *>(&bounds))
        {
            const auto var = static_cast<const ratio::enum_item &>(itm).get_var();
            const auto val = slv.get_ov_theory().value(var);
            if (val.size() > 1)
                record({slv.get_ov_theory().allows(var, va->val), !reason});
            else if (*val.begin() != &va->val)
            { // we have a conflict..
                cnfl.push_back(slv.get_ov_theory().allows(var, va->val));
                cnfl.push_back(!reason);
                return false;
            }
        }
        return true;
    }

    void executor::reset_relevant_predicates()
    {
        relevant_predicates.clear();
        for (const auto &pred : slv.get_predicates())
            if (slv.is_impulse(pred.get()) || slv.is_interval(pred.get()))
                relevant_predicates.insert(&pred.get());
        std::queue<riddle::complex_type *> q;
        for (const auto &tp : slv.get_types())
            if (!tp.get().is_primitive())
                if (auto ct = dynamic_cast<riddle::complex_type *>(&tp.get()))
                    q.push(ct);
        while (!q.empty())
        {
            for (const auto &st : q.front()->get_types())
                if (!st.get().is_primitive())
                    if (auto ct = dynamic_cast<riddle::complex_type *>(&st.get()))
                        q.push(ct);
            for (const auto &pred : q.front()->get_predicates())
                if (slv.is_impulse(pred.get()) || slv.is_interval(pred.get()))
                    relevant_predicates.insert(&pred.get());
            q.pop();
        }
    }
} // namespace ratio::executor
