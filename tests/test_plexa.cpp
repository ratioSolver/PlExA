#if defined(SEMITONE)
#include "stexecutor.hpp"
#elif defined(MathSAT)
#include "msatexecutor.hpp"
#elif defined(Z3)
#include "z3executor.hpp"
#endif
#include <cassert>

void test_basic_exec()
{
#if defined(SEMITONE)
    ratio::executor::executor exec;
#elif defined(MathSAT)
    ratio::executor::msatexecutor slv;
#elif defined(Z3)
    ratio::executor::z3executor slv;
#endif
}

int main()
{
    test_basic_exec();

    return 0;
}
