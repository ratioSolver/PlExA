#pragma once

#include "solver_api.hpp"
#include "executor_state.hpp"

namespace ratio::executor
{
  class executor;

  /**
   * Converts an executor_state enum value to its corresponding string representation.
   *
   * @param state The executor_state value to convert.
   * @return The string representation of the executor_state value.
   */
  [[nodiscard]] std::string to_string(const executor_state &state) noexcept;

  /**
   * @brief Creates a JSON message for an executor.
   *
   * This function creates a JSON message that represents an executor. The message includes the executor's ID, name, time, and state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the executor message.
   */
  [[nodiscard]] json::json to_json(const executor &exec) noexcept;

  /**
   * @brief Creates a JSON message for a new executor.
   *
   * This function creates a JSON message that represents a new executor. The message includes the executor's ID, name, and state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the new executor message.
   */
  [[nodiscard]] json::json make_new_solver_message(const executor &exec) noexcept;

  /**
   * @brief Creates a JSON message indicating that an executor has been deleted.
   *
   * This function creates a JSON message that indicates that an executor has been deleted. The message includes the ID of the deleted executor.
   *
   * @param id The ID of the deleted executor.
   * @return A JSON object representing the deleted executor message.
   */
  [[nodiscard]] json::json make_deleted_solver_message(const uintptr_t id) noexcept;

  /**
   * Creates a JSON message representing the state of an executor.
   *
   * @param exec The executor object.
   * @return A JSON object representing the state of the executor.
   */
  [[nodiscard]] json::json make_solver_state_message(const executor &exec) noexcept;

  /**
   * @brief Creates a JSON message indicating that the state of an executor has changed.
   *
   * This function creates a JSON message that indicates that the state of an executor has changed. The message includes the ID of the executor and its new state.
   *
   * @param exec The executor object.
   * @return A JSON object representing the state changed message.
   */
  [[nodiscard]] json::json make_solver_execution_state_changed_message(const executor &exec) noexcept;

  /**
   * Creates a JSON tick message.
   *
   * This function creates a JSON message of type "tick" that contains the solver ID and the current time.
   *
   * @param exec The executor object.
   * @return A JSON object representing the tick message.
   */
  [[nodiscard]] json::json make_tick_message(const executor &exec) noexcept;

  const json::json executor_schemas{
      {"solver",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "integer"}}},
          {"name", {{"type", "string"}}},
          {"time", {{"$ref", "#/components/schemas/rational"}}},
          {"state", {{"type", "string"}, {"enum", {"reasoning", "adapting", "idle", "executing", "finished", "failed"}}}}}}}}};

  const json::json executor_messages{
      {"new_solver_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"new_solver"}}}},
           {"id", {{"type", "integer"}}},
           {"name", {{"type", "string"}}},
           {"time", {{"$ref", "#/components/schemas/rational"}}},
           {"state", {{"type", "string"}, {"enum", {"reasoning", "adapting", "idle", "executing", "finished", "failed"}}}}}},
         {"required", {"id", "name", "state"}}}}},
      {"deleted_solver_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"deleted_solver"}}}},
           {"id", {{"type", "integer"}}}}}}}},
      {"solver_state_message",
       {"payload",
        {{"allOf",
          std::vector<json::json>{{"$ref", "#/components/schemas/solver_state"}}},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"solver_state"}}}},
           {"id", {{"type", "integer"}}},
           {"time", {{"$ref", "#/components/schemas/rational"}}},
           {"timelines", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/timeline"}}}}},
           {"executing_atoms", {{"type", "array"}, {"description", "The IDs of the atoms that are currently executing."}, {"items", {{"type", "integer"}}}}}}}}}},
      {"solver_execution_state_changed_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"solver_execution_state_changed"}}}},
           {"id", {{"type", "integer"}}},
           {"state", {{"type", "string"}, {"enum", {"reasoning", "adapting", "idle", "executing", "finished", "failed"}}}}}},
         {"required", std::vector<json::json>{"id", "state"}}}}},
      {"tick_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"tick"}}}},
           {"solver_id", {{"type", "integer"}}},
           {"time", {{"$ref", "#/components/schemas/rational"}}}}}}}}};
} // namespace ratio::executor
