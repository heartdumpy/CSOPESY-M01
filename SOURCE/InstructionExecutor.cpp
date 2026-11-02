#include "InstructionExecutor.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

// Removes completed frames or starts their next repetition.
void normalize(Process& process) {
    while (!process.loopStack.empty()) {
        LoopFrame& frame = process.loopStack.back();

        if (frame.index < frame.block->size()) {
            return;
        }

        if (frame.remaining > 1) {
            --frame.remaining;
            frame.index = 0;
        }
        else {
            process.loopStack.pop_back();
        }
    }
}

std::uint16_t readOperand(
    Process& process,
    const Operand& operand
) {
    if (operand.isVariable) {
        // operator[] creates an undeclared variable with value 0.
        return process.variables[operand.variable];
    }

    return operand.literal;
}

std::string createLogLine(
    const std::string& message,
    int coreId
) {
    const auto currentTime =
        std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()
        );

    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &currentTime);
#else
    localtime_r(&currentTime, &localTime);
#endif

    std::ostringstream output;

    output
        << '('
        << std::put_time(
            &localTime,
            "%m/%d/%Y %I:%M:%S%p"
        )
        << ") Core:"
        << coreId
        << " \""
        << message
        << '"';

    return output.str();
}

} // namespace

StepResult InstructionExecutor::executeStep(
    Process& process,
    std::uint64_t tick,
    int coreId
) {
    std::lock_guard<std::mutex> guard(process.mutex);

    if (process.state == ProcessState::TERMINATED) {
        return StepResult::FINISHED;
    }

    if (
        process.state == ProcessState::WAITING &&
        tick < process.wakeUpTick
    ) {
        return StepResult::SLEEPING;
    }

    if (!process.instructions) {
        throw std::logic_error(
            "Process has no program"
        );
    }

    normalize(process);

    const Instruction* nextInstruction = nullptr;

    // Traverse FOR metadata until one executable instruction is found.
    // The entire FOR body is never executed in one scheduler call.
    while (!process.loopStack.empty()) {
        LoopFrame& frame = process.loopStack.back();

        const Instruction& instruction =
            (*frame.block)[frame.index++];

        if (instruction.type == InstructionType::FOR) {
            const bool hasExecutableBody =
                instruction.repeats != 0 &&
                countInstructions(instruction.body) > 0;

            if (hasExecutableBody) {
                process.loopStack.push_back({
                    &instruction.body,
                    0,
                    instruction.repeats
                });
            }

            normalize(process);
        }
        else {
            nextInstruction = &instruction;
            break;
        }
    }

    if (nextInstruction == nullptr) {
        process.state = ProcessState::TERMINATED;
        process.assignedCore = -1;
        process.variables.clear();

        return StepResult::FINISHED;
    }

    process.state = ProcessState::RUNNING;
    process.assignedCore = coreId;

    const Instruction& instruction = *nextInstruction;

    switch (instruction.type) {
        case InstructionType::PRINT: {
            std::string message = instruction.message;

            if (!instruction.printVariable.empty()) {
                message += std::to_string(
                    process.variables[
                        instruction.printVariable
                    ]
                );
            }

            // Store privately. The screen decides when to display it.
            process.logs.push_back(
                createLogLine(message, coreId)
            );

            break;
        }

        case InstructionType::DECLARE: {
            process.variables[instruction.destination] =
                instruction.initialValue;

            break;
        }

        case InstructionType::ADD: {
            const std::uint16_t left =
                readOperand(process, instruction.left);

            const std::uint16_t right =
                readOperand(process, instruction.right);

            // Use a wider intermediate before clamping.
            const unsigned int result =
                static_cast<unsigned int>(left) + right;

            process.variables[instruction.destination] =
                static_cast<std::uint16_t>(
                    std::min(65535u, result)
                );

            break;
        }

        case InstructionType::SUBTRACT: {
            const int left =
                readOperand(process, instruction.left);

            const int right =
                readOperand(process, instruction.right);

            const int result = left - right;

            process.variables[instruction.destination] =
                static_cast<std::uint16_t>(
                    std::max(0, result)
                );

            break;
        }

        case InstructionType::SLEEP: {
            const std::uint64_t maximumTick =
                std::numeric_limits<std::uint64_t>::max();

            // Prevent unsigned overflow near the tick counter's limit.
            if (
                tick >
                maximumTick - instruction.sleepTicks
            ) {
                process.wakeUpTick = maximumTick;
            }
            else {
                process.wakeUpTick =
                    tick + instruction.sleepTicks;
            }

            process.state = ProcessState::WAITING;
            process.assignedCore = -1;

            break;
        }

        case InstructionType::FOR:
            throw std::logic_error(
                "Unexpected FOR dispatch"
            );
    }

    ++process.instructionPointer;

    normalize(process);

    if (
        process.instructionPointer ==
        process.totalInstructions
    ) {
        process.state = ProcessState::TERMINATED;
        process.assignedCore = -1;

        // Runtime variables remain until execution finishes.
        process.variables.clear();
        process.loopStack.clear();

        // Identity, instruction count and PRINT logs remain available.
        return StepResult::FINISHED;
    }

    if (process.state == ProcessState::WAITING) {
        return StepResult::SLEEPING;
    }

    return StepResult::CONTINUE;
}