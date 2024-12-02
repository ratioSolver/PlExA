#pragma once

#include "solver.hpp"

namespace ratio::executor
{
  class executor;
  class executor_theory;

  class solver : public ratio::solver
  {
  public:
    solver(executor &exec, const std::string &name);

  protected:
    void flaw_created(const ratio::flaw &f) override;

  private:
    executor &exec;
  };

  struct atom_adaptation
  {
    struct item_bounds
    {
      virtual ~item_bounds() = default;
    };

    struct bool_bounds : public item_bounds
    {
      bool_bounds(const utils::lbool &val) : val(val) {}
      const utils::lbool val;
    };
    struct arith_bounds : public item_bounds
    {
      arith_bounds(const utils::inf_rational &lb, const utils::inf_rational &ub) : lb(lb), ub(ub) {}
      utils::inf_rational lb, ub;
    };
    struct var_bounds : public item_bounds
    {
      var_bounds(utils::enum_val &val) : val(val) {}
      utils::enum_val &val;
    };

    atom_adaptation(const utils::lit &sigma_xi) : sigma_xi(sigma_xi) {}

    utils::lit sigma_xi;
    std::unordered_map<riddle::item *, std::unique_ptr<item_bounds>> bounds;
  };

  class executor_theory : public semitone::theory
  {
    friend class executor;
    friend class solver;

  public:
    executor_theory(executor &exec) noexcept;

    executor &get_executor() noexcept { return exec; }

    void init() noexcept;

    void failure(const std::unordered_set<const ratio::atom *> &atoms);

  private:
    void new_atom(ratio::atom &atm);

    bool propagate_bounds(const riddle::item &itm, const atom_adaptation::item_bounds &bounds, const utils::lit &reason);

    bool propagate(const utils::lit &) noexcept override;
    bool check() noexcept override { return true; }
    void push() noexcept override {}
    void pop() noexcept override {}

  private:
    executor &exec;                                                       // the executor
    utils::lit xi;                                                        // the execution variable..
    std::unordered_map<utils::var, const ratio::atom *> all_atoms;        // all the interesting atoms indexed by their sigma_xi variable..
    std::unordered_map<const ratio::atom *, atom_adaptation> adaptations; // for each atom, the numeric adaptations done during the executions (i.e., freezes and delays)..
  };
} // namespace ratio::executor
