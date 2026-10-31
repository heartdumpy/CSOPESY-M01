CSOPESY MO1 - Operating System Emulator

1. Project Overview

CSOPESY is a C++17 command-line operating system emulator. It supports
configuration loading, process creation and tracking, process screens,
multi-core CPU scheduling, CPU utilization reporting, and process status
display.

The scheduler supports FCFS and Round Robin (RR). Processes receive a
random instruction count within the configured minimum and maximum. The
scheduler simulates execution by advancing each process's instruction
counter.

2. Requirements

- C++17 compiler
- CMake 3.16 or later (recommended)
- Or a compatible compiler such as MinGW-w64 / MSYS2 GCC

3. Project Structure

CSOPESY-MO1/
|-- README.txt
|-- SOURCE/
|   |-- CMakeLists.txt
|   |-- main.cpp
|   |-- ConsoleManager.cpp / ConsoleManager.h
|   |-- CommandInterpreter.cpp / CommandInterpreter.h
|   |-- ScreenManager.cpp / ScreenManager.h
|   |-- Process.h
|   |-- ProcessManager.cpp / ProcessManager.h
|   |-- Scheduler.cpp / Scheduler.h
|   |-- ReportManager.cpp / ReportManager.h
|   |-- config.cpp / config.h
|   `-- config.txt
`-- tests/

4. Build and Run

Using CMake with MinGW or another supported toolchain:

1. Open a terminal in SOURCE.
2. Configure and build:

   cmake -S . -B build
   cmake --build build

3. Run the executable from the build directory:

   .\build\CSOPESY.exe

CMake copies config.txt into the build directory. If running an executable
built another way, make sure config.txt is in the program's current working
directory.

Alternatively, compile directly with GCC from SOURCE:

   g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp ConsoleManager.cpp CommandInterpreter.cpp ScreenManager.cpp config.cpp ProcessManager.cpp Scheduler.cpp ReportManager.cpp -o CSOPESY.exe

Then run:

   .\CSOPESY.exe

5. Configuration

The program reads config.txt from its current working directory. The supplied
configuration is:

   num-cpu 4
   scheduler "rr"
   quantum-cycles 5
   batch-process-freq 1
   min-ins 1000
   max-ins 2000
   delays-per-exec 0

All seven settings are required.

- num-cpu: Number of simulated CPU cores; must be between 1 and 128.
- scheduler: Scheduling algorithm; must be "fcfs" or "rr".
- quantum-cycles: RR time quantum; must be greater than zero.
- batch-process-freq: Must be greater than zero. It is loaded from the
  configuration but is not currently used to generate processes.
- min-ins: Minimum generated instruction count; must be greater than zero.
- max-ins: Maximum generated instruction count; must be at least min-ins.
- delays-per-exec: Scheduler-loop delay in milliseconds; zero is allowed.
  When set to zero, the scheduler uses a 1 ms delay.

6. Commands

After starting the program, enter initialize before using other commands.
Initialization loads and validates config.txt.

- initialize
  Loads and validates the configuration.
- screen -s <process name>
  Creates a process and opens its screen. Process names must be a single
  command-line token (no spaces).
- screen -r <process name>
  Reattaches to an existing process that has not terminated.
- screen -ls
  Lists registered processes and their states.
- scheduler-start
  Starts the scheduler using the loaded configuration.
- scheduler-stop
  Stops the scheduler. A process running on a core is returned to Ready.
- process-smi
  Displays simulated CPU utilization, core assignments, and process status.
- report-util
  Writes the utilization and process report to csopesy-log.txt in the
  current working directory. A new report replaces an existing file.
- exit
  Stops the scheduler and exits the program.

The scheduler does not start automatically. Processes can be created before
or after it starts.

7. Process Screen

Opening or reattaching to a process screen enters the PROCESS> prompt.
Commands available there are:

- process-smi
  Displays system CPU and process information.
- help
  Lists process-screen commands.
- exit
  Detaches from the screen without terminating or deleting the process.

A process screen does not terminate a process when detached. A terminated
process cannot be reopened or reattached.

8. Scheduling and Utilization Notes

FCFS processes continue on their assigned core until they finish. RR processes
are returned to Ready when they use the configured quantum and may then be
scheduled again.

CPU utilization is calculated from the scheduler's simulated active and idle
ticks. It is not a measurement of the computer's actual CPU usage.

9. Output

report-util generates csopesy-log.txt in the current working directory. The
report includes the generation time, simulated CPU utilization, core tick
counts and assignments, and registered process status and instruction
progress.