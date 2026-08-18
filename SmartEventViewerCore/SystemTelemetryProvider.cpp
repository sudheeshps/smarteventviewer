#include "pch.h"
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "Core/SystemTelemetryProvider.h"
#include "Dto/TelemetryDtos.h"
#include "System/Console.h"
#include "System/DateTime.h"
#include "System/Diagnostics/SystemMetrics.h"
#include "System/Diagnostics/ProcessStreamer.h"
#include "System/Diagnostics/ProcessStreamOptions.h"
#include "System/Diagnostics/ActiveUserSession.h"
#include "System/Diagnostics/TerminalSession.h"
#include "System/Security/Principal/UserPrincipal.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/Collections/Generic/Dictionary.h"

namespace SmartEventViewer {
    using Console = DotNetDupe::System::Console;
    using SystemMetrics = DotNetDupe::System::Diagnostics::SystemMetrics;
    using ProcessStreamer = DotNetDupe::System::Diagnostics::ProcessStreamer;
    using ProcessStreamOptions = DotNetDupe::System::Diagnostics::ProcessStreamOptions;
    using ProcessMetricsDetail = DotNetDupe::System::Diagnostics::ProcessMetricsDetail;
    using ActiveUserSession = DotNetDupe::System::Diagnostics::ActiveUserSession;
    using TerminalSession = DotNetDupe::System::Diagnostics::TerminalSession;
    using RdpSessionState = DotNetDupe::System::Diagnostics::RdpSessionState;
    using UserPrincipal = DotNetDupe::System::Security::Principal::UserPrincipal;
    using UserClass = DotNetDupe::System::Security::Principal::UserClass;
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    static DotNetDupe::System::Collections::Generic::Dictionary<unsigned long, ProcessResourceDto> s_processCache;
    static CriticalSection s_processCacheCs;
    static DotNetDupe::System::SmartPointer<ProcessStreamer> s_pProcessStreamer = nullptr;
    static unsigned long long s_lastProcessStreamStartMs = 0;

    static unsigned long long GetTickMs() {
#if defined(_WIN32) || defined(_WIN64)
        return static_cast<unsigned long long>(GetTickCount64());
#else
        return static_cast<unsigned long long>(DotNetDupe::System::DateTime::UtcNow().GetTicks() / 10000);
#endif
    }

    static void UpdateBatchInCache(const DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::Diagnostics::ProcessInfo>& batch) {
        LockCS lock(s_processCacheCs);
        for (int i = 0; i < batch.GetCount(); ++i) {
            auto dto = SystemTelemetryProvider::MapProcessResourceDto(batch[i]);
            s_processCache[dto.ProcessId] = dto;
        }
    }

    static void EnsureProcessStreamerActive() {
        LockCS lock(s_processCacheCs);
        unsigned long long cur = GetTickMs();
        if (!s_pProcessStreamer.IsNull() && (s_pProcessStreamer->IsRunning() || cur - s_lastProcessStreamStartMs < 2500)) return;
        s_lastProcessStreamStartMs = cur;
        ProcessStreamOptions options;
        options.eDetailLevel = ProcessMetricsDetail::Progressive;
        options.iBatchSize = 25;
        options.iBatchIntervalMs = 50;
        options.bIncludeNetworkInfo = true;

        s_pProcessStreamer = DotNetDupe::System::SmartPointer<ProcessStreamer>::NewShared(options);
        s_pProcessStreamer->OnBatch([](const DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::Diagnostics::ProcessInfo>& batch) {
            UpdateBatchInCache(batch);
        });
        s_pProcessStreamer->OnProcessUpdated([](const DotNetDupe::System::Diagnostics::ProcessInfo& proc) {
            LockCS innerLock(s_processCacheCs);
            auto dto = SystemTelemetryProvider::MapProcessResourceDto(proc);
            s_processCache[dto.ProcessId] = dto;
        });
        s_pProcessStreamer->Start();
    }

