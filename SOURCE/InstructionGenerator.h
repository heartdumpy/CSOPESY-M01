#ifndef INSTRUCTION_GENERATOR_H
#define INSTRUCTION_GENERATOR_H

#include "Instruction.h"

#include <random>

class InstructionGenerator {
public:
    explicit InstructionGenerator(
        std::uint64_t seed = std::random_device{}()
    );

    std::vector<Instruction> generate(
        const std::string& processName,
        std::uint64_t length
    );

private:
    std::mt19937_64 random;

    std::vector<Instruction> block(
        const std::string& processName,
        std::uint64_t budget,
        unsigned int depth
    );
};

#endif
