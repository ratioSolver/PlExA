#pragma once

#include "solver.hpp"
#ifdef MULTIPLE_EXECUTORS
#include <mutex>
#include <atomic>
#endif

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

  /**
   * Converts an executor_state enum value to its corresponding string representation.
   *
   * @param state The executor_state value to convert.
   * @return The string representation of the executor_state value.
   */
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
     * @brief Adapts the planning problem taking into account the given RiDDLe script.
     *
     * This function adapts the planning problem by taking into account the given RiDDLe script.
     *
     * @param script The RiDDLe script to adapt the planning problem.
     */
    void adapt(const std::string &script);
    /**
     * @brief Adapts the planning problem taking into account the given RiDDLe files.
     *
     * This function adapts the planning problem by taking into account the given RiDDLe files.
     *
     * @param files The RiDDLe files to adapt the planning problem.
     */
    void adapt(const std::vector<std::string> &files);

    /**
     * @brief Checks if the executor is currently running.
     *
     * @return true if the executor is running, false otherwise.
     */
    bool is_running() const noexcept { return running; }

    /**
     * @brief Starts the executor.
     *
     * This function starts the executor and sets the state to `Executing`.
     */
    void start();

    /**
     * @brief Pauses the executor.
     *
     * This function pauses the executor and sets the state to `Idle`.
     */
    void pause();

    /**
     * @brief Executes a single tick of the executor.
     *
     * This function performs a single iteration of the executor's main loop.
     * It is responsible for executing any pending tasks or events.
     *
     * @note This function should be called periodically to ensure proper execution of the executor.
     */
    void tick();

    /**
     * @brief Returns a vector of const references to the executing atoms.
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

    /**
     * @brief Inserts the given atoms into the `dont_start` set.
     * 
     * This function inserts the given atoms into the `dont_start` set, which contains the atoms that are not yet ready to start. The solver will adapt the plan to delay the starting of these atoms.
     *
     * @param atoms The set of atoms which are not yet ready to start and the corresponding delay time.
     */
    void dont_start_yet(const std::unordered_map<const ratio::atom *, utils::rational> &atoms) { dont_start.insert(atoms.cbegin(), atoms.cend()); }
    /**
     * @brief Inserts the given atoms into the `dont_end` unordered map.
     * 
     * This function inserts the given atoms into the `dont_end` set, which contains the atoms that are not yet ready to end. The solver will adapt the plan to delay the ending of these atoms.
     *
     * @param atoms The set of atoms which are not yet ready to end and the corresponding delay time.
     */
    void dont_end_yet(const std::unordered_map<const ratio::atom *, utils::rational> &atoms) { dont_end.insert(atoms.cbegin(), atoms.cend()); }
    /**
     * @brief Notifies the executor that the given atoms have failed.
     * 
     * This function notifies the executor that the given atoms have failed. The solver will adapt the plan to handle the failure of these atoms.
     * 
     * @param atoms The set of atoms that have failed.
     */
    void failure(const std::unordered_set<const ratio::atom *> &atoms);

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
#ifdef MULTIPLE_EXECUTORS
    std::mutex mtx;                    // the mutex for the critical sections..
    std::atomic<bool> running = false; // the running state..
#else
    bool running = false; // the execution state..
#endif
    bool pending_requirements = false;                                   // whether there are pending requirements to be solved or not..
    utils::rational current_time;                                        // the current time in plan units..
    std::unordered_set<const ratio::atom *> executing;                   // the atoms that are currently executing..
    std::unordered_map<const ratio::atom *, utils::rational> dont_start; // the starting atoms which are not yet ready to start..
    std::unordered_map<const ratio::atom *, utils::rational> dont_end;   // the ending atoms which are not yet ready to end..
  };

