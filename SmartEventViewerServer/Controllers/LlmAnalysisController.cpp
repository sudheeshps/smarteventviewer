#include "LlmAnalysisController.h"
#include "Core/AnalysisService.h"
#include "Logging/AppLoggerManager.h"

namespace SmartEventViewer {
    LlmAnalysisController::LlmAnalysisController()
        : m_spAnalysisService(DotNetDupe::System::SmartPointer<IAnalysisService>(AnalysisService::GetSharedInstance())) {
    }

    LlmAnalysisController::LlmAnalysisController(const DotNetDupe::System::SmartPointer<IAnalysisService>& spService)
        : m_spAnalysisService(spService.IsNull() ? DotNetDupe::System::SmartPointer<IAnalysisService>(AnalysisService::GetSharedInstance()) : spService) {
    }

    AnalyzeResponseDto LlmAnalysisController::AnalyzeEvents(const AnalyzeRequestDto& request) {
        AppLoggerManager::Info("SERVER", String::Format("[LLM_CONTROLLER] Enqueueing analyze request for channel '{0}', query '{1}'", request.Channel, request.Query));
        return m_spAnalysisService->EnqueueTask(request);
    }

    AnalyzeResponseDto LlmAnalysisController::GetAnalyzeStatus(const DotNetDupe::System::String& sTaskId) {
        return m_spAnalysisService->GetTaskStatus(sTaskId);
    }

    AnalyzeResponseDto LlmAnalysisController::GetAnalyzeStatus() {
        String sTaskId;
        if (!m_httpContext.IsNull() && !Request().IsNull()) {
            Request()->GetQuery().TryGetValue("taskId", sTaskId);
        }
        return GetAnalyzeStatus(sTaskId);
    }
}
