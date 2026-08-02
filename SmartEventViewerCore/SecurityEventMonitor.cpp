#include "pch.h"
#include "../Include/Core/SecurityEventMonitor.h"

namespace SmartEventViewer
{
    SecurityEventMonitor::SecurityEventMonitor() = default;
    SecurityEventMonitor::~SecurityEventMonitor()
    {
        StopMonitoring();
    }

    void SecurityEventMonitor::StartMonitoring(SecurityEventCallback pCallback)
    {
        m_pCallback = pCallback;
        m_bIsMonitoring = true;
    }

    void SecurityEventMonitor::StopMonitoring()
    {
        m_bIsMonitoring = false;
        m_pCallback = nullptr;
    }

    bool SecurityEventMonitor::IsMonitoring() const
    {
        return m_bIsMonitoring;
    }

    void SecurityEventMonitor::OnEventReceived(const EventRecord& eventRec)
    {
        if (!m_bIsMonitoring || !m_pCallback) return;

        RiskLevel eRisk = AnomalyEngine::EvaluateRisk(eventRec);
        if (eRisk == RiskLevel::Critical || eRisk == RiskLevel::High)
        {
            m_pCallback(eventRec, eRisk);
        }
    }
}
