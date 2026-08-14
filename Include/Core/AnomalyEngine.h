#pragma once

#include "EventRecord.h"

namespace SmartEventViewer {
    class AnomalyEngine {
    public:
        SMARTEVENTVIEWER_API static RiskLevel EvaluateRisk(const EventRecord& eventRec);
    };
}
