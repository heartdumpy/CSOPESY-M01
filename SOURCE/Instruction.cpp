#include "Instruction.h"

#include <limits>
#include <stdexcept>

Instruction Instruction::print(
    std::string message,
    std::string variable
) {
    Instruction instruction;

    instruction.message = std::move(message);
    instruction.printVariable = std::move(variable);

    return instruction;
}

Instruction Instruction::declare(
    std::string variable,
    std::uint16_t value
) {
    Instruction instruction;

    instruction.type = InstructionType::DECLARE;
    instruction.destination = std::move(variable);
    instruction.initialValue = value;

    return instruction;
}

Instruction Instruction::add(
    std::string destination,
    Operand left,
    Operand right
) {
    Instruction instruction;

    instruction.type = InstructionType::ADD;
    instruction.destination = std::move(destination);
    instruction.left = std::move(left);
    instruction.right = std::move(right);

    return instruction;
}

Instruction Instruction::subtract(
    std::string destination,
    Operand left,
    Operand right
) {
    Instruction instruction = add(
        std::move(destination),
        std::move(left),
        std::move(right)
    );

    instruction.type = InstructionType::SUBTRACT;

    return instruction;
}

Instruction Instruction::sleep(std::uint8_t ticks) {
    Instruction instruction;

    instruction.type = InstructionType::SLEEP;
    instruction.sleepTicks = ticks;

    return instruction;
}

Instruction Instruction::loop(
    std::vector<Instruction> body,
    std::uint64_t repeats
) {
    Instruction instruction;

    instruction.type = InstructionType::FOR;
    instruction.body = std::move(body);
    instruction.repeats = repeats;

    return instruction;
}

namespace {

std::uint64_t countProgram(
    const std::vector<Instruction>& program,
    unsigned int depth
) {
    std::uint64_t total = 0;

    for (const Instruction& instruction : program) {
        std::uint64_t instructionCount = 1;

        switch (instruction.type) {
            case InstructionType::FOR: {
                if (depth >= 3) {
                    throw std::invalid_argument(
                        "FOR nesting exceeds three levels"
                    );
                }

                const std::uint64_t bodyCount = countProgram(
                    instruction.body,
                    depth + 1
                );

                if (
                    instruction.repeats != 0 &&
                    bodyCount >
                        std::numeric_limits<std::uint64_t>::max() /
                        instruction.repeats
                ) {
                    throw std::overflow_error(
                        "Instruction count overflow"
                    );
                }

                instructionCount =
                    bodyCount * instruction.repeats;

                break;
            }

            case InstructionType::DECLARE:
            case InstructionType::ADD:
            case InstructionType::SUBTRACT: {
                if (instruction.destination.empty()) {
                    throw std::invalid_argument(
                        "Empty variable name"
                    );
                }

                const bool invalidLeft =
                    instruction.left.isVariable &&
                    instruction.left.variable.empty();

                const bool invalidRight =
                    instruction.right.isVariable &&
                    instruction.right.variable.empty();

                if (invalidLeft || invalidRight) {
                    throw std::invalid_argument(
                        "Empty operand variable"
                    );
                }

                break;
            }

            case InstructionType::PRINT:
            case InstructionType::SLEEP:
                break;

            default:
                throw std::invalid_argument(
                    "Unknown instruction type"
                );
        }

        if (
            instructionCount >
                std::numeric_limits<std::uint64_t>::max() - total
        ) {
            throw std::overflow_error(
                "Instruction count overflow"
            );
        }

        total += instructionCount;
    }

    return total;
}

} // namespace

std::uint64_t countInstructions(
    const std::vector<Instruction>& program
) {
    return countProgram(program, 0);
}