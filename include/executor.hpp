#pragma once

#include "solver.hpp"
#include "executor_state.hpp"
#ifdef MULTIPLE_EXECUTORS
#include <mutex>
#include <atomic>
#endif

namespace ratio::executor
{
  class executor_theory;

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
} // namespace ratio::executor
