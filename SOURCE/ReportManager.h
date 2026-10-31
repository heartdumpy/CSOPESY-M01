#ifndef REPORT_MANAGER_H
#define REPORT_MANAGER_H

#include "Scheduler.h"
#include "ProcessManager.h"

#include <string>

class ReportManager {
public:
    bool generateReport(
        const Scheduler& scheduler,
        const ProcessManager& processManager,
        const std::string& filename,
        std::string& errorMessage
    ) const;
};

#endif
