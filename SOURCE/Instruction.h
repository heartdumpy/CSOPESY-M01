#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR
};

struct Operand {
    std::string variable;
    std::uint16_t literal = 0;
    bool isVariable = false;

    static Operand value(std::uint16_t number) {
        return {"", number, false};
    }

    static Operand var(std::string name) {
        return {std::move(name), 0, true};
    }
};

// Programs become immutable after installation in a process.
// FOR stores its body once rather than expanding every repetition.
struct Instruction {
    InstructionType type = InstructionType::PRINT;

    std::string destination;
    std::string message;
    std::string printVariable;

    Operand left;
    Operand right;

    std::uint16_t initialValue = 0;
    std::uint8_t sleepTicks = 0;

    std::uint64_t repeats = 0;
    std::vector<Instruction> body;

    static Instruction print(
        std::string message,
        std::string variable = ""
    );

    static Instruction declare(
        std::string variable,
        std::uint16_t value
    );

    static Instruction add(
        std::string destination,
        Operand left,
        Operand right
    );

    static Instruction subtract(
        std::string destination,
        Operand left,
        Operand right
    );

    static Instruction sleep(std::uint8_t ticks);

    static Instruction loop(
        std::vector<Instruction> body,
        std::uint64_t repeats
    );
};

// Progress counts executed non-FOR instructions, including repetitions.
// FOR itself is control metadata.
//
// Also validates nesting and detects instruction-count overflow.
std::uint64_t countInstructions(
    const std::vector<Instruction>& program
);

#endif
