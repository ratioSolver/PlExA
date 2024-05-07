#pragma once

#include "solver.hpp"

namespace ratio::executor
{
  class executor_theory;

  enum executor_state
  {
    Reasoning,
    Idle,
    Adapting,
    Executing,
    Finished,
    Failed
  };

  inline std::string to_string(executor_state state)
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
    default: // should never happen..
      return "unknown";
    }
  }

  class executor
  {
  public:
    executor(std::shared_ptr<ratio::solver> slv = std::make_shared<ratio::solver>(), const utils::rational &units_per_tick = utils::rational::one) noexcept;
    virtual ~executor() noexcept = default;

    /**
     * Returns a reference to the solver object.
     *
     * @return A reference to the solver object.
     */
    ratio::solver &get_solver() noexcept { return *slv; }
    /**
     * Returns a reference to the solver object.
     *
     * @return A reference to the solver object.
     */
    const ratio::solver &get_solver() const noexcept { return *slv; }
    /**
     * @brief Get the state of the executor.
     *
     * @return The state of the executor.
     */
    executor_state get_state() const noexcept { return state; }
    /**
     * Returns the current time.
     *
     * @return A reference to the current time.
     */
    const utils::rational &get_current_time() const noexcept { return current_time; }

  private:
    /**
     * @brief Called when the state of the executor changes.
     */
    virtual void executor_state_changed(executor_state state);

    /**
     * @brief Called each time the executor is ticked.
     *
     * @param time The current time in plan units.
     */
    virtual void tick(const utils::rational &time);

    /**
     * @brief Called when the executor is starting some atoms.
     *
     * This is the best time to tell the executor to do delay the starting of some atoms.
     *
     * @param atms The atoms that are starting.
     */
    virtual void starting(const std::vector<std::reference_wrapper<const ratio::atom>> &atms);

    /**
     * @brief Called when the executor started some atoms.
     *
     * @param atms The atoms that are started.
     */
    virtual void start(const std::vector<std::reference_wrapper<const ratio::atom>> &atms);

    /**
     * @brief Called when the executor is ending some atoms.
     *
     * This is the best time to tell the executor to do delay the ending of some atoms.
     *
     * @param atms The atoms that are ending.
     */
    virtual void ending(const std::vector<std::reference_wrapper<const ratio::atom>> &atms);

    /**
     * @brief Called when the executor ended some atoms.
     *
     * @param atms The atoms that ended.
     */
    virtual void end(const std::vector<std::reference_wrapper<const ratio::atom>> &atms);

  protected:
    std::shared_ptr<ratio::solver> slv; // the solver..

  private:
    executor_theory &exec_theory;                     // the executor theory..
    executor_state state = executor_state::Reasoning; // the current state of the executor..
    const utils::rational units_per_tick;             // the number of plan units for each tick..
    utils::rational current_time;                     // the current time in plan units..
  };

#ifdef ENABLE_VISUALIZATION
  /**
   * @brief Creates a JSON message for an executor.
   *
   * This function creates a JSON message that represents an executor. The message includes the executor's ID, name, and state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the executor message.
   */
  inline json::json to_json(const executor &exec) { return {{"id", get_id(exec.get_solver())}, {"name", exec.get_solver().get_name()}, {"state", to_string(exec.get_state())}}; }

  /**
   * @brief Creates a JSON message for a new executor.
   *
   * This function creates a JSON message that represents a new executor. The message includes the executor's ID, name, and state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the new executor message.
   */
  inline json::json new_executor_message(const executor &exec)
  {
    auto exec_msg = to_json(exec);
    exec_msg["type"] = "new_executor";
    return exec_msg;
  }

  /**
   * @brief Creates a JSON message indicating that an executor has been deleted.
   *
   * This function creates a JSON message that indicates that an executor has been deleted. The message includes the ID of the deleted executor.
   *
   * @param id The ID of the deleted executor.
   * @return A JSON object representing the deleted executor message.
   */
  inline json::json deleted_executor_message(const uintptr_t id) { return {{"type", "deleted_executor"}, {"id", id}}; }
#endif
} // namespace ratio::executor
