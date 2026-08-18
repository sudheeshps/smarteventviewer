#include "pch.h"
#include "Core/AnomalyEngine.h"

namespace SmartEventViewer {
    RiskLevel AnomalyEngine::StaticEvaluateRisk(const EventRecord& eventRec) {
        unsigned int uId = eventRec.GetEventId();
        if (uId == 1102 || uId == 104) return RiskLevel::Critical;
        if (uId == 4625 || uId == 4720 || uId == 4732 || uId == 4697 || uId == 7045) return RiskLevel::High;
        if (uId == 4672 || uId == 4688 || eventRec.GetLevel() == EventLevel::Critical || eventRec.GetLevel() == EventLevel::Error) return RiskLevel::Medium;
        if (eventRec.GetLevel() == EventLevel::Warning) return RiskLevel::Medium;
        return RiskLevel::Low;
    }

    static bool IsLolbinCommand(const String& sName, const String& sCmd) {
        String sLowerName = sName.ToLower();
        if (sLowerName.Contains("powershell") || sLowerName.Contains("pwsh") || sLowerName.Contains("cmd.exe") ||
            sLowerName.Contains("wscript") || sLowerName.Contains("cscript") || sLowerName.Contains("certutil") ||
            sLowerName.Contains("mshta") || sLowerName.Contains("bitsadmin") || sLowerName.Contains("rundll32")) {
            String sLowerCmd = sCmd.ToLower();
            return sLowerCmd.Contains("-enc") || sLowerCmd.Contains("downloadstring") || sLowerCmd.Contains("http://") ||
                   sLowerCmd.Contains("https://") || sLowerCmd.Contains("bypass") || sLowerCmd.Contains("hidden") ||
                   sLowerCmd.Contains("/urlcache") || sLowerCmd.Contains("iex");
        }
        return false;
    }

    static bool IsSuspiciousProcessPath(const String& sPath) {
        String sLower = sPath.ToLower();
        return sLower.Contains("\\appdata\\local\\temp") || sLower.Contains("\\users\\public") || sLower.Contains("\\windows\\temp");
    }

    bool AnomalyEngine::StaticEvaluateProcess(const ProcessResourceDto& proc, ProcessAnomalyDto& outAnomaly) {
        if (IsLolbinCommand(proc.Name, proc.CommandLine)) {
            outAnomaly.Process = proc;
            outAnomaly.Reason = "LOLBin script execution with remote/encoded arguments";
            outAnomaly.Risk = "High";
            return true;
        }
        if (IsSuspiciousProcessPath(proc.Path)) {
            outAnomaly.Process = proc;
            outAnomaly.Reason = "Binary executing from temporary or public directory";
            outAnomaly.Risk = "High";
            return true;
        }
        if (proc.ConnectionEstablished && (proc.Name.Contains("cmd.exe") || proc.Name.Contains("powershell.exe"))) {
            outAnomaly.Process = proc;
            outAnomaly.Reason = "Interactive shell maintaining established remote connection";
            outAnomaly.Risk = "High";
            return true;
        }
        if (proc.CpuUsagePercent > 85.0 || proc.MemoryUsageMB > 3072) {
            outAnomaly.Process = proc;
            outAnomaly.Reason = "Abnormal host resource consumption";
            outAnomaly.Risk = "Medium";
            return true;
        }
        return false;
    }

    static bool IsExternalIp(const String& sIp) {
        if (sIp.IsEmpty() || sIp == "-" || sIp == "127.0.0.1" || sIp == "::1") return false;
        if (sIp.StartsWith("10.") || sIp.StartsWith("192.168.")) return false;
        if (sIp.StartsWith("172.16.") || sIp.StartsWith("172.17.") || sIp.StartsWith("172.18.") || sIp.StartsWith("172.19.") ||
            sIp.StartsWith("172.20.") || sIp.StartsWith("172.21.") || sIp.StartsWith("172.22.") || sIp.StartsWith("172.23.") ||
            sIp.StartsWith("172.24.") || sIp.StartsWith("172.25.") || sIp.StartsWith("172.26.") || sIp.StartsWith("172.27.") ||
            sIp.StartsWith("172.28.") || sIp.StartsWith("172.29.") || sIp.StartsWith("172.30.") || sIp.StartsWith("172.31.")) return false;
        return true;
    }

    bool AnomalyEngine::StaticEvaluateSession(const RdpSessionDto& rdp, SessionAnomalyDto& outAnomaly) {
        if (IsExternalIp(rdp.ClientIpAddress)) {
            outAnomaly.Session = rdp;
            outAnomaly.Reason = "RDP session originating from external/public IP address";
            outAnomaly.Risk = "Critical";
            return true;
        }
        if (rdp.State == "Shadow") {
            outAnomaly.Session = rdp;
            outAnomaly.Reason = "Remote session shadow observation active";
            outAnomaly.Risk = "High";
            return true;
        }
        if (rdp.UserName.ToLower() == "guest" && rdp.IsRdpSession) {
            outAnomaly.Session = rdp;
            outAnomaly.Reason = "Active RDP session with default Guest account";
            outAnomaly.Risk = "High";
            return true;
        }
        return false;
    }

