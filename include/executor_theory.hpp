#pragma once

#include "theory.hpp"

namespace ratio::executor
{
  class executor;

  class executor_theory : public semitone::theory
  {
  public:
    executor_theory(executor &exec) noexcept;

  private:
    bool propagate(const utils::lit &) noexcept override { return true; }
    bool check() noexcept override { return true; }
    void push() noexcept override {}
    void pop() noexcept override {}

  private:
    executor &exec; // the executor
    utils::lit xi;  // the execution variable..
  };
} // namespace ratio::executor
