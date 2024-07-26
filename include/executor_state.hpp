#pragma once

namespace ratio::executor
{
  enum executor_state
  {
    Reasoning,
    Idle,
    Adapting,
    Executing,
    Finished,
    Failed
  };
} // namespace ratio::executor
