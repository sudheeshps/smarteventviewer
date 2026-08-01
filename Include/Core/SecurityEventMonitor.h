#pragma once

#include "../Common.h"
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;

    typedef void(*SecurityEventCallback)(const EventRecord& eventRec, RiskLevel eRisk);

    class SMARTEVENTVIEWER_API SecurityEventMonitor
    {
    private:
        bool m_bIsMonitoring{ false };
        SecurityEventCallback m_pCallback{ nullptr };

    public:
        SecurityEventMonitor();
        ~SecurityEventMonitor();

        void StartMonitoring(SecurityEventCallback pCallback);
        void StopMonitoring();
        bool IsMonitoring() const;
        void OnEventReceived(const EventRecord& eventRec);
    };
}
