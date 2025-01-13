#pragma once

#include "z3solver.hpp"
#include "executor.hpp"

namespace ratio::executor
{
  class z3executor : public z3solver, public executor
  {
  public:
    z3executor(std::string_view name = "oRatio", const utils::rational &units_per_tick = utils::rational::one);
  };
} // namespace ratio::executor
