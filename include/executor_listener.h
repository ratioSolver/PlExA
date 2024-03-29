#pragma once

#include "executor.h"

namespace ratio::executor
{
  class executor_listener
  {
    friend class executor;

  public:
    /**
     * @brief Construct a new executor listener object.
     *
     * @param e the executor to listen to.
     */
    executor_listener(executor &e) : exec(e) { exec.listeners.push_back(this); }
    executor_listener(const executor_listener &that) = delete;
    virtual ~executor_listener() { exec.listeners.erase(std::find(exec.listeners.cbegin(), exec.listeners.cend(), this)); }

  private:
    virtual void executor_state_changed([[maybe_unused]] executor_state state) {}

    /**
     * @brief Notifies the listener the passing of time.
     */
    virtual void tick([[maybe_unused]] const utils::rational &time) { LOG("current time: " << to_string(time)); }

    /**
     * @brief Notifies the listener that some atoms are going to start.
     *
     * This is the best time to tell the executor to do delay the starting of some atoms.
     *
     * @param atoms the set of atoms which are going to start.
     */
    virtual void starting(const std::unordered_set<ratio::atom *> &) {}
    /**
     * @brief Notifies the listener that some atoms have started.
     *
     * @param atoms the set of atoms which have started.
     */
    virtual void start(const std::unordered_set<ratio::atom *> &) {}

    /**
     * @brief Notifies the listener that some atoms are going to end.
     *
     * This is the best time to tell the executor to do delay the ending of some atoms.
     *
     * @param atoms the set of atoms which are going to end.
     */
    virtual void ending(const std::unordered_set<ratio::atom *> &) {}
    /**
     * @brief Notifies the listener that some atoms have ended.
     *
     * @param atoms the set of atoms which have ended.
     */
    virtual void end(const std::unordered_set<ratio::atom *> &) {}

  protected:
    executor &exec;
  };
} // namespace ratio::executor
