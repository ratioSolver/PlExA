# PlExA

![Build Status](https://github.com/ratioSolver/PlExA/actions/workflows/cmake.yml/badge.svg)

Plan Executor and Adaptor (PlExA) is a library that provides a generic interface to execute plans. It is designed to be used in conjunction with [oRatio](https://github.com/ratioSolver/oRatio).

The following figure shows the possible state transitions of PlExA.

```mermaid
stateDiagram-v2
    direction LR
    [*] --> Reasoning
    Reasoning --> Idle
    Reasoning --> Failed
    Reasoning --> Finished
    Idle --> Adapting
    Idle --> Executing
    Idle --> [*]
    Adapting --> Idle
    Adapting --> Executing
    Adapting --> Failed
    Adapting --> Finished
    Adapting --> [*]
    Executing --> Adapting
    Executing --> Finished
    Executing --> [*]
    Finished --> Adapting
    Finished --> [*]
    Failed --> [*]
```
