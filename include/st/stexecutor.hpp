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

    void dont_start_yet(const std::unordered_map<riddle::atom_term *, utils::rational> &atoms) override;
    void dont_end_yet(const std::unordered_map<riddle::atom_term *, utils::rational> &atoms) override;

    void failure(const std::unordered_set<const riddle::atom_term *> &atoms) override;

  private:
    bool pending_requirements = false; // whether there are pending requirements to be solved or not..
  };
} // namespace ratio::executor
