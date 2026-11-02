#ifndef INSTRUCTION_EXECUTOR_H
#define INSTRUCTION_EXECUTOR_H

#include "Process.h"

enum class StepResult {
    CONTINUE,
    SLEEPING,
    FINISHED
};

class InstructionExecutor {
public:
    // Executes at most one non-FOR instruction.
    //
    // tick: current global CPU tick.
    // coreId: CPU core executing this instruction.
    //
    // Locks process.mutex internally. The caller must not already
    // hold that same non-recursive mutex.
    static StepResult executeStep(
        Process& process,
        std::uint64_t tick,
        int coreId
    );
};

#endif
