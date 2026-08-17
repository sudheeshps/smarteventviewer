#pragma once

#include "ViewerCommon.h"
#include "Core/IDiagnosticsService.h"

namespace SmartEventViewer {
    class SMARTEVENTVIEWER_API DiagnosticsService : public IDiagnosticsService {
    public:
        DiagnosticsService() = default;
        ~DiagnosticsService() override = default;

        LogFormatResponseDto GetLogFormat() override;
        ServerLogsResponseDto GetServerLogs(int nMaxLines = 500) override;
    };
}
