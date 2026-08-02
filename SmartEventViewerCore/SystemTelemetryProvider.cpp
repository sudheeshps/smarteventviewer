#include "pch.h"
#include "../Include/Platform/SystemTelemetryProvider.h"
#include "System/Console.h"
#include "System/Diagnostics/SystemMetrics.h"
#include "System/Diagnostics/ActiveUserSession.h"

namespace SmartEventViewer
{
    using Console = DotNetDupe::System::Console;
    using SystemMetrics = DotNetDupe::System::Diagnostics::SystemMetrics;
    using ActiveUserSession = DotNetDupe::System::Diagnostics::ActiveUserSession;

    SystemMetricsResponseDto SystemTelemetryProvider::QuerySystemMetrics()
    {
        Console::WriteLine("[TELEMETRY] Delegating system metrics strictly to DotNetDupe::SystemMetrics & ActiveUserSession...");
        SystemMetricsResponseDto metrics;

        // 1. Fetch hardware & process telemetry directly via DotNetDupe::SystemMetrics
        auto realInfo = SystemMetrics::GetSystemMetrics();
        double sysCpu = realInfo.dCpuUsagePercent;
        if (sysCpu < 0.0) sysCpu = 0.0;
        if (sysCpu > 100.0) sysCpu = 100.0;
        metrics.CpuUsagePercent = sysCpu;
        metrics.MemoryUsagePercent = realInfo.dMemoryUsagePercent;
        metrics.MemoryTotalMB = realInfo.uMemoryTotalBytes / (1024 * 1024);
        metrics.MemoryUsedMB = realInfo.uMemoryUsedBytes / (1024 * 1024);
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

        double calculatedReadMb = static_cast<double>(deltaRead) / (1024.0 * 1024.0);
        double calculatedWriteMb = static_cast<double>(deltaWrite) / (1024.0 * 1024.0);

        // Sum across top processes as fallback if DotNetDupe system counters are static
        double processReadMbSum = 0.0;
        double processWriteMbSum = 0.0;

        for (int i = 0; i < realInfo.lstTopProcesses.GetCount(); ++i)
        {
            const auto& p = realInfo.lstTopProcesses[i];
            processReadMbSum += static_cast<double>(p.lDiskReadBytes > 0 ? (p.lDiskReadBytes / (1024.0 * 1024.0)) : 0.0);
            processWriteMbSum += static_cast<double>(p.lDiskWriteBytes > 0 ? (p.lDiskWriteBytes / (1024.0 * 1024.0)) : 0.0);
        }

        metrics.DiskReadMBps = (calculatedReadMb > 0.0) ? calculatedReadMb : processReadMbSum;
        metrics.DiskWriteMBps = (calculatedWriteMb > 0.0) ? calculatedWriteMb : processWriteMbSum;
        metrics.NetworkUsageMbps = realInfo.dNetworkUsageMbps;

        for (int i = 0; i < realInfo.lstTopProcesses.GetCount(); ++i)
        {
            const auto& proc = realInfo.lstTopProcesses[i];
            ProcessResourceDto procDto;
            procDto.ProcessId = static_cast<unsigned long>(proc.iProcessId);
            procDto.Name = proc.sName.IsEmpty() ? String("Access Denied") : proc.sName;
            procDto.Path = proc.sPath.IsEmpty() ? String("Access Denied (System Protected)") : proc.sPath;
            
            String sCmd = proc.sCommandLine;
            if (sCmd.IsEmpty())
            {
                procDto.CommandLine = String("Access Denied (System Protected)");
            }
            else
            {
                std::string stdCmd = sCmd.GetRawString();
                std::string stdPath = proc.sPath.GetRawString();
                
                // Strip leading quoted executable path or plain path prefix
                if (!stdPath.empty() && stdCmd.rfind(stdPath, 0) == 0)
                {
                    stdCmd = stdCmd.substr(stdPath.length());
                }
                else if (stdCmd.size() > 0 && stdCmd[0] == '"')
                {
                    size_t nextQuote = stdCmd.find('"', 1);
                    if (nextQuote != std::string::npos)
                    {
                        stdCmd = stdCmd.substr(nextQuote + 1);
                    }
                }
                else
                {
                    size_t spaceIdx = stdCmd.find(' ');
                    if (spaceIdx != std::string::npos)
                    {
                        stdCmd = stdCmd.substr(spaceIdx);
                    }
                }

                // Trim leading spaces
                size_t startPos = stdCmd.find_first_not_of(" \t");
                if (startPos != std::string::npos) stdCmd = stdCmd.substr(startPos);
                else stdCmd = "";

                procDto.CommandLine = stdCmd.empty() ? String("-") : String(stdCmd.c_str());
            }
            
            double procCpu = proc.dCpuUsagePercent;
            if (procCpu < 0.0) procCpu = 0.0;
            if (procCpu > 100.0) procCpu = 100.0;
            procDto.CpuUsagePercent = procCpu;

            // RAM usage returned by DotNetDupe is lMemoryUsageBytes -> convert to MB
            long long rawRamBytes = proc.lMemoryUsageBytes;
            procDto.MemoryUsageMB = static_cast<unsigned long long>(rawRamBytes > 0 ? (rawRamBytes / (1024 * 1024)) : 0);

            // Disk Read and Write bytes returned by DotNetDupe -> convert to MB
            double readMb = static_cast<double>(proc.lDiskReadBytes > 0 ? (proc.lDiskReadBytes / (1024.0 * 1024.0)) : 0.0);
            double writeMb = static_cast<double>(proc.lDiskWriteBytes > 0 ? (proc.lDiskWriteBytes / (1024.0 * 1024.0)) : 0.0);
            procDto.DiskReadKBps = readMb;
            procDto.DiskWriteKBps = writeMb;
            procDto.DiskIoKBps = readMb + writeMb;
            metrics.TopProcesses.Add(procDto);
        }

        // 2. Fetch active & expired user sessions via DotNetDupe::ActiveUserSession
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

        return metrics;
    }
}
