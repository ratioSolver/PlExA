#pragma once

#include "executor.hpp"

namespace ratio::executor
{
  class stexecutor : public executor
  {
  public:
    stexecutor(const utils::rational &units_per_tick = utils::rational::one) noexcept;
  };
} // namespace ratio::executor
