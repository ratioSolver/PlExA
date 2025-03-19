#pragma once

#include "plexa.hpp"
#include "stsolver.hpp"

namespace ratio::executor
{
  class executor : public plexa, public ratio::solver
  {
  public:
    executor(const utils::rational &units_per_tick = utils::rational::one) noexcept;
  };
} // namespace ratio::executor
