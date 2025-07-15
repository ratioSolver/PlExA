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

  private:
    void adapt() override;

    void delay(riddle::arith_expr tp, const utils::rational &d) override;

  private:
    bool pending_requirements = false; // whether there are pending requirements to be solved or not..
  };
} // namespace ratio::executor
