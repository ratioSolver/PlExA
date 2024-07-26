#include "executor_api.hpp"
#include "executor.hpp"

namespace ratio::executor
{
    [[nodiscard]] std::string to_string(const executor_state &state) noexcept
    {
        switch (state)
        {
        case Reasoning:
            return "reasoning";
        case Idle:
            return "idle";
        case Adapting:
            return "adapting";
        case Executing:
            return "executing";
        case Finished:
            return "finished";
        case Failed:
            return "failed";
        default: // should never happen..
            return "unknown";
        }
    }

    [[nodiscard]] json::json to_json(const executor &exec) noexcept { return {{"id", get_id(exec.get_solver())}, {"name", exec.get_solver().get_name()}, {"time", ratio::to_json(exec.get_current_time())}, {"state", to_string(exec.get_state())}}; }

    [[nodiscard]] json::json make_new_solver_message(const executor &exec) noexcept
    {
        auto exec_msg = to_json(exec);
        exec_msg["type"] = "new_solver";
        return exec_msg;
    }

    [[nodiscard]] json::json make_deleted_solver_message(const uintptr_t id) noexcept { return {{"type", "deleted_solver"}, {"id", id}}; }

    [[nodiscard]] json::json make_solver_execution_state_changed_message(const executor &exec) noexcept { return {{"type", "solver_execution_state_changed"}, {"id", get_id(exec.get_solver())}, {"state", to_string(exec.get_state())}}; }

    [[nodiscard]] json::json make_solver_state_message(const executor &exec) noexcept
    {
        auto state_msg = to_json(exec.get_solver());
        state_msg["type"] = "solver_state";
        state_msg["id"] = get_id(exec.get_solver());
        state_msg["time"] = ratio::to_json(exec.get_current_time());
        auto timelines = to_timelines(exec.get_solver());
        if (!timelines.as_array().empty())
            state_msg["timelines"] = std::move(timelines);
        json::json executing_atoms(json::json_type::array);
        for (const auto &atm : exec.get_executing_atoms())
            executing_atoms.push_back(get_id(atm.get()));
        if (!executing_atoms.as_array().empty())
            state_msg["executing_atoms"] = std::move(executing_atoms);
        return state_msg;
    }

    [[nodiscard]] json::json make_tick_message(const executor &exec) noexcept { return {{"type", "tick"}, {"solver_id", get_id(exec.get_solver())}, {"time", ratio::to_json(exec.get_current_time())}}; }
} // namespace ratio::executor