    void SystemTelemetryProvider::CalculateDiskRates(const DotNetDupe::System::Diagnostics::DiskInfo& diskInfo, double& outReadMb, double& outWriteMb) {
        static uint64_t s_lastDiskReadBytes = 0;
        static uint64_t s_lastDiskWriteBytes = 0;

        uint64_t curRead = static_cast<uint64_t>(diskInfo.lDiskReadBytes > 0 ? diskInfo.lDiskReadBytes : 0);
        uint64_t curWrite = static_cast<uint64_t>(diskInfo.lDiskWriteBytes > 0 ? diskInfo.lDiskWriteBytes : 0);
        if (s_lastDiskReadBytes == 0 && s_lastDiskWriteBytes == 0) {
            s_lastDiskReadBytes = curRead;
            s_lastDiskWriteBytes = curWrite;
        }
        uint64_t deltaRead = (curRead >= s_lastDiskReadBytes) ? (curRead - s_lastDiskReadBytes) : 0;
        uint64_t deltaWrite = (curWrite >= s_lastDiskWriteBytes) ? (curWrite - s_lastDiskWriteBytes) : 0;
        s_lastDiskReadBytes = curRead;
        s_lastDiskWriteBytes = curWrite;
        outReadMb = static_cast<double>(deltaRead) / (1024.0 * 1024.0);
        outWriteMb = static_cast<double>(deltaWrite) / (1024.0 * 1024.0);
    }

    String SystemTelemetryProvider::FormatCommandLine(const String& sPath, const String& sCmd) {
        if (sCmd.IsEmpty()) return String("-");
        try {
            String sResult = sCmd;
            int iLen = sResult.GetLength();
            if (!sPath.IsEmpty() && sResult.StartsWith(sPath) && sPath.GetLength() < iLen) {
                sResult = sResult.Substring(static_cast<int>(sPath.GetLength()));
            }
            sResult = sResult.Trim();
            if (sResult.IsEmpty()) return String("-");
            return sResult;
        } catch (...) {
            return String("-");
        }
    }

    ProcessResourceDto SystemTelemetryProvider::MapProcessResourceDto(const DotNetDupe::System::Diagnostics::ProcessInfo& proc) {
        ProcessResourceDto procDto;
        try {
            procDto.ProcessId = static_cast<unsigned long>(proc.iProcessId);
            procDto.Name = proc.sName.IsEmpty() ? String("System Process") : proc.sName;
            procDto.Path = proc.sPath.IsEmpty() ? String("System Protected") : proc.sPath;
            procDto.CommandLine = FormatCommandLine(proc.sPath, proc.sCommandLine);
            double procCpu = proc.dCpuUsagePercent;
            procDto.CpuUsagePercent = procCpu < 0.0 ? 0.0 : (procCpu > 100.0 ? 100.0 : procCpu);
            long long rawRamBytes = proc.memory.lPhysicalMemoryBytes > 0 ? proc.memory.lPhysicalMemoryBytes : proc.memory.lPrivateBytes;
            procDto.MemoryUsageMB = static_cast<unsigned long long>(rawRamBytes > 0 ? (rawRamBytes / (1024 * 1024)) : 0);
            procDto.NetworkReadBytes = static_cast<unsigned long long>(proc.network.lNetworkReadBytes > 0 ? proc.network.lNetworkReadBytes : 0);
            procDto.NetworkWriteBytes = static_cast<unsigned long long>(proc.network.lNetworkWriteBytes > 0 ? proc.network.lNetworkWriteBytes : 0);
            procDto.OpenPorts = String("-");
            procDto.ConnectionEstablished = false;
        } catch (...) {
        }
        return procDto;
    }

