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
#include "System/Collections/Generic/HashSet.h"
#include "Logging/AppLoggerManager.h"

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
    static DotNetDupe::System::Collections::Generic::HashSet<unsigned long> s_seenPids;
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

    static void LogBatchSample(const DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::Diagnostics::ProcessInfo>& batch) {
        if (batch.GetCount() == 0) return;
        const auto& p = batch[0];
        double dRamMb = static_cast<double>(p.memory.lPhysicalMemoryBytes / (1024 * 1024));
        AppLoggerManager::Info("TELEMETRY", String::Format("[DotNetDupe:BatchReady] BatchSize={0} | Sample PID={1} ({2}) CPU={3}% RAM={4}MB NetR={5} NetW={6}",
            static_cast<double>(batch.GetCount()), static_cast<double>(p.iProcessId), p.sName, p.dCpuUsagePercent, dRamMb, static_cast<double>(p.network.lNetworkReadBytes), static_cast<double>(p.network.lNetworkWriteBytes)));
    }

    static void LogProcessUpdated(const DotNetDupe::System::Diagnostics::ProcessInfo& proc) {
        double dRamMb = static_cast<double>(proc.memory.lPhysicalMemoryBytes / (1024 * 1024));
        AppLoggerManager::Info("TELEMETRY", String::Format("[DotNetDupe:ProcessUpdated] PID={0} ({1}) CPU={2}% RAM={3}MB NetR={4} NetW={5} Ports={6} Estab={7}",
            static_cast<double>(proc.iProcessId), proc.sName, proc.dCpuUsagePercent, dRamMb, static_cast<double>(proc.network.lNetworkReadBytes), static_cast<double>(proc.network.lNetworkWriteBytes), static_cast<double>(proc.lstOpenPorts.GetCount()), proc.bHasEstablishedConnection ? 1.0 : 0.0));
    }

    static void LogStreamCompleted(int iCacheCount, int iSeenCount) {
        AppLoggerManager::Info("TELEMETRY", String::Format("[DotNetDupe:Completed] Stream cycle completed. Cached={0}, ActiveSeen={1}",
            static_cast<double>(iCacheCount), static_cast<double>(iSeenCount)));
    }

    static void MergeExistingProcessMetrics(const ProcessResourceDto& existing, ProcessResourceDto& dto) {
        if (dto.CpuUsagePercent == 0.0 && existing.CpuUsagePercent > 0.0) {
            dto.CpuUsagePercent = existing.CpuUsagePercent;
        }
        if (dto.NetworkReadBytes == 0 && existing.NetworkReadBytes > 0) {
            dto.NetworkReadBytes = existing.NetworkReadBytes;
        }
        if (dto.NetworkWriteBytes == 0 && existing.NetworkWriteBytes > 0) {
            dto.NetworkWriteBytes = existing.NetworkWriteBytes;
        }
        if (dto.OpenPorts == "-" && existing.OpenPorts != "-") {
            dto.OpenPorts = existing.OpenPorts;
        }
        if (!dto.ConnectionEstablished && existing.ConnectionEstablished) {
            dto.ConnectionEstablished = existing.ConnectionEstablished;
        }
    }

    static void MergeBatchInCache(const DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::Diagnostics::ProcessInfo>& batch) {
        LockCS lock(s_processCacheCs);
        LogBatchSample(batch);
        for (int i = 0; i < batch.GetCount(); ++i) {
            if (batch[i].iProcessId <= 0) continue;
            unsigned long uPid = static_cast<unsigned long>(batch[i].iProcessId);
            s_seenPids.Add(uPid);
            auto dto = SystemTelemetryProvider::MapProcessResourceDto(batch[i]);
            if (s_processCache.ContainsKey(uPid)) {
                MergeExistingProcessMetrics(s_processCache[uPid], dto);
            }
            s_processCache[uPid] = dto;
        }
    }

    static void UpdateProcessInCache(const DotNetDupe::System::Diagnostics::ProcessInfo& proc) {
        if (proc.iProcessId <= 0) return;
        unsigned long uPid = static_cast<unsigned long>(proc.iProcessId);
        LockCS lock(s_processCacheCs);
        s_seenPids.Add(uPid);
        LogProcessUpdated(proc);
        auto dto = SystemTelemetryProvider::MapProcessResourceDto(proc);
        s_processCache[uPid] = dto;
    }

    static void EvictTerminatedProcesses() {
        LockCS lock(s_processCacheCs);
        auto keys = s_processCache.GetKeys();
        for (int i = 0; i < keys.GetLength(); ++i) {
            if (!s_seenPids.Contains(keys[i])) {
                s_processCache.Remove(keys[i]);
            }
        }
        LogStreamCompleted(s_processCache.GetCount(), s_seenPids.GetCount());
    }

    static void AttachStreamerEvents(const DotNetDupe::System::SmartPointer<ProcessStreamer>& spStreamer) {
        spStreamer->BatchReady += [](const void* pSender, const DotNetDupe::System::Diagnostics::ProcessBatchEventArgs& e) {
            MergeBatchInCache(e.GetBatch());
        };
        spStreamer->ProcessUpdated += [](const void* pSender, const DotNetDupe::System::Diagnostics::ProcessEventArgs& e) {
            UpdateProcessInCache(e.GetProcess());
        };
        spStreamer->Completed += [](const void* pSender, const DotNetDupe::System::EventArgs& e) {
            EvictTerminatedProcesses();
        };
    }

    static void EnsureProcessStreamerActive() {
        LockCS lock(s_processCacheCs);
        unsigned long long cur = GetTickMs();
        if (!s_pProcessStreamer.IsNull() && (s_pProcessStreamer->IsRunning() || cur - s_lastProcessStreamStartMs < 2000)) return;
        s_lastProcessStreamStartMs = cur;
        s_seenPids.Clear();

        ProcessStreamOptions options;
        options.eDetailLevel = ProcessMetricsDetail::Progressive;
        options.iBatchSize = 25;
        options.iBatchIntervalMs = 50;
        options.bIncludeNetworkInfo = true;

        AppLoggerManager::Info("TELEMETRY", String::Format("[ProcessStreamer] Starting background streamer pass (BatchSize={0})", static_cast<double>(options.iBatchSize)));
        s_pProcessStreamer = DotNetDupe::System::SmartPointer<ProcessStreamer>::NewShared(options);
        AttachStreamerEvents(s_pProcessStreamer);
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

    static String FormatPorts(const DotNetDupe::System::Collections::Generic::List<int>& lstPorts) {
        if (lstPorts.GetCount() == 0) return String("-");
        String sResult = "";
        for (int i = 0; i < lstPorts.GetCount() && i < 5; ++i) {
            if (i > 0) sResult = sResult + ", ";
            sResult = sResult + String::Format("{0}", static_cast<double>(lstPorts[i]));
        }
        return sResult;
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
            procDto.OpenPorts = FormatPorts(proc.lstOpenPorts);
            procDto.ConnectionEstablished = proc.bHasEstablishedConnection || (procDto.NetworkReadBytes > 0 || procDto.NetworkWriteBytes > 0);
        } catch (...) {
        }
        return procDto;
    }

    static void PopulateActiveSessions(SystemMetricsResponseDto& metrics) {
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
    }

    static void PopulateExpiredSessions(SystemMetricsResponseDto& metrics) {
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
    }

    static void PopulateSystemUsers(SystemMetricsResponseDto& metrics) {
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
    }

    static String GetRdpStateString(RdpSessionState state) {
        switch (state) {
            case RdpSessionState::Active: return "Active";
            case RdpSessionState::Connected: return "Connected";
            case RdpSessionState::Disconnected: return "Disconnected";
            case RdpSessionState::Idle: return "Idle";
            case RdpSessionState::Listen: return "Listen";
            case RdpSessionState::Shadow: return "Shadow";
            case RdpSessionState::Reset: return "Reset";
            case RdpSessionState::Down: return "Down";
            case RdpSessionState::Init: return "Init";
            default: return "Unknown";
        }
    }

    static void PopulateRdpSessions(SystemMetricsResponseDto& metrics) {
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
            dto.State = GetRdpStateString(r.eState);
            metrics.RdpSessions.Add(dto);
        }
    }

    void SystemTelemetryProvider::PopulateUserSessions(SystemMetricsResponseDto& metrics) {
        PopulateActiveSessions(metrics);
        PopulateExpiredSessions(metrics);
        PopulateSystemUsers(metrics);
        PopulateRdpSessions(metrics);
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
        AppLoggerManager::Debug("TELEMETRY", String::Format("[QueryProcesses] Serving {0} processes from cache", static_cast<double>(values.GetLength())));
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
        auto metrics = QuerySummary();
        try {
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
