#include "executor.hpp"
#include "executor_theory.hpp"
#include "logging.hpp"
#include <cassert>

namespace ratio::executor
{
    executor::executor(std::shared_ptr<ratio::solver> slv, const utils::rational &units_per_tick) noexcept : slv(slv), exec_theory(slv->get_sat().new_theory<executor_theory>(*this)), units_per_tick(units_per_tick), xi(slv->get_sat().new_var()) {}

    void executor::init()
    {
        exec_theory.init();
        slv->init();
    }

    void executor::adapt(const std::string &script)
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        while (!slv->get_sat().root_level()) // we go at root level..
            slv->get_sat().pop();
        slv->read(script);
        reset_relevant_predicates();
        pending_requirements = true;
    }
    void executor::adapt(const std::vector<std::string> &scripts)
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        while (!slv->get_sat().root_level()) // we go at root level..
            slv->get_sat().pop();
        slv->read(scripts);
        reset_relevant_predicates();
        pending_requirements = true;
    }

    void executor::start()
    {
        running = true;
        executor_state_changed(state = executor_state::Executing);
    }

    void executor::pause()
    {
        running = false;
        executor_state_changed(state = executor_state::Idle);
    }

    void executor::tick()
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        if (pending_requirements)
        { // we solve the problem..
            executor_state_changed(state = running ? executor_state::Adapting : executor_state::Reasoning);
            if (slv->solve())
            { // we have a solution..
                executor_state_changed(state = running ? executor_state::Executing : executor_state::Idle);
                build_timelines(); // we build the timelines..
            }
            else // we have no solution..
                executor_state_changed(state = executor_state::Failed);
            pending_requirements = false;
        }

        if (!running)
            return; // if not running, do nothing..

        LOG_DEBUG("[" + slv->get_name() + "] current time: " << to_string(current_time));
    manage_tick:
        while (!pulses.empty() && *pulses.cbegin() <= current_time)
        { // we have something to do..
            if (const auto starting_atms = s_atms.find(*pulses.cbegin()); starting_atms != s_atms.cend())
            {
                std::vector<std::reference_wrapper<ratio::atom>> atms;
                for (auto &atm : starting_atms->second)
                    atms.push_back(*atm);
                starting(atms); // we notify that some atoms might be starting their execution..
            }
            if (const auto ending_atms = e_atms.find(*pulses.cbegin()); ending_atms != e_atms.cend())
            {
                std::vector<std::reference_wrapper<ratio::atom>> atms;
                for (auto &atm : ending_atms->second)
                    atms.push_back(*atm);
                ending(atms); // we notify that some atoms might be ending their execution..
            }

            bool delays = false;
            if (const auto starting_atms = s_atms.find(*pulses.cbegin()); starting_atms != s_atms.cend())
                for (const auto &atm : starting_atms->second)
                    if (const auto at_atm = dont_start.find(atm); at_atm != dont_start.end())
                    { // this starting atom is not ready to be started..
                        auto &xpr = is_impulse(*atm) ? *atm->get("at") : *atm->get("start");
                        if (is_constant(static_cast<riddle::arith_item &>(xpr)))
                            throw execution_exception(); // we can't delay constants..
                        const auto lb = slv->arithmetic_value(static_cast<riddle::arith_item &>(xpr)) + (units_per_tick > at_atm->second ? units_per_tick : at_atm->second);
                        auto [it, added] = adaptations.at(atm).bounds.emplace(&xpr, nullptr);
                        if (added)
                        { // we have to add new bounds..
                            const auto bnds = slv->bounds(static_cast<riddle::arith_item &>(xpr));
                            it->second = std::make_unique<atom_adaptation::arith_bounds>(lb, bnds.second);
                        }
                        else // we update the lower bound..
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).lb = lb;

                        if (is_real(xpr))
                        { // we have a real variable..
                            auto var = static_cast<riddle::arith_item &>(xpr).get_value();
                            if (!slv->get_lra_theory().set_lb(slv->get_lra_theory().new_var(std::move(var)), lb, {adaptations.at(atm).sigma_xi}))
                                throw execution_exception(); // we have a conflict..
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
                        auto &xpr = is_impulse(*atm) ? *atm->get("at") : *atm->get("end");
                        if (is_constant(static_cast<riddle::arith_item &>(xpr)))
                            throw execution_exception(); // we can't delay constants
                        const auto lb = slv->arithmetic_value(static_cast<riddle::arith_item &>(xpr)) + (units_per_tick > at_atm->second ? units_per_tick : at_atm->second);
                        auto [it, added] = adaptations.at(atm).bounds.emplace(&xpr, nullptr);
                        if (added)
                        { // we have to add new bounds..
                            const auto bnds = slv->bounds(static_cast<riddle::arith_item &>(xpr));
                            it->second = std::make_unique<atom_adaptation::arith_bounds>(lb, bnds.second);
                        }
                        else // we update the lower bound..
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).lb = lb;
                        if (is_real(xpr))
                        { // we have a real variable..
                            auto lin = static_cast<riddle::arith_item &>(xpr).get_value();
                            if (!slv->get_lra_theory().set_lb(slv->get_lra_theory().new_var(std::move(lin)), lb, {adaptations.at(atm).sigma_xi}))
                                throw execution_exception(); // we have a conflict..
                        }
                        else
                            throw std::runtime_error("not implemented yet");
                        delays = true;
                        dont_end.erase(at_atm);
                    }

            if (delays)
            { // we have some delays: we propagate and remove new possible flaws..
                if (!slv->solve())
                    throw execution_exception();
                goto manage_tick;
            }

            if (const auto starting_atms = s_atms.find(*pulses.cbegin()); starting_atms != s_atms.cend())
            { // we have to freeze the starting atoms..
                for (auto &atm : starting_atms->second)
                    for (const auto &[xpr_name, xpr] : atm->get_items()) // we freeze the starting atoms' expressions..
                        if (xpr_name != "at" && xpr_name != "duration" && xpr_name != "end")
                        { // we store the value for propagating it in case of backtracking..
                            auto *itm = &*xpr;
                            if (const auto bi = dynamic_cast<const riddle::bool_item *>(itm))
                            { // we store the propositional value..
                                assert(slv->get_sat().value(bi->get_value()) != utils::Undefined);
                                adaptations.at(atm).bounds.emplace(itm, new atom_adaptation::bool_bounds(slv->get_sat().value(bi->get_value())));
                            }
                            else if (const auto ai = dynamic_cast<const riddle::arith_item *>(itm))
                            { // we store the arithmetic value and, if not a constant, we propagate also the bounds..
                                if (is_constant(*xpr))
                                    continue; // we have a constant: nothing to propagate..
                                if (&ai->get_type() == &slv->get_real_type())
                                { // we have a real variable..
                                    const auto val = slv->get_lra_theory().value(ai->get_value());
                                    adaptations.at(atm).bounds.emplace(itm, new atom_adaptation::arith_bounds(val, val));
                                    // we freeze the arithmetic value..
                                    auto lin = ai->get_value();
                                    if (!slv->get_lra_theory().set_value(slv->get_lra_theory().new_var(std::move(lin)), val, {adaptations.at(atm).sigma_xi})) // freezing the arithmetic expression caused a conflict..
                                        throw execution_exception();
                                }
                            }
                            else if (const auto vi = dynamic_cast<const riddle::enum_item *>(itm))
                            { // we store the variable value..
                                const auto vals = slv->get_ov_theory().domain(vi->get_value());
                                assert(vals.size() == 1);
                                adaptations.at(atm).bounds.emplace(itm, new atom_adaptation::var_bounds(vals.begin()->get()));
                            }
                        }
                // we add the starting atoms to the set of atoms executing..
                for (const auto &atm : starting_atms->second)
                    executing.insert(atm);
                // we notify that some atoms are starting their execution..
                std::vector<std::reference_wrapper<ratio::atom>> atms;
                for (auto &atm : starting_atms->second)
                    atms.push_back(*atm);
                start(atms);
            }
            if (const auto ending_atms = e_atms.find(*pulses.cbegin()); ending_atms != e_atms.cend())
            { // we freeze the `at` and the `end` of the ending atoms..
                for (auto &atm : ending_atms->second)
                    if (is_impulse(*atm))
                    { // we have an impulsive atom..
                        auto &at = *atm->get("at");
                        if (is_constant(at))
                            continue; // we have a constant: nothing to propagate..
                        const auto val = slv->arithmetic_value(static_cast<riddle::arith_item &>(at));
                        auto [it, added] = adaptations.at(atm).bounds.emplace(&at, nullptr);
                        if (added) // we have to add new bounds..
                            it->second = std::make_unique<atom_adaptation::arith_bounds>(val, val);
                        else
                        { // we update the bounds..
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).lb = val;
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).ub = val;
                        }
                        if (is_real(at))
                        { // we have a real variable..
                            auto lin = static_cast<riddle::arith_item &>(at).get_value();
                            if (!slv->get_lra_theory().set_value(slv->get_lra_theory().new_var(std::move(lin)), val, {adaptations.at(atm).sigma_xi})) // freezing the arithmetic expression caused a conflict..
                                throw execution_exception();
                        }
                        else
                            throw std::runtime_error("not implemented yet");
                    }
                    else if (is_interval(*atm))
                    { // we have an interval atom..
                        auto &end = *atm->get("end");
                        if (is_constant(end))
                            continue; // we have a constant: nothing to propagate..
                        const auto val = slv->arithmetic_value(static_cast<riddle::arith_item &>(end));
                        auto [it, added] = adaptations.at(atm).bounds.emplace(&end, nullptr);
                        if (added) // we have to add new bounds..
                            std::make_unique<atom_adaptation::arith_bounds>(val, val);
                        else
                        { // we update the bounds..
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).lb = val;
                            static_cast<atom_adaptation::arith_bounds &>(*it->second).ub = val;
                        }
                        if (is_real(end))
                        { // we have a real variable..
                            auto lin = static_cast<riddle::arith_item &>(end).get_value();
                            if (!slv->get_lra_theory().set_value(slv->get_lra_theory().new_var(std::move(lin)), val, {adaptations.at(atm).sigma_xi})) // freezing the arithmetic expression caused a conflict..
                                throw execution_exception();
                        }
                        else
                            throw std::runtime_error("not implemented yet");
                    }
                // we remove the ending atoms from the set of atoms executing..
                for (const auto &atm : ending_atms->second)
                    executing.erase(atm);
                // we notify that some atoms are ending their execution..
                std::vector<std::reference_wrapper<ratio::atom>> atms;
                for (auto &atm : ending_atms->second)
                    atms.push_back(*atm);
                end(atms);
            }

            pulses.erase(pulses.cbegin());
        }

        if (slv->arithmetic_value(*std::dynamic_pointer_cast<riddle::arith_item>(slv->core::get("horizon"))) <= current_time && dont_end.empty()) // we have reached the horizon..
            executor_state_changed(state = executor_state::Finished);

        // we update the current time..
        current_time += units_per_tick;
        tick(current_time);
    }

    void executor::failure(const std::unordered_set<const ratio::atom *> &atoms)
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        exec_theory.failure(atoms);
    }

    void executor::executor_state_changed([[maybe_unused]] executor_state state) { LOG_DEBUG("[" << slv->get_name() << "] executor is now " << state); }
    void executor::tick([[maybe_unused]] const utils::rational &time) { LOG_DEBUG("[" << slv->get_name() << "] current time is " << to_string(time)); }
    void executor::starting([[maybe_unused]] const std::vector<std::reference_wrapper<ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] starting " << atms.size() << " atoms"); }
    void executor::start([[maybe_unused]] const std::vector<std::reference_wrapper<ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] started " << atms.size() << " atoms"); }
    void executor::ending([[maybe_unused]] const std::vector<std::reference_wrapper<ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] ending " << atms.size() << " atoms"); }
    void executor::end([[maybe_unused]] const std::vector<std::reference_wrapper<ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] ended " << atms.size() << " atoms"); }

    void executor::build_timelines()
    {
        LOG_DEBUG("building timelines..");
        s_atms.clear();
        e_atms.clear();
        pulses.clear();

        std::vector<std::reference_wrapper<const ratio::atom>> cs_atms, ce_atms;
        // we collect all the active relevant atoms..
        for (const auto pred : relevant_predicates)
            for (const auto &atm : pred->get_atoms())
            {
                auto &c_atm = static_cast<ratio::atom &>(*atm);
                if (slv->get_sat().value(c_atm.get_sigma()) == utils::True)
                { // the atom is active..
                    if (is_impulse(c_atm))
                    {
                        auto at = slv->arithmetic_value(*std::dynamic_pointer_cast<riddle::arith_item>(c_atm.get("at")));
                        if (at < current_time)
                            continue; // this atom is already in the past..
                        s_atms[at].insert(&c_atm);
                        e_atms[at].insert(&c_atm);
                        pulses.insert(at);
                    }
                    else if (is_interval(c_atm))
                    {
                        auto end = slv->arithmetic_value(*std::dynamic_pointer_cast<riddle::arith_item>(c_atm.get("end")));
                        if (end < current_time)
                            continue; // this atom is already in the past..
                        auto start = slv->arithmetic_value(*std::dynamic_pointer_cast<riddle::arith_item>(c_atm.get("start")));
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

    void executor::reset_relevant_predicates()
    {
        relevant_predicates.clear();
        for (const auto &pred : slv->get_predicates())
            if (is_impulse(pred.get()) || is_interval(pred.get()))
                relevant_predicates.insert(&pred.get());
        std::queue<riddle::component_type *> q;
        for (const auto &tp : slv->get_types())
            if (!tp.get().is_primitive())
                if (auto ct = dynamic_cast<riddle::component_type *>(&tp.get()))
                    q.push(ct);
        while (!q.empty())
        {
            for (const auto &st : q.front()->get_types())
                if (!st.get().is_primitive())
                    if (auto ct = dynamic_cast<riddle::component_type *>(&st.get()))
                        q.push(ct);
            for (const auto &pred : q.front()->get_predicates())
                if (is_impulse(pred.get()) || is_interval(pred.get()))
                    relevant_predicates.insert(&pred.get());
            q.pop();
        }
    }
} // namespace ratio::executor