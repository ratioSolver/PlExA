#pragma once

#include "multi_executor.h"

namespace ratio::executor
{
  class multi_executor_listener
  {
    friend class multi_executor;

  public:
    multi_executor_listener(multi_executor &m_exec) : m_exec(m_exec) { m_exec.listeners.push_back(this); }
    virtual ~multi_executor_listener() { m_exec.listeners.erase(std::find(m_exec.listeners.begin(), m_exec.listeners.end(), this)); }

  private:
    virtual void solver_created(const multi_exec &) {}
    virtual void solver_destroyed(const multi_exec &) {}

    virtual void log(const multi_exec &, const std::string &) {}
    virtual void read(const multi_exec &, const std::string &) {}
    virtual void read(const multi_exec &, const std::vector<std::string> &) {}

    virtual void state_changed(const multi_exec &) {}

    virtual void started_solving(const multi_exec &) {}
    virtual void solution_found(const multi_exec &) {}
    virtual void inconsistent_problem(const multi_exec &) {}

    virtual void flaw_created(const multi_exec &, const flaw &) {}
    virtual void flaw_state_changed(const multi_exec &, const flaw &) {}
    virtual void flaw_cost_changed(const multi_exec &, const flaw &) {}
    virtual void flaw_position_changed(const multi_exec &, const flaw &) {}
    virtual void current_flaw(const multi_exec &, const flaw &) {}

    virtual void resolver_created(const multi_exec &, const resolver &) {}
    virtual void resolver_state_changed(const multi_exec &, const resolver &) {}
    virtual void current_resolver(const multi_exec &, const resolver &) {}

    virtual void causal_link_added(const multi_exec &, const flaw &, const resolver &) {}

    virtual void tick(const multi_exec &, const utils::rational &) {}

    virtual void starting(const multi_exec &, const std::unordered_set<ratio::atom *> &) {}
    virtual void start(const multi_exec &, const std::unordered_set<ratio::atom *> &) {}

    virtual void ending(const multi_exec &, const std::unordered_set<ratio::atom *> &) {}
    virtual void end(const multi_exec &, const std::unordered_set<ratio::atom *> &) {}

  protected:
    multi_executor &m_exec;
  };
} // namespace ratio::executor
