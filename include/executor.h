#pragma once

#include "plexa_export.h"
#include "core_listener.h"
#include "solver_listener.h"
#include "solver.h"

namespace ratio::executor
{
  class executor_listener;

  struct atom_adaptation
  {
    struct item_bounds
    {
      virtual ~item_bounds() = default;
    };
    struct bool_bounds : public item_bounds
    {
      bool_bounds(const semitone::lbool &val) : val(val) {}
      const semitone::lbool val;
    };
    struct arith_bounds : public item_bounds
    {
      arith_bounds(const semitone::inf_rational &lb, const semitone::inf_rational &ub) : lb(lb), ub(ub) {}
      semitone::inf_rational lb, ub;
    };
    struct var_bounds : public item_bounds
    {
      var_bounds(semitone::var_value &val) : val(val) {}
      semitone::var_value &val;
    };

    atom_adaptation(const semitone::lit &sigma_xi) : sigma_xi(sigma_xi) {}

    semitone::lit sigma_xi;
    std::unordered_map<ratio::core::item *, std::unique_ptr<item_bounds>> bounds;
  };

  class executor final : public ratio::core::core_listener, public ratio::solver::solver_listener, public semitone::theory
  {
    friend class executor_listener;

  public:
    PLEXA_EXPORT executor(ratio::solver::solver &slv, const semitone::rational &units_per_tick = semitone::rational::ONE);
    executor(const executor &orig) = delete;

    ratio::solver::solver &get_solver() { return slv; }
    PLEXA_EXPORT const semitone::rational &get_current_time() const { return current_time; };
    PLEXA_EXPORT const semitone::rational &get_units_per_tick() const { return units_per_tick; };

    PLEXA_EXPORT void tick();

    PLEXA_EXPORT void dont_start_yet(const std::unordered_map<const ratio::core::atom *, semitone::rational> &atoms) { dont_start.insert(atoms.cbegin(), atoms.cend()); }
    PLEXA_EXPORT void dont_end_yet(const std::unordered_map<const ratio::core::atom *, semitone::rational> &atoms) { dont_end.insert(atoms.cbegin(), atoms.cend()); }
    PLEXA_EXPORT void failure(const std::unordered_set<ratio::core::atom *> &atoms);

  private:
    bool propagate(const semitone::lit &p) noexcept override;
    bool check() noexcept override { return true; }
    void push() noexcept override {}
    void pop() noexcept override {}

    inline bool is_relevant(const ratio::core::predicate &pred) const noexcept { return relevant_predicates.count(&pred); }

    void read(const std::string &) override { reset_relevant_predicates(); }
    void read(const std::vector<std::string> &) override { reset_relevant_predicates(); }
    void solution_found() override;
    void inconsistent_problem() override;

    void flaw_created(const ratio::solver::flaw &f) override;

    void build_timelines();
    bool propagate_bounds(const ratio::core::item &itm, const atom_adaptation::item_bounds &bounds, const semitone::lit &reason);

    void reset_relevant_predicates();

  private:
    std::unordered_set<const ratio::core::predicate *> relevant_predicates;                   // impulses and intervals..
    semitone::rational current_time;                                                          // the current time in plan units..
    const semitone::rational units_per_tick;                                                  // the number of plan units for each tick..
    semitone::lit xi;                                                                         // the execution variable..
    std::unordered_map<const ratio::core::atom *, atom_adaptation> adaptations;               // for each atom, the numeric adaptations done during the executions (i.e., freezes and delays)..
    std::unordered_map<semitone::var, ratio::core::atom *> all_atoms;                         // all the interesting atoms indexed by their sigma_xi variable..
    std::unordered_map<const ratio::core::atom *, semitone::rational> dont_start;             // the starting atoms which are not yet ready to start..
    std::unordered_map<const ratio::core::atom *, semitone::rational> dont_end;               // the ending atoms which are not yet ready to end..
    std::map<semitone::inf_rational, std::unordered_set<ratio::core::atom *>> s_atms, e_atms; // for each pulse, the atoms starting/ending at that pulse..
    std::set<semitone::inf_rational> pulses;                                                  // all the pulses of the plan..
    std::vector<executor_listener *> listeners;                                               // the executor listeners..
  };

  class execution_exception : public std::exception
  {
    const char *what() const noexcept override { return "the plan cannot be executed.."; }
  };
} // namespace ratio::executor