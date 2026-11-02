#include "Process.h"

#include <stdexcept>

void Process::setProgram(
    std::vector<Instruction> program
) {
    const std::uint64_t total = countInstructions(program);

    if (total == 0) {
        throw std::invalid_argument(
            "A process needs executable instructions"
        );
    }

    auto storedProgram =
        std::make_shared<const std::vector<Instruction>>(
            std::move(program)
        );

    std::lock_guard<std::mutex> guard(mutex);

    if (
        state == ProcessState::RUNNING ||
        instructionPointer != 0 ||
        executionDelayInitialized
    ) {
        throw std::logic_error(
            "Cannot replace a started process program"
        );
    }

    instructions = std::move(storedProgram);
    totalInstructions = total;

    loopStack = {
        {instructions.get(), 0, 1}
    };

    variables.clear();
    logs.clear();

    wakeUpTick = 0;
    assignedCore = -1;

    executionDelayRemaining = 0;
    executionDelayInitialized = false;

    state = ProcessState::READY;
}

ProcessSnapshot Process::snapshot() const {
    std::lock_guard<std::mutex> guard(mutex);

    return {
        pid,
        name,
        creationTime,
        state,
        assignedCore,
        instructionPointer,
        totalInstructions,
        wakeUpTick,
        variables,
        logs
    };
}