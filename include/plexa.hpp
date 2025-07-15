#pragma once

#include "inf_rational.hpp"
#include "term.hpp"
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include <map>

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

  class plexa
  {
  public:
    plexa(const utils::rational &units_per_tick = utils::rational::one);
    virtual ~plexa() = default;

    virtual void adapt(const std::string &script) = 0;

    [[nodiscard]] bool is_running() const noexcept { return running; }
    [[nodiscard]] executor_state get_state() const noexcept { return state; }
    [[nodiscard]] utils::rational get_current_time() const noexcept { return current_time; }

    void start();
    void pause();

    void tick();

    /**
     * @brief Returns a vector of const references to the executing atoms.
     *
     * This function returns a vector containing const references to the atoms that are currently executing.
     * The references are wrapped in `std::reference_wrapper` to allow storing them in a vector.
     *
     * @return A vector of const references to the executing atoms.
     */
    std::vector<std::reference_wrapper<const riddle::atom_term>> get_executing_atoms() const noexcept
    {
      std::vector<std::reference_wrapper<const riddle::atom_term>> atoms;
      for (const auto &atm : executing)
        atoms.push_back(*atm);
      return atoms;
    }

    /**
     * @brief Inserts the given atoms into the `dont_start` set.
     *
     * This function inserts the given atoms into the `dont_start` set, which contains the atoms that are not yet ready to start. The solver will adapt the plan to delay the starting of these atoms.
     *
     * @param atoms The set of atoms which are not yet ready to start and the corresponding delay time.
     */
    void dont_start_yet(const std::unordered_map<const riddle::atom_term *, utils::rational> &atoms) { dont_start.insert(atoms.cbegin(), atoms.cend()); }
    /**
     * @brief Inserts the given atoms into the `dont_end` unordered map.
     *
     * This function inserts the given atoms into the `dont_end` set, which contains the atoms that are not yet ready to end. The solver will adapt the plan to delay the ending of these atoms.
     *
     * @param atoms The set of atoms which are not yet ready to end and the corresponding delay time.
     */
    void dont_end_yet(const std::unordered_map<const riddle::atom_term *, utils::rational> &atoms) { dont_end.insert(atoms.cbegin(), atoms.cend()); }
    /**
     * @brief Notifies the executor that the given atoms have failed.
     *
     * This function notifies the executor that the given atoms have failed. The solver will adapt the plan to handle the failure of these atoms.
     *
     * @param atoms The set of atoms that have failed.
     */
    virtual void failure(const std::unordered_set<const riddle::atom_term *> &atoms) = 0;

  protected:
    void build_timelines(const riddle::core &cr);

    void reset_executable_predicates(const riddle::core &cr);

  private:
    virtual void adapt() = 0;

  private:
    /**
     * @brief Called when the state of the executor changes.
     */
    virtual void executor_state_changed(executor_state) {}

    /**
     * @brief Called each time the executor is ticked.
     *
     * @param time The current time in plan units.
     */
    virtual void tick(const utils::rational &) {}

    /**
     * @brief Called when the executor is starting some atoms.
     *
     * This is the best time to tell the executor to do delay the starting of some atoms.
     *
     * @param atms The atoms that are starting.
     */
    virtual void starting(const std::vector<std::reference_wrapper<riddle::atom_term>> &) {}

    /**
     * @brief Called when the executor started some atoms.
     *
     * @param atms The atoms that are started.
     */
    virtual void start(const std::vector<std::reference_wrapper<riddle::atom_term>> &) {}

    /**
     * @brief Called when the executor is ending some atoms.
     *
     * This is the best time to tell the executor to do delay the ending of some atoms.
     *
     * @param atms The atoms that are ending.
     */
    virtual void ending(const std::vector<std::reference_wrapper<riddle::atom_term>> &) {}

    /**
     * @brief Called when the executor ended some atoms.
     *
     * @param atms The atoms that ended.
     */
    virtual void end(const std::vector<std::reference_wrapper<riddle::atom_term>> &) {}

  protected:
    std::mutex mtx; // the mutex for the critical sections..
  private:
    std::atomic<bool> running = false;                                                                                   // the running state..
    executor_state state = executor_state::Reasoning;                                                                    // the current state of the executor..
    std::unordered_set<const riddle::predicate *> executable_predicates;                                                 // the set of executable (impulses and intervals) predicates..
    const utils::rational units_per_tick;                                                                                // the number of plan units for each tick..
    bool pending_requirements = false;                                                                                   // whether there are pending requirements to be solved or not..
    utils::rational current_time;                                                                                        // the current time in plan units..
    std::unordered_set<const riddle::atom_term *> executing;                                                             // the atoms that are currently executing..
    std::unordered_map<const riddle::atom_term *, utils::rational> dont_start;                                           // the starting atoms which are not yet ready to start..
    std::unordered_map<const riddle::atom_term *, utils::rational> dont_end;                                             // the ending atoms which are not yet ready to end..
    std::map<utils::inf_rational, std::pair<std::vector<riddle::atom_term *>, std::vector<riddle::atom_term *>>> pulses; // the pulses of the executor, with the starting and ending atoms..
  };
} // namespace ratio::executor
