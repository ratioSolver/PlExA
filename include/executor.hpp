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

  /**
   * @class executor
   * @brief Represents an executor for solving and executing atoms.
   *
   * The `executor` class is responsible for managing the execution of atoms and solving the associated constraints.
   * It provides methods for initializing the executor, accessing the solver object, retrieving the executor state,
   * getting the current time, and obtaining information about the executing atoms.
   */
  class executor
  {
  public:
    /**
     * @brief Constructs an executor object.
     *
     * This constructor initializes an executor object with the specified solver and units per tick.
     *
     * @param slv A shared pointer to a `ratio::solver` object. Default is a newly created `ratio::solver` object.
     * @param units_per_tick The number of units per tick. Default is `utils::rational::one`.
     */
    executor(std::shared_ptr<ratio::solver> slv = std::make_shared<ratio::solver>(), const utils::rational &units_per_tick = utils::rational::one) noexcept;
    virtual ~executor() noexcept = default;

    /**
     * Initializes the executor.
     */
    void init();

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

    /**
     * Returns a vector of const references to the executing atoms.
     *
     * This function returns a vector containing const references to the atoms that are currently executing.
     * The references are wrapped in `std::reference_wrapper` to allow storing them in a vector.
     *
     * @return A vector of const references to the executing atoms.
     */
    std::vector<std::reference_wrapper<const ratio::atom>> get_executing_atoms() const noexcept
    {
      std::vector<std::reference_wrapper<const ratio::atom>> atoms;
      for (const auto &atm : executing)
        atoms.push_back(std::cref(*atm));
      return atoms;
    }

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
    executor_theory &exec_theory;                      // the executor theory..
    executor_state state = executor_state::Reasoning;  // the current state of the executor..
    const utils::rational units_per_tick;              // the number of plan units for each tick..
    utils::rational current_time;                      // the current time in plan units..
    std::unordered_set<const ratio::atom *> executing; // the atoms that are currently executing..
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

  /**
   * @brief Creates a JSON message indicating that the state of an executor has changed.
   *
   * This function creates a JSON message that indicates that the state of an executor has changed. The message includes the ID of the executor and its new state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the state changed message.
   */
  inline json::json state_changed_message(const executor &exec) { return {{"type", "executor_state_changed"}, {"id", get_id(exec.get_solver())}, {"state", to_string(exec.get_state())}}; }

  /**
   * Returns a JSON object representing the state message of the executor.
   *
   * @param exec The executor object.
   * @return A JSON object representing the state message.
   */
  inline json::json state_message(const executor &exec)
  {
    auto state_msg = to_json(exec.get_solver());
    state_msg["type"] = "executor_state";
    state_msg["state"] = to_string(exec.get_state());
    state_msg["time"] = ratio::to_json(exec.get_current_time());
    json::json executing_atoms(json::json_type::array);
    for (const auto &atm : exec.get_executing_atoms())
      executing_atoms.push_back(get_id(atm.get()));
    state_msg["executing_atoms"] = std::move(executing_atoms);
    return state_msg;
  }
#endif
} // namespace ratio::executor
