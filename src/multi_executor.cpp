#include "multi_executor.h"
#include "multi_executor_listener.h"

namespace ratio::executor
{
    multi_executor::multi_executor() {}

    multi_exec &multi_executor::new_solver(const std::string &name)
    {
        auto slv = new solver();
        auto exec = new executor(*slv);
        auto m_exec = new multi_exec(*this, *slv, *exec, name);
        executors.emplace_back(m_exec);
        fire_solver_created(*m_exec);
        return *m_exec;
    }
    void multi_executor::destroy_solver(multi_exec &exec)
    {
        auto it = std::find_if(executors.begin(), executors.end(), [&exec](const exec_ptr &e)
                               { return &*e == &exec; });
        if (it != executors.end())
        {
            fire_solver_destroyed(exec);
            auto slv = &(*it)->slv;
            auto exec = &(*it)->exec;
            executors.erase(it);
            delete exec;
            delete slv;
        }
    }

    void multi_executor::fire_solver_created(const multi_exec &exec)
    {
        for (auto &l : listeners)
            l->solver_created(exec);
    }
    void multi_executor::fire_solver_destroyed(const multi_exec &exec)
    {
        for (auto &l : listeners)
            l->solver_destroyed(exec);
    }
    void multi_executor::fire_log(const multi_exec &exec, const std::string &msg)
    {
        for (auto &l : listeners)
            l->log(exec, msg);
    }
    void multi_executor::fire_read(const multi_exec &exec, const std::string &rddl)
    {
        for (auto &l : listeners)
            l->read(exec, rddl);
    }
    void multi_executor::fire_read(const multi_exec &exec, const std::vector<std::string> &files)
    {
        for (auto &l : listeners)
            l->read(exec, files);
    }
    void multi_executor::fire_state_changed(const multi_exec &exec)
    {
        for (auto &l : listeners)
            l->state_changed(exec);
    }
    void multi_executor::fire_started_solving(const multi_exec &exec)
    {
        for (auto &l : listeners)
            l->started_solving(exec);
    }
    void multi_executor::fire_solution_found(const multi_exec &exec)
    {
        for (auto &l : listeners)
            l->solution_found(exec);
    }
    void multi_executor::fire_inconsistent_problem(const multi_exec &exec)
    {
        for (auto &l : listeners)
            l->inconsistent_problem(exec);
    }
    void multi_executor::fire_flaw_created(const multi_exec &exec, const flaw &f)
    {
        for (auto &l : listeners)
            l->flaw_created(exec, f);
    }
    void multi_executor::fire_flaw_state_changed(const multi_exec &exec, const flaw &f)
    {
        for (auto &l : listeners)
            l->flaw_state_changed(exec, f);
    }
    void multi_executor::fire_flaw_cost_changed(const multi_exec &exec, const flaw &f)
    {
        for (auto &l : listeners)
            l->flaw_cost_changed(exec, f);
    }
    void multi_executor::fire_flaw_position_changed(const multi_exec &exec, const flaw &f)
    {
        for (auto &l : listeners)
            l->flaw_position_changed(exec, f);
    }
    void multi_executor::fire_current_flaw(const multi_exec &exec, const flaw &f)
    {
        for (auto &l : listeners)
            l->current_flaw(exec, f);
    }
    void multi_executor::fire_resolver_created(const multi_exec &exec, const resolver &r)
    {
        for (auto &l : listeners)
            l->resolver_created(exec, r);
    }
    void multi_executor::fire_resolver_state_changed(const multi_exec &exec, const resolver &r)
    {
        for (auto &l : listeners)
            l->resolver_state_changed(exec, r);
    }
    void multi_executor::fire_current_resolver(const multi_exec &exec, const resolver &r)
    {
        for (auto &l : listeners)
            l->current_resolver(exec, r);
    }
    void multi_executor::fire_causal_link_added(const multi_exec &exec, const flaw &f, const resolver &r)
    {
        for (auto &l : listeners)
            l->causal_link_added(exec, f, r);
    }
    void multi_executor::fire_tick(const multi_exec &exec, const utils::rational &time)
    {
        for (auto &l : listeners)
            l->tick(exec, time);
    }
    void multi_executor::fire_starting(const multi_exec &exec, const std::unordered_set<ratio::atom *> &atms)
    {
        for (auto &l : listeners)
            l->starting(exec, atms);
    }
    void multi_executor::fire_start(const multi_exec &exec, const std::unordered_set<ratio::atom *> &atms)
    {
        for (auto &l : listeners)
            l->start(exec, atms);
    }
    void multi_executor::fire_ending(const multi_exec &exec, const std::unordered_set<ratio::atom *> &atms)
    {
        for (auto &l : listeners)
            l->ending(exec, atms);
    }
    void multi_executor::fire_end(const multi_exec &exec, const std::unordered_set<ratio::atom *> &atms)
    {
        for (auto &l : listeners)
            l->end(exec, atms);
    }
} // namespace ratio::executor
