#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "Core/EventRecord.h"

namespace SmartEventViewer {
    class IAnomalyEngine : public virtual DotNetDupe::System::Object {
    public:
        virtual ~IAnomalyEngine() = default;

        virtual RiskLevel EvaluateRisk(const EventRecord& eventRec) = 0;
    };
}
