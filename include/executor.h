#pragma once

#include "plexa_export.h"
#include "core_listener.h"
#include "solver_listener.h"
#include "solver.h"
#ifdef MULTIPLE_EXECUTORS
#include <mutex>
#include <atomic>
#endif

namespace ratio::executor
{
  class executor_listener;

  enum executor_state
  {
    Reasoning,
    Idle,
    Adapting,
    Executing,
    Finished,
    Failed
  };

  struct atom_adaptation
  {
    struct item_bounds
    {
      virtual ~item_bounds() = default;
    };
    using bounds_ptr = utils::u_ptr<item_bounds>;

    struct bool_bounds : public item_bounds
    {
      bool_bounds(const utils::lbool &val) : val(val) {}
      const utils::lbool val;
    };
    struct arith_bounds : public item_bounds
    {
      arith_bounds(const utils::inf_rational &lb, const utils::inf_rational &ub) : lb(lb), ub(ub) {}
      utils::inf_rational lb, ub;
    };
    struct var_bounds : public item_bounds
    {
      var_bounds(utils::enum_val &val) : val(val) {}
      utils::enum_val &val;
    };

    atom_adaptation(const semitone::lit &sigma_xi) : sigma_xi(sigma_xi) {}

    semitone::lit sigma_xi;
    std::unordered_map<riddle::item *, bounds_ptr> bounds;
  };

  class executor final : public riddle::core_listener, public ratio::solver_listener, public semitone::theory
  {
    friend class executor_listener;

  public:
    /**
     * @brief Construct a new executor object.
     *
     * @param slv the solver maintaining the solution to execute.
     * @param units_per_tick the amount of units to increase the current time at each tick.
     */
    PLEXA_EXPORT executor(ratio::solver &slv, const std::string &name = "default", const utils::rational &units_per_tick = utils::rational::ONE);
    executor(const executor &orig) = delete;

    ratio::solver &get_solver() { return slv; }
    const ratio::solver &get_solver() const { return slv; }
    const std::string &get_name() const { return name; }
    executor_state get_state() const { return state; }

    /**
     * @brief Gets the current time.
     *
     * @return const utils::rational& the current time.
     */
    const utils::rational &get_current_time() const { return current_time; };
    /**
     * @brief Gets the amount of units to increase the current time at each tick.
     *
     * @return const utils::rational& the amount of units to increase the current time at each tick.
     */
    const utils::rational &get_units_per_tick() const { return units_per_tick; };

    /**
     * @brief Checks whether the current solution is being executed.
     *
     * @return true if the current solution is being executed.
     * @return false if the current solution is paused.
     */
    bool is_running() const { return running; }

    /**
     * @brief Gets the atoms which are currently executing.
     *
     * @return const std::unordered_set<const ratio::atom *>& the atoms which are currently executing.
     */
    const std::unordered_set<const ratio::atom *> &get_executing() const { return executing; }

    /**
     * @brief Starts the execution of the current solution.
     *
     */
    PLEXA_EXPORT void start_execution();
    /**
     * @brief Pauses the execution of the current solution.
     *
     */
    PLEXA_EXPORT void pause_execution();

    /**
     * @brief Performs a single execution step, increasing the current time of a `units_per_tick` amount, starting (ending) any task which starts (ends) between the `current_time` and `current_time + units_per_tick`.
     *
     * Before starting (ending) the execution of a task, the executor notifies the listeners via the `starting` (`ending`) methods. Listeners can here introduce delays through the `dont_start_yet` (`dont_end_yet`) methods.
     */
    PLEXA_EXPORT void tick();

    PLEXA_EXPORT void adapt(const std::string &script);
    PLEXA_EXPORT void adapt(const std::vector<std::string> &files);

    PLEXA_EXPORT void dont_start_yet(const std::unordered_map<const ratio::atom *, utils::rational> &atoms) { dont_start.insert(atoms.cbegin(), atoms.cend()); }
    PLEXA_EXPORT void dont_end_yet(const std::unordered_map<const ratio::atom *, utils::rational> &atoms) { dont_end.insert(atoms.cbegin(), atoms.cend()); }
    PLEXA_EXPORT void failure(const std::unordered_set<const ratio::atom *> &atoms);

  private:
    bool propagate(const semitone::lit &p) noexcept override;
    bool check() noexcept override { return true; }
    void push() noexcept override {}
    void pop() noexcept override {}

    inline bool is_relevant(const riddle::predicate &pred) const noexcept { return relevant_predicates.count(&pred); }

    void read(const std::string &) override { reset_relevant_predicates(); }
    void read(const std::vector<std::string> &) override { reset_relevant_predicates(); }
    void started_solving() override;
    void solution_found() override;
    void inconsistent_problem() override;

    void flaw_created(const ratio::flaw &f) override;

