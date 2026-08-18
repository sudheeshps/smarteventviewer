#include "pch.h"
#include "Core/SystemTelemetryProvider.h"
#include "Dto/TelemetryDtos.h"
#include "System/Console.h"
#include "System/Diagnostics/SystemMetrics.h"
#include "System/Diagnostics/ActiveUserSession.h"
#include "System/Diagnostics/TerminalSession.h"
#include "System/Security/Principal/UserPrincipal.h"

namespace SmartEventViewer {
    using Console = DotNetDupe::System::Console;
    using SystemMetrics = DotNetDupe::System::Diagnostics::SystemMetrics;
    using ActiveUserSession = DotNetDupe::System::Diagnostics::ActiveUserSession;
    using TerminalSession = DotNetDupe::System::Diagnostics::TerminalSession;
    using RdpSessionState = DotNetDupe::System::Diagnostics::RdpSessionState;
    using UserPrincipal = DotNetDupe::System::Security::Principal::UserPrincipal;
    using UserClass = DotNetDupe::System::Security::Principal::UserClass;

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

            auto topProcs = SystemMetrics::GetTopProcesses(DotNetDupe::System::Diagnostics::SystemResource::Cpu, 10);
            for (int i = 0; i < topProcs.GetCount(); ++i) {
                metrics.TopProcesses.Add(MapProcessResourceDto(topProcs[i]));
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
        SystemMetricsResponseDto metrics;
        try {
            auto topProcs = SystemMetrics::GetTopProcesses(DotNetDupe::System::Diagnostics::SystemResource::Cpu, 20);
            for (int i = 0; i < topProcs.GetCount(); ++i) {
                metrics.TopProcesses.Add(MapProcessResourceDto(topProcs[i]));
            }
        } catch (...) {
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

            auto topProcs = SystemMetrics::GetTopProcesses(DotNetDupe::System::Diagnostics::SystemResource::Cpu, 20);
            double processReadMbSum = 0.0, processWriteMbSum = 0.0;
            for (int i = 0; i < topProcs.GetCount(); ++i) {
                const auto& proc = topProcs[i];
                processReadMbSum += static_cast<double>(proc.disk.lDiskReadBytes > 0 ? (proc.disk.lDiskReadBytes / (1024.0 * 1024.0)) : 0.0);
                processWriteMbSum += static_cast<double>(proc.disk.lDiskWriteBytes > 0 ? (proc.disk.lDiskWriteBytes / (1024.0 * 1024.0)) : 0.0);
                metrics.TopProcesses.Add(MapProcessResourceDto(proc));
            }

            metrics.DiskReadMBps = (calculatedReadMb > 0.0) ? calculatedReadMb : processReadMbSum;
            metrics.DiskWriteMBps = (calculatedWriteMb > 0.0) ? calculatedWriteMb : processWriteMbSum;
            metrics.NetworkUsageMbps = SystemMetrics::GetSystemNetworkUsage();

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
