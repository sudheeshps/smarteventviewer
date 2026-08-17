#include "DiagnosticsController.h"
#include "Core/DiagnosticsService.h"

namespace SmartEventViewer {
    DiagnosticsController::DiagnosticsController()
        : m_spDiagnosticsService(DotNetDupe::System::SmartPointer<DiagnosticsService>::NewShared()) {
    }

    DiagnosticsController::DiagnosticsController(const DotNetDupe::System::SmartPointer<IDiagnosticsService>& spService)
        : m_spDiagnosticsService(spService.IsNull() ? DotNetDupe::System::SmartPointer<IDiagnosticsService>(DotNetDupe::System::SmartPointer<DiagnosticsService>::NewShared()) : spService) {
    }

    LogFormatResponseDto DiagnosticsController::GetLogFormat() {
        return m_spDiagnosticsService->GetLogFormat();
    }

    ServerLogsResponseDto DiagnosticsController::GetServerLogs() {
        return m_spDiagnosticsService->GetServerLogs(500);
    }
}
