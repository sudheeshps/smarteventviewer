#pragma once

#include "Core/ISystemTelemetryProvider.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"

namespace SmartEventViewer {
    namespace Tests {
        using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
        using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

        class MockSystemTelemetryProvider : public ISystemTelemetryProvider {
        private:
            SystemMetricsResponseDto m_metrics{};
            ServicesResponseDto m_services{};
            mutable CriticalSection m_csLock{};

        public:
            MockSystemTelemetryProvider() {
                m_metrics.CpuUsagePercent = 12.5;
                m_metrics.MemoryUsagePercent = 45.0;
                m_metrics.MemoryUsedMB = 7372;
                m_metrics.MemoryTotalMB = 16384;
                m_metrics.DiskUsagePercent = 55.0;
                m_metrics.DiskReadMBps = 1.2;
                m_metrics.DiskWriteMBps = 3.4;
                m_metrics.NetworkUsageMbps = 15.6;
            }
            ~MockSystemTelemetryProvider() override = default;

            SystemMetricsResponseDto QuerySummary() override {
                LockCS lock(m_csLock);
                SystemMetricsResponseDto dto;
                dto.CpuUsagePercent = m_metrics.CpuUsagePercent;
                dto.MemoryUsagePercent = m_metrics.MemoryUsagePercent;
                dto.MemoryUsedMB = m_metrics.MemoryUsedMB;
                dto.MemoryTotalMB = m_metrics.MemoryTotalMB;
                dto.DiskUsagePercent = m_metrics.DiskUsagePercent;
                dto.DiskReadMBps = m_metrics.DiskReadMBps;
                dto.DiskWriteMBps = m_metrics.DiskWriteMBps;
                dto.NetworkUsageMbps = m_metrics.NetworkUsageMbps;
                return dto;
            }

            SystemMetricsResponseDto QueryCpuUsage() override {
                return QuerySummary();
            }

            SystemMetricsResponseDto QueryMemoryUsage() override {
                return QuerySummary();
            }

            SystemMetricsResponseDto QueryDiskUsage() override {
                return QuerySummary();
            }

            SystemMetricsResponseDto QueryNetworkUsage() override {
                return QuerySummary();
            }

            SystemMetricsResponseDto QueryProcesses() override {
                LockCS lock(m_csLock);
                SystemMetricsResponseDto dto;
                dto.TopProcesses = m_metrics.TopProcesses;
                return dto;
            }

            SystemMetricsResponseDto QuerySessions() override {
                LockCS lock(m_csLock);
                SystemMetricsResponseDto dto;
                dto.ActiveUserSessions = m_metrics.ActiveUserSessions;
                dto.ExpiredUserSessions = m_metrics.ExpiredUserSessions;
                dto.SystemUsers = m_metrics.SystemUsers;
                dto.RdpSessions = m_metrics.RdpSessions;
                return dto;
            }

            ServicesResponseDto QueryServices() override {
                LockCS lock(m_csLock);
                return m_services;
            }

            SystemMetricsResponseDto QuerySystemMetrics() override {
                LockCS lock(m_csLock);
                return m_metrics;
            }

            void SetMetrics(const SystemMetricsResponseDto& metrics) {
                LockCS lock(m_csLock);
                m_metrics = metrics;
            }

            void SetCpuUsage(double dCpuPercent) {
                LockCS lock(m_csLock);
                m_metrics.CpuUsagePercent = dCpuPercent;
            }

            void SetMemoryUsage(double dMemPercent, unsigned long long uUsedMb, unsigned long long uTotalMb) {
                LockCS lock(m_csLock);
                m_metrics.MemoryUsagePercent = dMemPercent;
                m_metrics.MemoryUsedMB = uUsedMb;
                m_metrics.MemoryTotalMB = uTotalMb;
            }

            void AddProcess(const ProcessResourceDto& process) {
                LockCS lock(m_csLock);
                m_metrics.TopProcesses.Add(process);
            }

            void AddActiveSession(const UserSessionDto& session) {
                LockCS lock(m_csLock);
                m_metrics.ActiveUserSessions.Add(session);
            }

            void AddService(const ServiceInfoDto& service) {
                LockCS lock(m_csLock);
                m_services.Services.Add(service);
            }

            void ClearProcesses() {
                LockCS lock(m_csLock);
                m_metrics.TopProcesses.Clear();
            }
        };
    }
}