    void SystemTelemetryProvider::PopulateUserSessions(SystemMetricsResponseDto& metrics) {
        auto activeSessions = ActiveUserSession::GetActiveSessions();
        for (int i = 0; i < activeSessions.GetCount(); ++i) {
            const auto& s = activeSessions[i];
            UserSessionDto dto;
            dto.Username = s.sUsername;
            dto.Privilege = s.sPrivilege;
            dto.LoginTimestamp = s.sLoginTimestamp;
            dto.LogoutTimestamp = s.sLogoutTimestamp;
            dto.IsActive = s.bIsActive;
            metrics.ActiveUserSessions.Add(dto);
        }

        auto expiredSessions = ActiveUserSession::GetExpiredSessions();
        for (int i = 0; i < expiredSessions.GetCount(); ++i) {
            const auto& s = expiredSessions[i];
            UserSessionDto dto;
            dto.Username = s.sUsername;
            dto.Privilege = s.sPrivilege;
            dto.LoginTimestamp = s.sLoginTimestamp;
            dto.LogoutTimestamp = s.sLogoutTimestamp;
            dto.IsActive = s.bIsActive;
            metrics.ExpiredUserSessions.Add(dto);
        }

        auto users = UserPrincipal::EnumerateUsers();
        for (int i = 0; i < users.GetCount(); ++i) {
            const auto& u = users[i];
            UserPrincipalDto dto;
            dto.Username = u.sUsername;
            dto.Domain = u.sDomain;
            dto.SidOrUid = u.sSidOrUid;
            if (u.eUserClass == UserClass::Admin) dto.UserClass = "Admin";
            else if (u.eUserClass == UserClass::System) dto.UserClass = "System";
            else if (u.eUserClass == UserClass::Guest) dto.UserClass = "Guest";
            else dto.UserClass = "Normal";
            dto.IsDisabled = u.bIsDisabled;
            dto.IsAccountLocked = u.bIsAccountLocked;
            dto.Groups = u.lstGroups;
            dto.Permissions = u.lstPermissions;
            metrics.SystemUsers.Add(dto);
        }

        auto rdpList = TerminalSession::GetSessions();
        for (int i = 0; i < rdpList.GetCount(); ++i) {
            const auto& r = rdpList[i];
            RdpSessionDto dto;
            dto.SessionId = r.uSessionId;
            dto.SessionName = r.sSessionName;
            dto.UserName = r.sUserName;
            dto.DomainName = r.sDomainName;
            dto.ClientName = r.sClientName;
            dto.ClientIpAddress = r.sClientIpAddress;
            dto.IsRdpSession = r.bIsRdpSession;

            switch (r.eState) {
                case RdpSessionState::Active: dto.State = "Active"; break;
                case RdpSessionState::Connected: dto.State = "Connected"; break;
                case RdpSessionState::Disconnected: dto.State = "Disconnected"; break;
                case RdpSessionState::Idle: dto.State = "Idle"; break;
                case RdpSessionState::Listen: dto.State = "Listen"; break;
                case RdpSessionState::Shadow: dto.State = "Shadow"; break;
                case RdpSessionState::Reset: dto.State = "Reset"; break;
                case RdpSessionState::Down: dto.State = "Down"; break;
                case RdpSessionState::Init: dto.State = "Init"; break;
                default: dto.State = "Unknown"; break;
            }
            metrics.RdpSessions.Add(dto);
        }
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QuerySummary() {
        EnsureProcessStreamerActive();
        SystemMetricsResponseDto metrics;
        try {
            double sysCpu = SystemMetrics::GetSystemCpuUsage();
            metrics.CpuUsagePercent = (sysCpu < 0.0) ? 0.0 : ((sysCpu > 100.0) ? 100.0 : sysCpu);

            auto memInfo = SystemMetrics::GetSystemMemoryUsage();
            metrics.MemoryUsagePercent = memInfo.dMemoryUsagePercent;
            metrics.MemoryTotalMB = memInfo.uMemoryTotalBytes / (1024 * 1024);
            metrics.MemoryUsedMB = memInfo.uMemoryUsedBytes / (1024 * 1024);

            auto diskInfo = SystemMetrics::GetSystemDiskUsage();
            double calculatedReadMb = 0.0, calculatedWriteMb = 0.0;
            CalculateDiskRates(diskInfo, calculatedReadMb, calculatedWriteMb);
            metrics.DiskReadMBps = calculatedReadMb;
            metrics.DiskWriteMBps = calculatedWriteMb;
            metrics.NetworkUsageMbps = SystemMetrics::GetSystemNetworkUsage();

            LockCS lock(s_processCacheCs);
            auto values = s_processCache.GetValues();
            for (int i = 0; i < values.GetLength(); ++i) {
                metrics.TopProcesses.Add(values[i]);
            }
            return metrics;
        } catch (...) {
            return metrics;
        }
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QueryCpuUsage() {
        SystemMetricsResponseDto metrics;
        double sysCpu = SystemMetrics::GetSystemCpuUsage();
        metrics.CpuUsagePercent = (sysCpu < 0.0) ? 0.0 : ((sysCpu > 100.0) ? 100.0 : sysCpu);
        return metrics;
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QueryMemoryUsage() {
        SystemMetricsResponseDto metrics;
        try {
            auto memInfo = SystemMetrics::GetSystemMemoryUsage();
            metrics.MemoryUsagePercent = memInfo.dMemoryUsagePercent;
            metrics.MemoryTotalMB = memInfo.uMemoryTotalBytes / (1024 * 1024);
            metrics.MemoryUsedMB = memInfo.uMemoryUsedBytes / (1024 * 1024);
        } catch (...) {
        }
        return metrics;
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QueryDiskUsage() {
        SystemMetricsResponseDto metrics;
        try {
            auto diskInfo = SystemMetrics::GetSystemDiskUsage();
            double calculatedReadMb = 0.0, calculatedWriteMb = 0.0;
            CalculateDiskRates(diskInfo, calculatedReadMb, calculatedWriteMb);
            metrics.DiskReadMBps = calculatedReadMb;
            metrics.DiskWriteMBps = calculatedWriteMb;
        } catch (...) {
        }
        return metrics;
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QueryNetworkUsage() {
        SystemMetricsResponseDto metrics;
        try {
            metrics.NetworkUsageMbps = SystemMetrics::GetSystemNetworkUsage();
        } catch (...) {
        }
        return metrics;
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QueryProcesses() {
        EnsureProcessStreamerActive();
        SystemMetricsResponseDto metrics;
        LockCS lock(s_processCacheCs);
        auto values = s_processCache.GetValues();
        for (int i = 0; i < values.GetLength(); ++i) {
            metrics.TopProcesses.Add(values[i]);
        }
        return metrics;
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QuerySessions() {
        SystemMetricsResponseDto metrics;
        try {
            PopulateUserSessions(metrics);
        } catch (...) {
        }
        return metrics;
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QuerySystemMetrics() {
        EnsureProcessStreamerActive();
        SystemMetricsResponseDto metrics;
        try {
            double sysCpu = SystemMetrics::GetSystemCpuUsage();
            metrics.CpuUsagePercent = (sysCpu < 0.0) ? 0.0 : ((sysCpu > 100.0) ? 100.0 : sysCpu);

            auto memInfo = SystemMetrics::GetSystemMemoryUsage();
            metrics.MemoryUsagePercent = memInfo.dMemoryUsagePercent;
            metrics.MemoryTotalMB = memInfo.uMemoryTotalBytes / (1024 * 1024);
            metrics.MemoryUsedMB = memInfo.uMemoryUsedBytes / (1024 * 1024);

            auto diskInfo = SystemMetrics::GetSystemDiskUsage();
            double calculatedReadMb = 0.0, calculatedWriteMb = 0.0;
            CalculateDiskRates(diskInfo, calculatedReadMb, calculatedWriteMb);
            metrics.DiskReadMBps = calculatedReadMb;
            metrics.DiskWriteMBps = calculatedWriteMb;
            metrics.NetworkUsageMbps = SystemMetrics::GetSystemNetworkUsage();

            LockCS lock(s_processCacheCs);
            auto values = s_processCache.GetValues();
            for (int i = 0; i < values.GetLength(); ++i) {
                metrics.TopProcesses.Add(values[i]);
            }

            PopulateUserSessions(metrics);
        } catch (...) {
        }
        return metrics;
    }

    ServicesResponseDto SystemTelemetryProvider::QueryServices() {
        ServicesResponseDto dto;
        try {
            auto rawServices = SystemMetrics::GetAllServices();
            for (int i = 0; i < rawServices.GetCount(); ++i) {
                const auto& svc = rawServices[i];
                dto.Services.Add(ServiceInfoDto(svc.sServiceName, svc.sDisplayName, svc.sStatus, svc.sStartType, svc.iProcessId));
            }
        } catch (...) {
        }
        return dto;
    }
}
