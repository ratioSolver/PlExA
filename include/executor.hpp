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
      return "Reasoning";
    case Idle:
      return "Idle";
    case Adapting:
      return "Adapting";
    case Executing:
      return "Executing";
    case Finished:
      return "Finished";
    case Failed:
      return "Failed";
    default:
      return "Unknown";
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
} // namespace ratio::executor
