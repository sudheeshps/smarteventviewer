#pragma once

#include "ViewerCommon.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "System/SmartPointer.h"
#include "Dto/DiagnosticsDtos.h"
#include "Core/IDiagnosticsService.h"

namespace SmartEventViewer {
    class SMARTEVENTVIEWER_API DiagnosticsController : public DotNetDupe::WebAppCore::Controllers::ControllerBase {
    private:
        DotNetDupe::System::SmartPointer<IDiagnosticsService> m_spDiagnosticsService{ nullptr };

    public:
        DiagnosticsController();
        explicit DiagnosticsController(const DotNetDupe::System::SmartPointer<IDiagnosticsService>& spService);
        ~DiagnosticsController() override = default;

        LogFormatResponseDto GetLogFormat();
        ServerLogsResponseDto GetServerLogs();
    };
}
