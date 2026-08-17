#pragma once

#include "ViewerCommon.h"
#include "Core/EventRecord.h"
#include "Core/IAnomalyEngine.h"

namespace SmartEventViewer {
    class SMARTEVENTVIEWER_API AnomalyEngine : public IAnomalyEngine {
    public:
        AnomalyEngine() = default;
        ~AnomalyEngine() override = default;

        RiskLevel EvaluateRisk(const EventRecord& eventRec) override;

        static RiskLevel StaticEvaluateRisk(const EventRecord& eventRec);
    };
}
