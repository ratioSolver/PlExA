#include "plexa.hpp"
#include "graph.hpp"
#include "logging.hpp"
#include <queue>
#include <cassert>

namespace ratio::executor
{
    plexa::plexa(const utils::rational &units_per_tick) : units_per_tick(units_per_tick) {}

    void plexa::start()
    {
        running = true;
        executor_state_changed(state = executor_state::Executing);
    }

    void plexa::pause()
    {
        running = false;
        executor_state_changed(state = executor_state::Idle);
    }

    void plexa::tick(ratio::graph &gr)
    {
        const std::lock_guard<std::mutex> lock(mtx);
        if (pending_requirements)
        {
            executor_state_changed(state = running ? executor_state::Adapting : executor_state::Reasoning);
            try
            { // we solve the problem..
                gr.solve();
            }
            catch (const std::exception &e)
            { // adaptation failed..
                executor_state_changed(state = executor_state::Failed);
                return;
            }
            pending_requirements = false;
        }

        if (!running)
            return; // if not running, do nothing..

        while (!pulses.empty() && pulses.cbegin()->first <= current_time)
        {
            if (!pulses.cbegin()->second.first.empty())
            { // we have some atoms to start..
                std::vector<std::reference_wrapper<riddle::atom_term>> atms;
                for (const auto &atm : pulses.cbegin()->second.first)
                    atms.emplace_back(*atm);
                starting(atms);
            }
            if (!pulses.cbegin()->second.second.empty())
            { // we have some atoms to end..
                std::vector<std::reference_wrapper<riddle::atom_term>> atms;
                for (const auto &atm : pulses.cbegin()->second.second)
                    atms.emplace_back(*atm);
                ending(atms);
            }
        }
    }

    void plexa::build_timelines(const riddle::core &cr)
    {
        LOG_DEBUG("Building timelines");
        const std::lock_guard<std::mutex> lock(mtx);
        pulses.clear();
        // we collect all the active executable atoms..
        for (const auto &pred : executable_predicates)
            for (const auto &atm : pred->get_atoms())
                if (atm->get_state() == riddle::atom_state::active)
                { // the atom is active..
                    assert(cr.get_predicate(riddle::interval_kw).is_assignable_from(*pred) || cr.get_predicate(riddle::impulse_kw).is_assignable_from(*pred));
                    if (cr.get_predicate(riddle::impulse_kw).is_assignable_from(*pred))
                    {
                        auto at = cr.arith_value(static_cast<riddle::arith_term &>(*atm->get("at")));
                        if (at < current_time)
                            continue; // this atom is already in the past..
                        pulses[at].first.emplace(atm.get());
                        pulses[at].second.emplace(atm.get());
                    }
                    else
                    {
                        auto end = cr.arith_value(static_cast<riddle::arith_term &>(*atm->get("end")));
                        if (end < current_time)
                            continue; // this atom is already in the past..
                        pulses[end].second.emplace(atm.get());
                        auto start = cr.arith_value(static_cast<riddle::arith_term &>(*atm->get("start")));
                        if (start >= current_time)
                            pulses[start].first.emplace(atm.get());
                    }
                }
    }

    void plexa::reset_executable_predicates(const riddle::core &cr)
    {
        LOG_DEBUG("Resetting executable predicates");
        const std::lock_guard<std::mutex> lock(mtx);
        executable_predicates.clear();
        for (const auto &[_, pred] : cr.get_predicates())
            if (cr.get_predicate(riddle::interval_kw).is_assignable_from(*pred) || cr.get_predicate(riddle::impulse_kw).is_assignable_from(*pred))
                executable_predicates.emplace(pred.get());
        std::queue<riddle::component_type *> q;
        for (const auto &[_, tp] : cr.get_types())
            if (auto *st_tp = dynamic_cast<riddle::component_type *>(tp.get()))
                q.push(st_tp);
        while (!q.empty())
        {
            auto *tp = q.front();
            q.pop();
            for (const auto &[_, pred] : tp->get_predicates())
                if (cr.get_predicate(riddle::interval_kw).is_assignable_from(*pred) || cr.get_predicate(riddle::impulse_kw).is_assignable_from(*pred))
                    executable_predicates.emplace(pred.get());
            for (const auto &[_, sub_tp] : tp->get_types())
                if (auto *st_sub_tp = dynamic_cast<riddle::component_type *>(sub_tp.get()))
                    q.push(st_sub_tp);
        }
    }
} // namespace ratio::executor