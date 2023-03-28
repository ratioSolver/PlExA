#pragma once

#include "executor.h"
#include "executor_listener.h"

namespace ratio::executor
{
  class multi_exec;
  using exec_ptr = utils::u_ptr<multi_exec>;
  class multi_executor_listener;

  class multi_executor
  {
    friend class multi_exec;
    friend class multi_executor_listener;

  public:
    multi_executor();

    multi_exec &new_solver(const std::string &name);
    void destroy_solver(multi_exec &exec);

  private:
    void fire_solver_created(const multi_exec &exec);
    void fire_solver_destroyed(const multi_exec &exec);

    void fire_log(const multi_exec &exec, const std::string &msg);
    void fire_read(const multi_exec &exec, const std::string &rddl);
    void fire_read(const multi_exec &exec, const std::vector<std::string> &files);

    void fire_state_changed(const multi_exec &exec);

    void fire_started_solving(const multi_exec &exec);
    void fire_solution_found(const multi_exec &exec);
    void fire_inconsistent_problem(const multi_exec &exec);

    void fire_flaw_created(const multi_exec &exec, const flaw &f);
    void fire_flaw_state_changed(const multi_exec &exec, const flaw &f);
    void fire_flaw_cost_changed(const multi_exec &exec, const flaw &f);
    void fire_flaw_position_changed(const multi_exec &exec, const flaw &f);
    void fire_current_flaw(const multi_exec &exec, const flaw &f);

    void fire_resolver_created(const multi_exec &exec, const resolver &r);
    void fire_resolver_state_changed(const multi_exec &exec, const resolver &r);
    void fire_current_resolver(const multi_exec &exec, const resolver &r);

    void fire_causal_link_added(const multi_exec &exec, const flaw &f, const resolver &r);

    void fire_tick(const multi_exec &exec, const utils::rational &time);

    void fire_starting(const multi_exec &exec, const std::unordered_set<ratio::atom *> &atms);
    void fire_start(const multi_exec &exec, const std::unordered_set<ratio::atom *> &atms);

    void fire_ending(const multi_exec &exec, const std::unordered_set<ratio::atom *> &atms);
    void fire_end(const multi_exec &exec, const std::unordered_set<ratio::atom *> &atms);

  private:
    std::vector<exec_ptr> executors;
    std::vector<multi_executor_listener *> listeners;
  };

  class multi_exec final : public riddle::core_listener, public ratio::solver_listener, public ratio::executor::executor_listener
  {
    friend class multi_executor;

  public:
    multi_exec(multi_executor &m_exec, ratio::solver &slv, ratio::executor::executor &exec, const std::string &name) : core_listener(slv), solver_listener(slv), executor_listener(exec), m_exec(m_exec), slv(slv), exec(exec), name(name) {}

    ratio::solver &get_solver() const noexcept { return slv; }
    ratio::executor::executor &get_executor() const noexcept { return exec; }
    const std::string &get_name() const noexcept { return name; }

  private:
    void log(const std::string &msg) override { m_exec.fire_log(*this, msg); }
    void read(const std::string &rddl) override { m_exec.fire_read(*this, rddl); }
    void read(const std::vector<std::string> &files) override { m_exec.fire_read(*this, files); }

    void state_changed() override { m_exec.fire_state_changed(*this); }

    void started_solving() override { m_exec.fire_started_solving(*this); }
    void solution_found() override { m_exec.fire_solution_found(*this); }
    void inconsistent_problem() override { m_exec.fire_inconsistent_problem(*this); }

    void flaw_created(const flaw &f) override { m_exec.fire_flaw_created(*this, f); }
    void flaw_state_changed(const flaw &f) override { m_exec.fire_flaw_state_changed(*this, f); }
    void flaw_cost_changed(const flaw &f) override { m_exec.fire_flaw_cost_changed(*this, f); }
    void flaw_position_changed(const flaw &f) override { m_exec.fire_flaw_position_changed(*this, f); }
    void current_flaw(const flaw &f) override { m_exec.fire_current_flaw(*this, f); }

    void resolver_created(const resolver &r) override { m_exec.fire_resolver_created(*this, r); }
    void resolver_state_changed(const resolver &r) override { m_exec.fire_resolver_state_changed(*this, r); }
    void current_resolver(const resolver &r) override { m_exec.fire_current_resolver(*this, r); }

    void causal_link_added(const flaw &f, const resolver &r) override { m_exec.fire_causal_link_added(*this, f, r); }

    void tick(const utils::rational &time) override { m_exec.fire_tick(*this, time); }

    void starting(const std::unordered_set<ratio::atom *> &atms) override { m_exec.fire_starting(*this, atms); }
    void start(const std::unordered_set<ratio::atom *> &atms) override { m_exec.fire_start(*this, atms); }

    void ending(const std::unordered_set<ratio::atom *> &atms) override { m_exec.fire_ending(*this, atms); }
    void end(const std::unordered_set<ratio::atom *> &atms) override { m_exec.fire_end(*this, atms); }

  private:
    multi_executor &m_exec;
    ratio::solver &slv;
    ratio::executor::executor &exec;
    const std::string &name;
  };

  inline uintptr_t get_id(const multi_exec &exec) noexcept { return reinterpret_cast<uintptr_t>(&exec); }

  json::json state_changed_message(const multi_exec &exec) noexcept { return json::json{{"type", "state_changed"}, {"id", get_id(exec)}}; }
} // namespace ratio::executor
