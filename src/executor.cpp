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

    void executor::executor_state_changed([[maybe_unused]] executor_state state) { LOG_DEBUG("[" << slv->get_name() << "] executor is now " << state); }
    void executor::tick([[maybe_unused]] const utils::rational &time) { LOG_DEBUG("[" << slv->get_name() << "] current time is " << to_string(time)); }
    void executor::starting([[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] starting " << atms.size() << " atoms"); }
    void executor::start([[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] started " << atms.size() << " atoms"); }
    void executor::ending([[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] ending " << atms.size() << " atoms"); }
    void executor::end([[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atms) { LOG_DEBUG("[" << slv->get_name() << "] ended " << atms.size() << " atoms"); }
} // namespace ratio::executor