#pragma once

#include "plexa.hpp"
#include "stsolver.hpp"

namespace ratio::executor
{
  class executor : public ratio::solver, public plexa
  {
  public:
    executor(std::string_view name = "oRatio", const utils::rational &units_per_tick = utils::rational::one) noexcept;

    void adapt(const std::string &script) override;

    void failure(const std::unordered_set<const riddle::atom_term *> &atoms) override;
  };
} // namespace ratio::executor
