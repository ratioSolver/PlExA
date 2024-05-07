#include "executor.hpp"
#include "executor_theory.hpp"
#include "logging.hpp"

namespace ratio::executor
{
    executor::executor(std::shared_ptr<ratio::solver> slv, const utils::rational &units_per_tick) noexcept : slv(slv), exec_theory(slv->get_sat().new_theory<executor_theory>(*this)), units_per_tick(units_per_tick) {}

    void executor::init()
    {
        exec_theory.init();
        slv->init();
    }

    void executor::adapt(const std::string &script)
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        while (!slv->get_sat().root_level()) // we go at root level..
            slv->get_sat().pop();
        slv->read(script);
        pending_requirements = true;
    }
    void executor::adapt(const std::vector<std::string> &scripts)
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        while (!slv->get_sat().root_level()) // we go at root level..
            slv->get_sat().pop();
        slv->read(scripts);
        pending_requirements = true;
    }

    void executor::start()
    {
        running = true;
        executor_state_changed(state = executor_state::Executing);
    }

    void executor::pause()
    {
        running = false;
        executor_state_changed(state = executor_state::Idle);
    }

    void executor::tick()
    {
#ifdef MULTIPLE_EXECUTORS
        const std::lock_guard<std::mutex> lock(mtx);
#endif
        if (pending_requirements)
        { // we solve the problem..
            executor_state_changed(state = running ? executor_state::Adapting : executor_state::Reasoning);
            if (slv->solve()) // we have a solution..
                executor_state_changed(state = running ? executor_state::Executing : executor_state::Idle);
            else // we have no solution..
                executor_state_changed(state = executor_state::Failed);
            pending_requirements = false;
        }

        if (!running)
            return; // if not running, do nothing..
    }

    void executor::executor_state_changed([[maybe_unused]] executor_state state) { LOG_DEBUG("[" << slv->get_name() << "] executor is now " << state); }
    void executor::tick([[maybe_unused]] const utils::rational &time) { LOG_DEBUG("[" << slv->get_name() << "] current time is " << to_string(time)); }
    void executor::starting([[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] starting " << atms.size() << " atoms"); }
    void executor::start([[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] started " << atms.size() << " atoms"); }
    void executor::ending([[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] ending " << atms.size() << " atoms"); }
    void executor::end([[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] ended " << atms.size() << " atoms"); }
} // namespace ratio::executor