    bool AnomalyEngine::StaticEvaluateUser(const UserPrincipalDto& user, UserAnomalyDto& outAnomaly) {
        String sLowerUser = user.Username.ToLower();
        if (sLowerUser == "guest" && !user.IsDisabled) {
            outAnomaly.User = user;
            outAnomaly.Reason = "Insecure default Guest account is enabled";
            outAnomaly.Risk = "High";
            return true;
        }
        if (user.UserClass == "Admin" && sLowerUser != "administrator" && sLowerUser != "system") {
            outAnomaly.User = user;
            outAnomaly.Reason = "Non-standard local administrator account";
            outAnomaly.Risk = "Medium";
            return true;
        }
        if (user.Groups.Contains("Remote Desktop Users") && user.UserClass != "Admin") {
            outAnomaly.User = user;
            outAnomaly.Reason = "User granted explicit remote desktop access";
            outAnomaly.Risk = "Medium";
            return true;
        }
        return false;
    }

    bool AnomalyEngine::StaticEvaluateService(const ServiceInfoDto& service, ServiceAnomalyDto& outAnomaly) {
        String sName = service.ServiceName.ToLower();
        if (service.StartType == "Auto" && service.Status == "Stopped") {
            outAnomaly.Service = service;
            outAnomaly.Reason = "Automatic service failed to start or stopped unexpectedly";
            outAnomaly.Risk = "Low";
            return true;
        }
        if (!sName.StartsWith("win") && !sName.StartsWith("app") && !sName.StartsWith("sys") && service.ProcessId > 0 && service.Status == "Running") {
            outAnomaly.Service = service;
            outAnomaly.Reason = "Active third-party service registered on host";
            outAnomaly.Risk = "Low";
            return true;
        }
        return false;
    }

    static void PopulateFlaggedArtifacts(
        const SystemMetricsResponseDto& metrics,
        const ServicesResponseDto& services,
        TelemetryPostureReportDto& outReport,
        int& outScore) {
        for (int i = 0; i < metrics.TopProcesses.GetCount(); ++i) {
            ProcessAnomalyDto anom;
            if (AnomalyEngine::StaticEvaluateProcess(metrics.TopProcesses[i], anom)) {
                outReport.FlaggedProcesses.Add(anom);
                outScore += (anom.Risk == "High" ? 15 : 8);
            }
        }
        for (int i = 0; i < metrics.RdpSessions.GetCount(); ++i) {
            SessionAnomalyDto anom;
            if (AnomalyEngine::StaticEvaluateSession(metrics.RdpSessions[i], anom)) {
                outReport.SuspiciousSessions.Add(anom);
                outScore += (anom.Risk == "Critical" ? 25 : 15);
            }
        }
        for (int i = 0; i < metrics.SystemUsers.GetCount(); ++i) {
            UserAnomalyDto anom;
            if (AnomalyEngine::StaticEvaluateUser(metrics.SystemUsers[i], anom)) {
                outReport.FlaggedUsers.Add(anom);
                outScore += (anom.Risk == "High" ? 12 : 6);
            }
        }
        for (int i = 0; i < services.Services.GetCount(); ++i) {
            ServiceAnomalyDto anom;
            if (AnomalyEngine::StaticEvaluateService(services.Services[i], anom)) {
                outReport.SuspiciousServices.Add(anom);
                outScore += 4;
            }
        }
    }

    TelemetryPostureReportDto AnomalyEngine::StaticEvaluatePosture(const SystemMetricsResponseDto& metrics, const ServicesResponseDto& services) {
        TelemetryPostureReportDto report;
        int score = 0;
        PopulateFlaggedArtifacts(metrics, services, report, score);
        report.ThreatScore = score > 100 ? 100 : score;
        if (report.ThreatScore >= 60) report.OverallRisk = "CRITICAL";
        else if (report.ThreatScore >= 35) report.OverallRisk = "HIGH";
        else if (report.ThreatScore >= 15) report.OverallRisk = "MEDIUM";
        else report.OverallRisk = "LOW";
        return report;
    }

    RiskLevel AnomalyEngine::EvaluateRisk(const EventRecord& eventRec) {
        return StaticEvaluateRisk(eventRec);
    }
    bool AnomalyEngine::EvaluateProcess(const ProcessResourceDto& proc, ProcessAnomalyDto& outAnomaly) {
        return StaticEvaluateProcess(proc, outAnomaly);
    }
    bool AnomalyEngine::EvaluateSession(const RdpSessionDto& rdp, SessionAnomalyDto& outAnomaly) {
        return StaticEvaluateSession(rdp, outAnomaly);
    }
    bool AnomalyEngine::EvaluateUser(const UserPrincipalDto& user, UserAnomalyDto& outAnomaly) {
        return StaticEvaluateUser(user, outAnomaly);
    }
    bool AnomalyEngine::EvaluateService(const ServiceInfoDto& service, ServiceAnomalyDto& outAnomaly) {
        return StaticEvaluateService(service, outAnomaly);
    }
    TelemetryPostureReportDto AnomalyEngine::EvaluatePosture(const SystemMetricsResponseDto& metrics, const ServicesResponseDto& services) {
        return StaticEvaluatePosture(metrics, services);
    }
}