    void build_timelines();
    bool propagate_bounds(const riddle::item &itm, const atom_adaptation::item_bounds &bounds, const semitone::lit &reason);

    void reset_relevant_predicates();

  private:
    const std::string name;
    executor_state state = executor_state::Reasoning;                  // the current state of the executor..
    std::unordered_set<const riddle::predicate *> relevant_predicates; // impulses and intervals..
    utils::rational current_time;                                      // the current time in plan units..
    const utils::rational units_per_tick;                              // the number of plan units for each tick..
    semitone::lit xi;                                                  // the execution variable..
    bool pending_requirements = false;                                 // whether there are pending requirements to be solved or not..
#ifdef MULTIPLE_EXECUTORS
    std::mutex mtx;                    // the mutex for the critical sections..
    std::atomic<bool> running = false; // the running state..
#else
    bool running = false; // the execution state..
#endif
    std::unordered_set<const ratio::atom *> executing;                               // the atoms currently executing..
    std::unordered_map<const ratio::atom *, atom_adaptation> adaptations;            // for each atom, the numeric adaptations done during the executions (i.e., freezes and delays)..
    std::unordered_map<semitone::var, const ratio::atom *> all_atoms;                // all the interesting atoms indexed by their sigma_xi variable..
    std::unordered_map<const ratio::atom *, utils::rational> dont_start;             // the starting atoms which are not yet ready to start..
    std::unordered_map<const ratio::atom *, utils::rational> dont_end;               // the ending atoms which are not yet ready to end..
    std::map<utils::inf_rational, std::unordered_set<ratio::atom *>> s_atms, e_atms; // for each pulse, the atoms starting/ending at that pulse..
    std::set<utils::inf_rational> pulses;                                            // all the pulses of the plan..
    std::vector<executor_listener *> listeners;                                      // the executor listeners..
  };

  class execution_exception : public std::exception
  {
    const char *what() const noexcept override { return "the plan cannot be executed.."; }
  };

  inline std::string to_string(executor_state state) noexcept
  {
    switch (state)
    {
    case Reasoning:
      return "reasoning";
    case Idle:
      return "idle";
    case Adapting:
      return "adapting";
    case Executing:
      return "executing";
    case Finished:
      return "finished";
    case Failed:
      return "failed";
    default:
      return "unknown";
    }
  }

  inline json::json new_solver_message(const executor &exec) { return {{"type", "new_solver"}, {"solver_id", get_id(exec.get_solver())}, {"name", exec.get_name()}}; }
  inline json::json deleted_solver_message(const uintptr_t id) { return {{"type", "removed_solver"}, {"solver_id", id}}; }

  inline json::json executor_state_changed_message(const executor &exec) { return {{"type", "executor_state_changed"}, {"solver_id", get_id(exec.get_solver())}, {"state", to_string(exec.get_state())}}; }

  inline json::json tick_message(const executor &exec, const utils::rational &time) { return {{"type", "tick"}, {"solver_id", get_id(exec.get_solver())}, {"time", to_json(time)}}; }

  inline json::json starting_message(const executor &exec, const std::unordered_set<ratio::atom *> &atoms)
  {
    json::json starting(json::json_type::array);
    for (const auto &atm : atoms)
      starting.push_back(get_id(*atm));
    return {{"type", "starting"}, {"solver_id", get_id(exec.get_solver())}, {"starting", std::move(starting)}};
  }
  inline json::json start_message(const executor &exec, const std::unordered_set<ratio::atom *> &atoms)
  {
    json::json starting(json::json_type::array);
    for (const auto &atm : atoms)
      starting.push_back(get_id(*atm));
    return {{"type", "start"}, {"solver_id", get_id(exec.get_solver())}, {"start", std::move(starting)}};
  }
  inline json::json ending_message(const executor &exec, const std::unordered_set<ratio::atom *> &atoms)
  {
    json::json ending(json::json_type::array);
    for (const auto &atm : atoms)
      ending.push_back(get_id(*atm));
    return {{"type", "ending"}, {"solver_id", get_id(exec.get_solver())}, {"ending", std::move(ending)}};
  }
  inline json::json end_message(const executor &exec, const std::unordered_set<ratio::atom *> &atoms)
  {
    json::json ending(json::json_type::array);
    for (const auto &atm : atoms)
      ending.push_back(get_id(*atm));
    return {{"type", "end"}, {"solver_id", get_id(exec.get_solver())}, {"end", std::move(ending)}};
  }

  inline json::json executor_state_message(const executor &exec)
  {
    json::json j_sc = solver_state_changed_message(exec.get_solver());
    j_sc["time"] = ratio::to_json(exec.get_current_time());
    json::json j_executing(json::json_type::array);
    for (const auto &atm : exec.get_executing())
      j_executing.push_back(get_id(*atm));
    j_sc["executing"] = std::move(j_executing);
    return j_sc;
  }
} // namespace ratio::executor