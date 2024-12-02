#include "executor_theory.hpp"
#include "executor.hpp"
#include "atom_flaw.hpp"
#include <cassert>

namespace ratio::executor
{
    solver::solver(executor_theory &exec, const std::string &name) : ratio::solver(name), exec(exec) {}

    void solver::flaw_created(const ratio::flaw &f)
    {
        if (const auto af = dynamic_cast<const ratio::atom_flaw *>(&f))
            exec.new_atom(static_cast<ratio::atom &>(*af->get_atom()));
    }

    executor_theory::executor_theory(executor &exec) noexcept : exec(exec), xi(exec.get_solver().get_sat().new_var()) {}

    void executor_theory::init() noexcept { bind(variable(xi)); }

    void executor_theory::failure(const std::unordered_set<const ratio::atom *> &atoms)
    {
        std::vector<utils::lit> cnfl;
        for (const auto &atm : atoms)
            cnfl.push_back(!atm->get_sigma());
        set_theory_conflict(std::move(cnfl));
        // we backtrack to a level at which we can analyze the conflict..
        if (!backtrack_analyze_and_backjump() || !exec.get_solver().solve())
            throw riddle::unsolvable_exception();
    }

    void executor_theory::new_atom(ratio::atom &atm)
    { // we create a new variable for propagating the execution constraints..
        const auto sigma_xi = exec.get_solver().get_sat().new_var();
        // we bind the sigma variable for propagating the bounds..
        bind(sigma_xi);
        all_atoms.emplace(sigma_xi, &atm);
        // either the atom is not active, or the xi variable is false, or the execution bounds must be enforced..
        [[maybe_unused]] bool nc = exec.get_solver().get_sat().new_clause({!atm.get_sigma(), !xi, utils::lit(sigma_xi)});
        assert(nc);
        auto [at_adapt, added] = adaptations.emplace(&atm, utils::lit(sigma_xi));

        if (is_impulse(atm))
        { // we create a new adaptation for the impulse atom..
            auto &xpr = *atm.get("at");
            at_adapt->second.bounds.emplace(&xpr, new atom_adaptation::arith_bounds(utils::inf_rational(exec.get_current_time()), utils::inf_rational(utils::rational::positive_infinite)));
        }
        else if (is_interval(atm))
        { // we create a new adaptation for the interval atom..
            auto &xpr = *atm.get("start");
            at_adapt->second.bounds.emplace(&xpr, new atom_adaptation::arith_bounds(utils::inf_rational(exec.get_current_time()), utils::inf_rational(utils::rational::positive_infinite)));
        }
    }

    bool executor_theory::propagate_bounds(const riddle::item &itm, const atom_adaptation::item_bounds &bounds, const utils::lit &reason)
    {
        if (const auto ba = dynamic_cast<const atom_adaptation::bool_bounds *>(&bounds))
        {
            const auto var = static_cast<const riddle::bool_item *>(&itm)->get_value();
            const auto val = exec.get_solver().get_sat().value(var);
            if (val == utils::Undefined)
                record({var, !reason});
            else if (val != ba->val)
            { // we have a conflict..
                std::vector<utils::lit> cnfl;
                cnfl.push_back(var);
                cnfl.push_back(!reason);
                set_theory_conflict(std::move(cnfl));
                return false;
            }
        }
        else if (const auto aa = dynamic_cast<const atom_adaptation::arith_bounds *>(&bounds))
        {
            if (static_cast<const riddle::arith_item &>(itm).get_value().vars.empty())
                return true; // we have a constant: nothing to propagate..
            if (is_real(itm))
            { // we have a real variable..
                auto val = static_cast<const riddle::arith_item &>(itm).get_value();
                if (!exec.get_solver().get_lra_theory().set_value(exec.get_solver().get_lra_theory().new_var(std::move(val)), aa->lb, {reason}))
                    return false;
            }
            else
                throw std::runtime_error("not implemented yet..");
        }
        else if (const auto va = dynamic_cast<const atom_adaptation::var_bounds *>(&bounds))
        {
            const auto var = static_cast<const riddle::enum_item &>(itm).get_value();
            const auto val = exec.get_solver().get_ov_theory().domain(var);
            if (val.size() > 1)
                record({exec.get_solver().get_ov_theory().allows(var, va->val), !reason});
            else if (&val.begin()->get() != &va->val)
            { // we have a conflict..
                std::vector<utils::lit> cnfl;
                cnfl.push_back(exec.get_solver().get_ov_theory().allows(var, va->val));
                cnfl.push_back(!reason);
                set_theory_conflict(std::move(cnfl));
                return false;
            }
        }
        return true;
    }

    bool executor_theory::propagate(const utils::lit &p) noexcept
    {
        if (p == xi)
        { // we propagate the active bounds..
            for (const auto &adapt : adaptations)
                if (exec.get_solver().get_sat().value(adapt.second.sigma_xi) == utils::True)
                    for (const auto &bnds : adapt.second.bounds)
                        if (!propagate_bounds(*bnds.first, *bnds.second, adapt.second.sigma_xi))
                            return false;
        }
        else if (exec.get_solver().get_sat().value(variable(p)) == utils::True)
        { // an atom has been activated..
            const auto atm = all_atoms.at(variable(p));
            const auto &adapt = adaptations.at(atm);
            for (const auto &bnds : adapt.bounds)
                if (!propagate_bounds(*bnds.first, *bnds.second, p))
                    return false;
        }
        return true;
    }
} // namespace ratio::executor
