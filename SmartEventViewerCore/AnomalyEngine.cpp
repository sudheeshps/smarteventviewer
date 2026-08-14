#include "pch.h"
#include "../Include/Core/AnomalyEngine.h"

namespace SmartEventViewer {
    RiskLevel AnomalyEngine::EvaluateRisk(const EventRecord& eventRec) {
        if (eventRec.GetEventId() == 4625) {
            return RiskLevel::High;
        }
        if (eventRec.GetEventId() == 7045) {
            return RiskLevel::Medium;
        }
        if (eventRec.GetLevel() == EventLevel::Critical || eventRec.GetLevel() == EventLevel::Error) {
            return RiskLevel::Medium;
        }
        return RiskLevel::Low;
    }
}
