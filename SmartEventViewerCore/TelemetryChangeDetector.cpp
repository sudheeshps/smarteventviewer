#include "pch.h"
#include "Core/TelemetryChangeDetector.h"
#include <cmath>

namespace SmartEventViewer {
    static bool IsCpuOrMemDiff(const SystemMetricsResponseDto& prev, const SystemMetricsResponseDto& cur) {
        if (std::abs(prev.CpuUsagePercent - cur.CpuUsagePercent) >= 0.5) return true;
        if (std::abs(prev.MemoryUsagePercent - cur.MemoryUsagePercent) >= 0.5) return true;
        if (prev.MemoryUsedMB != cur.MemoryUsedMB) return true;
        return (prev.DiskUsagePercent != cur.DiskUsagePercent || prev.NetworkUsageMbps != cur.NetworkUsageMbps);
    }

    bool TelemetryChangeDetector::HasSummaryChanged(const SystemMetricsResponseDto& current) {
        LockCS lock(m_csLock);
        if (!m_bHasInitialSummary) {
            m_prevSummary = current;
            m_bHasInitialSummary = true;
            return true;
        }
        if (IsCpuOrMemDiff(m_prevSummary, current)) {
            m_prevSummary = current;
            return true;
        }
        return false;
    }

    static bool IsProcMetricDiff(const ProcessResourceDto& a, const ProcessResourceDto& b) {
        if (std::abs(a.CpuUsagePercent - b.CpuUsagePercent) >= 0.2) return true;
        if (a.MemoryUsageMB != b.MemoryUsageMB) return true;
        if (a.NetworkReadBytes != b.NetworkReadBytes) return true;
        if (a.NetworkWriteBytes != b.NetworkWriteBytes) return true;
        return false;
    }

    static bool TryFindProcess(const DotNetDupe::System::Collections::Generic::List<ProcessResourceDto>& list,
                               unsigned long uPid, ProcessResourceDto& outProc) {
        for (int i = 0; i < list.GetCount(); ++i) {
            if (list[i].ProcessId == uPid) {
                outProc = list[i];
                return true;
            }
        }
        return false;
    }

    static bool AreProcessesEqual(const DotNetDupe::System::Collections::Generic::List<ProcessResourceDto>& a,
                                  const DotNetDupe::System::Collections::Generic::List<ProcessResourceDto>& b) {
        if (a.GetCount() != b.GetCount()) return false;
        for (int i = 0; i < a.GetCount(); ++i) {
            ProcessResourceDto match;
            if (!TryFindProcess(b, a[i].ProcessId, match)) return false;
            if (IsProcMetricDiff(a[i], match)) return false;
        }
        return true;
    }

    bool TelemetryChangeDetector::HaveProcessesChanged(const SystemMetricsResponseDto& current) {
        LockCS lock(m_csLock);
        if (!m_bHasInitialProcesses) {
            m_prevProcesses = current;
            m_bHasInitialProcesses = true;
            return true;
        }
        if (!AreProcessesEqual(m_prevProcesses.TopProcesses, current.TopProcesses)) {
            m_prevProcesses = current;
            return true;
        }
        return false;
    }

    bool TelemetryChangeDetector::HaveSessionsChanged(const SystemMetricsResponseDto& current) {
        LockCS lock(m_csLock);
        if (!m_bHasInitialSessions) {
            m_prevSessions = current;
            m_bHasInitialSessions = true;
            return true;
        }
        bool bChanged = (m_prevSessions.ActiveUserSessions.GetCount() != current.ActiveUserSessions.GetCount()) ||
                        (m_prevSessions.RdpSessions.GetCount() != current.RdpSessions.GetCount()) ||
                        (m_prevSessions.SystemUsers.GetCount() != current.SystemUsers.GetCount());
        if (bChanged) m_prevSessions = current;
        return bChanged;
    }

    bool TelemetryChangeDetector::HaveServicesChanged(const ServicesResponseDto& current) {
        LockCS lock(m_csLock);
        if (!m_bHasInitialServices) {
            m_prevServices = current;
            m_bHasInitialServices = true;
            return true;
        }
        if (m_prevServices.Services.GetCount() != current.Services.GetCount()) {
            m_prevServices = current;
            return true;
        }
        return false;
    }

    void TelemetryChangeDetector::Reset() {
        LockCS lock(m_csLock);
        m_bHasInitialSummary = false;
        m_bHasInitialProcesses = false;
        m_bHasInitialSessions = false;
        m_bHasInitialServices = false;
    }
}
