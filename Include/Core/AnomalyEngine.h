#pragma once

#include "EventRecord.h"

namespace SmartEventViewer
{
    class SMARTEVENTVIEWER_API AnomalyEngine
    {
    public:
        static RiskLevel EvaluateRisk(const EventRecord& eventRec);
    };
}
