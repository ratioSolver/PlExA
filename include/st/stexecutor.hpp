#pragma once

#include "plexa.hpp"
#include "stsolver.hpp"

namespace ratio::executor
{
  struct adaptation
  {
    explicit adaptation(const riddle::atom &atm) : atm(atm) {}
    virtual ~adaptation() = default;

    const riddle::atom &atm; // the atom to adapt
  };

  struct bool_adaptation : public adaptation
  {
    bool_adaptation(const riddle::atom &atm, const riddle::bool_item &bi, const utils::lbool &val) : adaptation(atm), bi(bi), val(val) {}

    const riddle::bool_item &bi; // the bool item to adapt
    utils::lbool val;            // the value to adapt to
  };

  struct lb_adaption : public adaptation
  {
    lb_adaption(const riddle::atom &atm, const riddle::arith_item &ai, const utils::inf_rational &lb) : adaptation(atm), ai(ai), lb(lb) {}

    const riddle::arith_item &ai; // the arith item to adapt
    utils::inf_rational lb;       // the lower bound to adapt to
  };

  struct ub_adaption : public adaptation
  {
    ub_adaption(const riddle::atom &atm, const riddle::arith_item &ai, const utils::inf_rational &ub) : adaptation(atm), ai(ai), ub(ub) {}

    const riddle::arith_item &ai; // the arith item to adapt
    utils::inf_rational ub;       // the upper bound to adapt to
  };

  struct var_adaptation : public adaptation
  {
    var_adaptation(const riddle::atom &atm, const riddle::enum_item &ei, utils::enum_val &val) : adaptation(atm), ei(ei), val(val) {}

    const riddle::enum_item &ei; // the enum item to adapt
    utils::enum_val &val;        // the value to adapt to
  };

  class executor : public ratio::solver, public smt::theory, public plexa
  {
  public:
    executor(std::string_view name = "oRatio", const utils::rational &units_per_tick = utils::rational::one) noexcept;

    void adapt(const std::string &script) override;

    void dont_start_yet(const std::unordered_map<riddle::atom_term *, utils::rational> &atoms) override;
    void dont_end_yet(const std::unordered_map<riddle::atom_term *, utils::rational> &atoms) override;

    void freeze_start(const std::vector<std::reference_wrapper<riddle::atom_term>> &atoms) override;
    void freeze_end(const std::vector<std::reference_wrapper<riddle::atom_term>> &atoms) override;

    void failure(const std::unordered_set<const riddle::atom_term *> &atoms) override;

  private:
    [[nodiscard]] bool propagate(const utils::lit &p) noexcept override;
    [[nodiscard]] bool check() noexcept override;
    void push() noexcept override;
    void pop() noexcept override;

  private:
    bool pending_requirements = false;                                                    // whether there are pending requirements to be solved or not..
    std::unordered_map<utils::var, std::vector<std::unique_ptr<adaptation>>> adaptations; // the adaptations to be applied
  };
} // namespace ratio::executor
