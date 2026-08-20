#pragma once

#include "ViewerCommon.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "System/SmartPointer.h"
#include "Dto/AnalysisDtos.h"
#include "Core/IAnalysisService.h"

namespace SmartEventViewer {
    class SMARTEVENTVIEWER_API LlmAnalysisController : public DotNetDupe::WebAppCore::Controllers::ControllerBase {
    private:
        DotNetDupe::System::SmartPointer<IAnalysisService> m_spAnalysisService{ nullptr };

    public:
        LlmAnalysisController();
        explicit LlmAnalysisController(const DotNetDupe::System::SmartPointer<IAnalysisService>& spService);
        ~LlmAnalysisController() override = default;

        AnalyzeResponseDto AnalyzeEvents(const AnalyzeRequestDto& request);
        AnalyzeResponseDto GetAnalyzeStatus(const DotNetDupe::System::String& sTaskId);
        AnalyzeResponseDto GetAnalyzeStatus();
    };
}
