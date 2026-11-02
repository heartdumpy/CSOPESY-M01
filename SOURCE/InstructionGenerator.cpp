#include "InstructionGenerator.h"

#include <algorithm>
#include <stdexcept>

InstructionGenerator::InstructionGenerator(
    std::uint64_t seed
)
    : random(seed) {
}

std::vector<Instruction> InstructionGenerator::generate(
    const std::string& processName,
    std::uint64_t length
) {
    if (
        length == 0 ||
        length > 4294967296ULL
    ) {
        throw std::invalid_argument(
            "Instruction length out of range"
        );
    }

    // Avoid allocating billions of instruction nodes.
    // The body is randomized once, before execution.
    constexpr std::uint64_t chunkSize = 256;

    if (length > chunkSize) {
        std::vector<Instruction> program{
            Instruction::loop(
                block(processName, chunkSize, 1),
                length / chunkSize
            )
        };

        const std::uint64_t remainder =
            length % chunkSize;

        if (remainder != 0) {
            std::vector<Instruction> tail = block(
                processName,
                remainder,
                0
            );

            program.insert(
                program.end(),
                tail.begin(),
                tail.end()
            );
        }

        return program;
    }

    return block(processName, length, 0);
}

std::vector<Instruction> InstructionGenerator::block(
    const std::string& processName,
    std::uint64_t budget,
    unsigned int depth
) {
    std::vector<Instruction> program;

    while (budget != 0) {
        const auto type = random() % 6;

        const std::string destination =
            "v" + std::to_string(random() % 8);

        auto generateOperand = [&]() -> Operand {
            if (random() % 2 != 0) {
                return Operand::var(
                    "v" + std::to_string(random() % 8)
                );
            }

            return Operand::value(
                static_cast<std::uint16_t>(
                    random() % 65536
                )
            );
        };

        if (
            type == 5 &&
            depth < 3 &&
            budget >= 2
        ) {
            const std::uint64_t repeats =
                2 + random() % 3;

            if (budget >= repeats) {
                const std::uint64_t bodySize =
                    std::min<std::uint64_t>(
                        budget / repeats,
                        1 + random() % 8
                    );

                program.push_back(
                    Instruction::loop(
                        block(
                            processName,
                            bodySize,
                            depth + 1
                        ),
                        repeats
                    )
                );

                budget -= bodySize * repeats;
                continue;
            }
        }

        switch (type) {
            case 1: {
                program.push_back(
                    Instruction::declare(
                        destination,
                        static_cast<std::uint16_t>(
                            random() % 65536
                        )
                    )
                );

                break;
            }

            case 2: {
                Operand left = generateOperand();
                Operand right = generateOperand();

                program.push_back(
                    Instruction::add(
                        destination,
                        left,
                        right
                    )
                );

                break;
            }

            case 3: {
                Operand left = generateOperand();
                Operand right = generateOperand();

                program.push_back(
                    Instruction::subtract(
                        destination,
                        left,
                        right
                    )
                );

                break;
            }

            case 4: {
                program.push_back(
                    Instruction::sleep(
                        static_cast<std::uint8_t>(
                            random() % 6
                        )
                    )
                );

                break;
            }

            default: {
                program.push_back(
                    Instruction::print(
                        "Hello world from " +
                        processName +
                        "!"
                    )
                );

                break;
            }
        }

        --budget;
    }

    return program;
}