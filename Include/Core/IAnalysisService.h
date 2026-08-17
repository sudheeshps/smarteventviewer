#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "System/String.h"
#include "Dto/AnalysisDtos.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;

    class IAnalysisService : public virtual DotNetDupe::System::Object {
    public:
        virtual ~IAnalysisService() = default;

        virtual AnalyzeResponseDto EnqueueTask(const AnalyzeRequestDto& request) = 0;
        virtual AnalyzeResponseDto GetTaskStatus(const String& sTaskId) = 0;
        virtual void Shutdown() = 0;
    };
}