#ifdef ENABLE_VISUALIZATION
  /**
   * @brief Creates a JSON message for an executor.
   *
   * This function creates a JSON message that represents an executor. The message includes the executor's ID, name, time, and state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the executor message.
   */
  inline json::json to_json(const executor &exec) { return {{"id", get_id(exec.get_solver())}, {"name", exec.get_solver().get_name()}, {"time", ratio::to_json(exec.get_current_time())}, {"state", to_string(exec.get_state())}}; }

  /**
   * @brief Creates a JSON message for a new executor.
   *
   * This function creates a JSON message that represents a new executor. The message includes the executor's ID, name, and state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the new executor message.
   */
  inline json::json make_new_solver_message(const executor &exec)
  {
    auto exec_msg = to_json(exec);
    exec_msg["type"] = "new_solver";
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
  inline json::json make_deleted_solver_message(const uintptr_t id) { return {{"type", "deleted_solver"}, {"id", id}}; }

  /**
   * @brief Creates a JSON message indicating that the state of an executor has changed.
   *
   * This function creates a JSON message that indicates that the state of an executor has changed. The message includes the ID of the executor and its new state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the state changed message.
   */
  inline json::json make_solver_execution_state_changed_message(const executor &exec) { return {{"type", "solver_execution_state_changed"}, {"id", get_id(exec.get_solver())}, {"state", to_string(exec.get_state())}}; }

  /**
   * Creates a JSON message representing the state of an executor.
   *
   * @param exec The executor object.
   * @return A JSON object representing the state of the executor.
   */
  inline json::json make_solver_state_message(const executor &exec)
  {
    auto state_msg = to_json(exec.get_solver());
    state_msg["type"] = "solver_state";
    state_msg["id"] = get_id(exec.get_solver());
    state_msg["time"] = ratio::to_json(exec.get_current_time());
    auto timelines = to_timelines(exec.get_solver());
    if (!timelines.as_array().empty())
      state_msg["timelines"] = std::move(timelines);
    json::json executing_atoms(json::json_type::array);
    for (const auto &atm : exec.get_executing_atoms())
      executing_atoms.push_back(get_id(atm.get()));
    if (!executing_atoms.as_array().empty())
      state_msg["executing_atoms"] = std::move(executing_atoms);
    return state_msg;
  }

  /**
   * Creates a JSON tick message.
   *
   * This function creates a JSON message of type "tick" that contains the solver ID and the current time.
   *
   * @param exec The executor object.
   * @return A JSON object representing the tick message.
   */
  inline json::json make_tick_message(const executor &exec) { return {{"type", "tick"}, {"solver_id", get_id(exec.get_solver())}, {"time", ratio::to_json(exec.get_current_time())}}; }

  const json::json solver_schema{"solver",
                                 {{"type", "object"},
                                  {"properties",
                                   {{"id", {{"type", "string"}}},
                                    {"name", {{"type", "string"}}},
                                    {"time", {{"$ref", "#/components/schemas/rational"}}},
                                    {"state", {{"type", "string"}, {"enum", {"reasoning", "adapting", "idle", "executing", "finished", "failed"}}}}}}}};
  const json::json new_solver_message{"new_solver_message",
                                      {"payload",
                                       {{"type", "object"},
                                        {"properties",
                                         {{"type", {{"type", "string"}, {"enum", {"new_solver"}}}},
                                          {"id", {{"type", "string"}}},
                                          {"name", {{"type", "string"}}},
                                          {"time", {{"$ref", "#/components/schemas/rational"}}},
                                          {"state", {{"type", "string"}, {"enum", {"reasoning", "adapting", "idle", "executing", "finished", "failed"}}}}}},
                                        {"required", {"id", "name", "state"}}}}};
  const json::json solver_execution_state_changed_message{"solver_execution_state_changed_message",
                                                          {"payload",
                                                           {{"type", "object"},
                                                            {"properties",
                                                             {{"type", {{"type", "string"}, {"enum", {"solver_execution_state_changed"}}}},
                                                              {"id", {{"type", "string"}}},
                                                              {"state", {{"type", "string"}, {"enum", {"reasoning", "adapting", "idle", "executing", "finished", "failed"}}}}}},
                                                            {"required", std::vector<json::json>{"id", "state"}}}}};
  const json::json tick_message{"tick_message",
                                {"payload",
                                 {{"type", "object"},
                                  {"properties",
                                   {{"type", {{"type", "string"}, {"enum", {"tick"}}}},
                                    {"solver_id", {{"type", "string"}}},
                                    {"time", {{"$ref", "#/components/schemas/rational"}}}}}}}};
  const json::json solver_state_message{
      {"state_message",
       {"payload",
        {{"allOf",
          std::vector<json::json>{{"$ref", "#/components/schemas/solver_state"}}},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"solver_state"}}}},
           {"id", {{"type", "integer"}}},
           {"time", {{"$ref", "#/components/schemas/rational"}}},
           {"timelines", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/timeline"}}}}},
           {"executing_atoms", {{"type", "array"}, {"description", "The IDs of the atoms that are currently executing."}, {"items", {{"type", "integer"}}}}}}}}}}};
#endif
} // namespace ratio::executor
