#pragma once

#include "executor.h"

namespace ratio::executor
{
  class executor_listener
  {
    friend class executor;

  public:
    executor_listener(executor &e) : exec(e) { exec.listeners.push_back(this); }
    executor_listener(const executor_listener &that) = delete;
    virtual ~executor_listener() { exec.listeners.erase(std::find(exec.listeners.cbegin(), exec.listeners.cend(), this)); }

  private:
    virtual void tick([[maybe_unused]] const semitone::rational &time) { LOG("current time: " << to_string(time)); }

    /**
     * @brief Notifies the listener that some atoms are going to start.
     *
     * This is the best time to tell the executor to do delay the starting of some atoms.
     *
     * @param atoms the set of atoms which are going to start.
     */
    virtual void starting(const std::unordered_set<ratio::core::atom *> &) {}
    virtual void start(const std::unordered_set<ratio::core::atom *> &) {}

    /**
     * @brief Notifies the listener that some atoms are going to end.
     *
     * This is the best time to tell the executor to do delay the ending of some atoms.
     *
     * @param atoms the set of atoms which are going to end.
     */
    virtual void ending(const std::unordered_set<ratio::core::atom *> &) {}
    virtual void end(const std::unordered_set<ratio::core::atom *> &) {}

    /**
     * @brief Notifies the listener that there are no more activities to be executed.
     *
     */
    virtual void finished() {}

  protected:
    executor &exec;
  };
} // namespace ratio::executor
