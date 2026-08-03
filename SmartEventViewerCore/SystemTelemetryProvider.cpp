#include "pch.h"
#include "../Include/Platform/SystemTelemetryProvider.h"
#include "System/Console.h"
#include "System/Diagnostics/SystemMetrics.h"
#include "System/Diagnostics/ActiveUserSession.h"
#include "System/Security/Principal/UserPrincipal.h"

namespace SmartEventViewer
{
    using Console = DotNetDupe::System::Console;
    using SystemMetrics = DotNetDupe::System::Diagnostics::SystemMetrics;
    using ActiveUserSession = DotNetDupe::System::Diagnostics::ActiveUserSession;
    using UserPrincipal = DotNetDupe::System::Security::Principal::UserPrincipal;
    using UserClass = DotNetDupe::System::Security::Principal::UserClass;

    static void CalculateDiskRates(const DotNetDupe::System::Diagnostics::RealTimeSystemInfo& realInfo, double& outReadMb, double& outWriteMb)
    {
        static uint64_t s_lastDiskReadBytes = 0;
        static uint64_t s_lastDiskWriteBytes = 0;

        uint64_t curRead = realInfo.uDiskReadBytes;
        uint64_t curWrite = realInfo.uDiskWriteBytes;

        if (s_lastDiskReadBytes == 0 && s_lastDiskWriteBytes == 0)
        {
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

    static String FormatCommandLine(const String& sPath, const String& sCmd)
    {
        if (sCmd.IsEmpty()) return String("Access Denied (System Protected)");

        String sResult = sCmd;
        if (!sPath.IsEmpty() && sResult.StartsWith(sPath))
        {
            sResult = sResult.Substring(static_cast<int>(sPath.GetLength()));
        }
        else if (sResult.StartsWith(String("\"")))
        {
            int nextQuote = sResult.IndexOf(String("\""), 1);
            if (nextQuote != -1) sResult = sResult.Substring(nextQuote + 1);
        }
        else
        {
            int spaceIdx = sResult.IndexOf(String(" "));
            if (spaceIdx != -1) sResult = sResult.Substring(spaceIdx);
        }

        sResult = sResult.Trim();
        return sResult.IsEmpty() ? String("-") : sResult;
    }

    static ProcessResourceDto MapProcessResourceDto(const DotNetDupe::System::Diagnostics::ProcessResourceInfo& proc)
    {
        ProcessResourceDto procDto;
        procDto.ProcessId = static_cast<unsigned long>(proc.iProcessId);
        procDto.Name = proc.sName.IsEmpty() ? String("Access Denied") : proc.sName;
        procDto.Path = proc.sPath.IsEmpty() ? String("Access Denied (System Protected)") : proc.sPath;
        procDto.CommandLine = FormatCommandLine(proc.sPath, proc.sCommandLine);

        double procCpu = proc.dCpuUsagePercent;
        if (procCpu < 0.0) procCpu = 0.0;
        if (procCpu > 100.0) procCpu = 100.0;
        procDto.CpuUsagePercent = procCpu;

        long long rawRamBytes = proc.lMemoryUsageBytes;
        procDto.MemoryUsageMB = static_cast<unsigned long long>(rawRamBytes > 0 ? (rawRamBytes / (1024 * 1024)) : 0);

        double readMb = static_cast<double>(proc.lDiskReadBytes > 0 ? (proc.lDiskReadBytes / (1024.0 * 1024.0)) : 0.0);
        double writeMb = static_cast<double>(proc.lDiskWriteBytes > 0 ? (proc.lDiskWriteBytes / (1024.0 * 1024.0)) : 0.0);
        procDto.DiskReadKBps = readMb;
        procDto.DiskWriteKBps = writeMb;
        procDto.DiskIoKBps = readMb + writeMb;
        return procDto;
    }

    static void PopulateUserSessions(SystemMetricsResponseDto& metrics)
    {
        auto activeSessions = ActiveUserSession::GetActiveSessions();
        for (int i = 0; i < activeSessions.GetCount(); ++i)
        {
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
        for (int i = 0; i < expiredSessions.GetCount(); ++i)
        {
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
        for (int i = 0; i < users.GetCount(); ++i)
        {
            const auto& u = users[i];
            UserPrincipalDto dto;
            dto.Username = u.sUsername;
            dto.Domain = u.sDomain;
            dto.SidOrUid = u.sSidOrUid;
            if (u.eUserClass == UserClass::Admin) dto.UserClass = String("Admin");
            else if (u.eUserClass == UserClass::System) dto.UserClass = String("System");
            else if (u.eUserClass == UserClass::Guest) dto.UserClass = String("Guest");
            else dto.UserClass = String("Normal");
            dto.IsDisabled = u.bIsDisabled;
            dto.IsAccountLocked = u.bIsAccountLocked;
            dto.Groups = u.lstGroups;
            dto.Permissions = u.lstPermissions;
            metrics.SystemUsers.Add(dto);
        }
    }

    SystemMetricsResponseDto SystemTelemetryProvider::QuerySystemMetrics()
    {
        Console::WriteLine("[TELEMETRY] Querying SystemMetrics & ActiveUserSession...");
        SystemMetricsResponseDto metrics;

        auto realInfo = SystemMetrics::GetSystemMetrics();
        double sysCpu = realInfo.dCpuUsagePercent;
        if (sysCpu < 0.0) sysCpu = 0.0;
        if (sysCpu > 100.0) sysCpu = 100.0;

        metrics.CpuUsagePercent = sysCpu;
        metrics.MemoryUsagePercent = realInfo.dMemoryUsagePercent;
        metrics.MemoryTotalMB = realInfo.uMemoryTotalBytes / (1024 * 1024);
        metrics.MemoryUsedMB = realInfo.uMemoryUsedBytes / (1024 * 1024);

        double calculatedReadMb = 0.0, calculatedWriteMb = 0.0;
        CalculateDiskRates(realInfo, calculatedReadMb, calculatedWriteMb);

        double processReadMbSum = 0.0, processWriteMbSum = 0.0;
        for (int i = 0; i < realInfo.lstTopProcesses.GetCount(); ++i)
        {
            const auto& proc = realInfo.lstTopProcesses[i];
            processReadMbSum += static_cast<double>(proc.lDiskReadBytes > 0 ? (proc.lDiskReadBytes / (1024.0 * 1024.0)) : 0.0);
            processWriteMbSum += static_cast<double>(proc.lDiskWriteBytes > 0 ? (proc.lDiskWriteBytes / (1024.0 * 1024.0)) : 0.0);
            metrics.TopProcesses.Add(MapProcessResourceDto(proc));
        }

        metrics.DiskReadMBps = (calculatedReadMb > 0.0) ? calculatedReadMb : processReadMbSum;
        metrics.DiskWriteMBps = (calculatedWriteMb > 0.0) ? calculatedWriteMb : processWriteMbSum;
        metrics.NetworkUsageMbps = realInfo.dNetworkUsageMbps;

        PopulateUserSessions(metrics);
        return metrics;
    }
}
