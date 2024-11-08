#include "executor_theory.hpp"
#include "executor.hpp"

namespace ratio::executor
{
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
} // namespace ratio::executor
