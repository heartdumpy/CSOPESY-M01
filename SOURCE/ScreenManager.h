#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "ProcessManager.h"

#include <memory>

class ScreenManager {
public:
    explicit ScreenManager(
        ProcessManager& manager
    );

    bool openScreen(
        const std::shared_ptr<Process>& process
    );

    bool reattachScreen(
        const std::shared_ptr<Process>& process
    );

    void displayScreen() const;

    void detachScreen();

    bool isAttached() const;

    std::shared_ptr<Process>
    getCurrentProcess() const;

private:
    ProcessManager& processManager;

    std::shared_ptr<Process> currentProcess = nullptr;

    bool attached = false;
};

#endif
