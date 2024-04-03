#pragma once

#include "solver.hpp"

namespace ratio::executor
{
  enum executor_state
  {
    Reasoning,
    Idle,
    Adapting,
    Executing,
    Finished,
    Failed
  };

  class executor : public semitone::theory
  {
  public:
    executor(ratio::solver &slv, const utils::rational &units_per_tick = utils::rational::one) noexcept;

    executor_state get_state() const noexcept { return state; }
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
    ratio::solver &slv; // the solver..

  private:
    executor_state state = executor_state::Reasoning; // the current state of the executor..
    const utils::rational units_per_tick;             // the number of plan units for each tick..
    utils::rational current_time;                     // the current time in plan units..
    utils::lit xi;                                    // the execution variable..
  };
} // namespace ratio::executor
