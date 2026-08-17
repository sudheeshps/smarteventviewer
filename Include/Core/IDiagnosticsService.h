#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "Dto/DiagnosticsDtos.h"

namespace SmartEventViewer {
    class IDiagnosticsService : public virtual DotNetDupe::System::Object {
    public:
        virtual ~IDiagnosticsService() = default;

        virtual LogFormatResponseDto GetLogFormat() = 0;
        virtual ServerLogsResponseDto GetServerLogs(int nMaxLines = 500) = 0;
    };
}
