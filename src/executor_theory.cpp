#include "executor_theory.hpp"
#include "executor.hpp"

namespace ratio::executor
{
    executor_theory::executor_theory(executor &exec) noexcept : exec(exec), xi(exec.get_solver().get_sat().new_var()) {}

    void executor_theory::init() noexcept
    {
        bind(variable(xi));
        exec.get_solver().init();
    }
} // namespace ratio::executor
