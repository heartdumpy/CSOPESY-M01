#ifndef REPORT_MANAGER_H
#define REPORT_MANAGER_H

#include "Scheduler.h"

class ReportManager {
public:
    // Shared by screen -ls and report-util.
    std::string formatReport(
        const SystemSnapshot& system
    ) const;

    bool generateReport(
        const Scheduler& scheduler,
        const ProcessManager& manager,
        const std::string& filename,
        std::string& errorMessage
    ) const;
};

#endif
