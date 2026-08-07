#pragma once

#include "ViewerCommon.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "System/Text/Json/JsonSerializer.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    struct ProcessResourceDto
    {
        unsigned long ProcessId{ 0 };
        String Name{};
        String Path{};
        String CommandLine{};
        double CpuUsagePercent{ 0.0 };
        unsigned long long MemoryUsageMB{ 0 };
        unsigned long long NetworkReadBytes{ 0 };
        unsigned long long NetworkWriteBytes{ 0 };
        String OpenPorts{};
        bool ConnectionEstablished{ false };

        ProcessResourceDto() = default;
        ProcessResourceDto(const ProcessResourceDto&) = default;
        ProcessResourceDto& operator=(const ProcessResourceDto&) = default;
    };

    struct RdpSessionDto
    {
        unsigned long SessionId{ 0 };
        String SessionName{};
        String UserName{};
        String DomainName{};
        String ClientName{};
        String ClientIpAddress{};
        String State{};
        bool IsRdpSession{ false };

        RdpSessionDto() = default;
        RdpSessionDto(const RdpSessionDto&) = default;
        RdpSessionDto& operator=(const RdpSessionDto&) = default;
    };

    struct UserSessionDto
    {
        String Username{};
        String Privilege{};
        String LoginTimestamp{};
        String LogoutTimestamp{};
        bool IsActive{ true };

        UserSessionDto() = default;
        UserSessionDto(const UserSessionDto&) = default;
        UserSessionDto& operator=(const UserSessionDto&) = default;
    };

    struct UserPrincipalDto
    {
        String Username{};
        String Domain{};
        String SidOrUid{};
        String UserClass{};
        bool IsDisabled{ false };
        bool IsAccountLocked{ false };
        StringList Groups{};
        StringList Permissions{};

        UserPrincipalDto() = default;
        UserPrincipalDto(const UserPrincipalDto&) = default;
        UserPrincipalDto& operator=(const UserPrincipalDto&) = default;
    };

    struct SystemMetricsResponseDto
    {
        double CpuUsagePercent{ 0.0 };
        double MemoryUsagePercent{ 0.0 };
        unsigned long long MemoryUsedMB{ 0 };
        unsigned long long MemoryTotalMB{ 0 };
        double DiskUsagePercent{ 0.0 };
        double DiskReadMBps{ 0.0 };
        double DiskWriteMBps{ 0.0 };
        double NetworkUsageMbps{ 0.0 };

        DotNetDupe::System::Collections::Generic::List<ProcessResourceDto> TopProcesses{};
        DotNetDupe::System::Collections::Generic::List<UserSessionDto> ActiveUserSessions{};
        DotNetDupe::System::Collections::Generic::List<UserSessionDto> ExpiredUserSessions{};
        DotNetDupe::System::Collections::Generic::List<UserPrincipalDto> SystemUsers{};
        DotNetDupe::System::Collections::Generic::List<RdpSessionDto> RdpSessions{};
    };
}

namespace DotNetDupe {
    namespace System {
        namespace Text {
            namespace Json {
                template <typename Enable>
                struct JsonConverter<SmartEventViewer::ProcessResourceDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::ProcessResourceDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("processId"), JsonElement(static_cast<double>(value.ProcessId)));
                        obj.SetProperty(String("name"), JsonElement(value.Name));
                        obj.SetProperty(String("path"), JsonElement(value.Path));
                        obj.SetProperty(String("commandLine"), JsonElement(value.CommandLine));
                        obj.SetProperty(String("cpuUsagePercent"), JsonElement(value.CpuUsagePercent));
                        obj.SetProperty(String("memoryUsageMB"), JsonElement(static_cast<double>(value.MemoryUsageMB)));
                        obj.SetProperty(String("networkReadBytes"), JsonElement(static_cast<double>(value.NetworkReadBytes)));
                        obj.SetProperty(String("networkWriteBytes"), JsonElement(static_cast<double>(value.NetworkWriteBytes)));
                        obj.SetProperty(String("openPorts"), JsonElement(value.OpenPorts));
                        obj.SetProperty(String("connectionEstablished"), JsonElement(value.ConnectionEstablished));
                        return obj;
                    }
                    static SmartEventViewer::ProcessResourceDto Read(const JsonElement& element) {
                        SmartEventViewer::ProcessResourceDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("processId"), prop)) dto.ProcessId = static_cast<unsigned long>(prop.GetDouble());
                        if (element.TryGetProperty(String("name"), prop)) dto.Name = prop.GetString();
                        if (element.TryGetProperty(String("path"), prop)) dto.Path = prop.GetString();
                        if (element.TryGetProperty(String("commandLine"), prop)) dto.CommandLine = prop.GetString();
                        if (element.TryGetProperty(String("cpuUsagePercent"), prop)) dto.CpuUsagePercent = prop.GetDouble();
                        if (element.TryGetProperty(String("memoryUsageMB"), prop)) dto.MemoryUsageMB = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("networkReadBytes"), prop)) dto.NetworkReadBytes = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("networkWriteBytes"), prop)) dto.NetworkWriteBytes = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("openPorts"), prop)) dto.OpenPorts = prop.GetString();
                        if (element.TryGetProperty(String("connectionEstablished"), prop)) dto.ConnectionEstablished = prop.GetBoolean();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::UserSessionDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::UserSessionDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("username"), JsonElement(value.Username));
                        obj.SetProperty(String("privilege"), JsonElement(value.Privilege));
                        obj.SetProperty(String("loginTimestamp"), JsonElement(value.LoginTimestamp));
                        obj.SetProperty(String("logoutTimestamp"), JsonElement(value.LogoutTimestamp));
                        obj.SetProperty(String("isActive"), JsonElement(value.IsActive));
                        return obj;
                    }
                    static SmartEventViewer::UserSessionDto Read(const JsonElement& element) {
                        SmartEventViewer::UserSessionDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("username"), prop)) dto.Username = prop.GetString();
                        if (element.TryGetProperty(String("privilege"), prop)) dto.Privilege = prop.GetString();
                        if (element.TryGetProperty(String("loginTimestamp"), prop)) dto.LoginTimestamp = prop.GetString();
                        if (element.TryGetProperty(String("logoutTimestamp"), prop)) dto.LogoutTimestamp = prop.GetString();
                        if (element.TryGetProperty(String("isActive"), prop)) dto.IsActive = prop.GetBoolean();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::UserPrincipalDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::UserPrincipalDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("username"), JsonElement(value.Username));
                        obj.SetProperty(String("domain"), JsonElement(value.Domain));
                        obj.SetProperty(String("sidOrUid"), JsonElement(value.SidOrUid));
                        obj.SetProperty(String("userClass"), JsonElement(value.UserClass));
                        obj.SetProperty(String("isDisabled"), JsonElement(value.IsDisabled));
                        obj.SetProperty(String("isAccountLocked"), JsonElement(value.IsAccountLocked));
                        obj.SetProperty(String("groups"), JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Write(value.Groups));
                        obj.SetProperty(String("permissions"), JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Write(value.Permissions));
                        return obj;
                    }
                    static SmartEventViewer::UserPrincipalDto Read(const JsonElement& element) {
                        SmartEventViewer::UserPrincipalDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("username"), prop)) dto.Username = prop.GetString();
                        if (element.TryGetProperty(String("domain"), prop)) dto.Domain = prop.GetString();
                        if (element.TryGetProperty(String("sidOrUid"), prop)) dto.SidOrUid = prop.GetString();
                        if (element.TryGetProperty(String("userClass"), prop)) dto.UserClass = prop.GetString();
                        if (element.TryGetProperty(String("isDisabled"), prop)) dto.IsDisabled = prop.GetBoolean();
                        if (element.TryGetProperty(String("isAccountLocked"), prop)) dto.IsAccountLocked = prop.GetBoolean();
                        if (element.TryGetProperty(String("groups"), prop)) dto.Groups = JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Read(prop);
                        if (element.TryGetProperty(String("permissions"), prop)) dto.Permissions = JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Read(prop);
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::RdpSessionDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::RdpSessionDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("sessionId"), JsonElement(static_cast<double>(value.SessionId)));
                        obj.SetProperty(String("sessionName"), JsonElement(value.SessionName));
                        obj.SetProperty(String("userName"), JsonElement(value.UserName));
                        obj.SetProperty(String("domainName"), JsonElement(value.DomainName));
                        obj.SetProperty(String("clientName"), JsonElement(value.ClientName));
                        obj.SetProperty(String("clientIpAddress"), JsonElement(value.ClientIpAddress));
                        obj.SetProperty(String("state"), JsonElement(value.State));
                        obj.SetProperty(String("isRdpSession"), JsonElement(value.IsRdpSession));
                        return obj;
                    }
                    static SmartEventViewer::RdpSessionDto Read(const JsonElement& element) {
                        SmartEventViewer::RdpSessionDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("sessionId"), prop)) dto.SessionId = static_cast<unsigned long>(prop.GetDouble());
                        if (element.TryGetProperty(String("sessionName"), prop)) dto.SessionName = prop.GetString();
                        if (element.TryGetProperty(String("userName"), prop)) dto.UserName = prop.GetString();
                        if (element.TryGetProperty(String("domainName"), prop)) dto.DomainName = prop.GetString();
                        if (element.TryGetProperty(String("clientName"), prop)) dto.ClientName = prop.GetString();
                        if (element.TryGetProperty(String("clientIpAddress"), prop)) dto.ClientIpAddress = prop.GetString();
                        if (element.TryGetProperty(String("state"), prop)) dto.State = prop.GetString();
                        if (element.TryGetProperty(String("isRdpSession"), prop)) dto.IsRdpSession = prop.GetBoolean();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::SystemMetricsResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::SystemMetricsResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("cpuUsagePercent"), JsonElement(value.CpuUsagePercent));
                        obj.SetProperty(String("memoryUsagePercent"), JsonElement(value.MemoryUsagePercent));
                        obj.SetProperty(String("memoryUsedMB"), JsonElement(static_cast<double>(value.MemoryUsedMB)));
                        obj.SetProperty(String("memoryTotalMB"), JsonElement(static_cast<double>(value.MemoryTotalMB)));
                        obj.SetProperty(String("diskUsagePercent"), JsonElement(value.DiskUsagePercent));
                        obj.SetProperty(String("diskReadMBps"), JsonElement(value.DiskReadMBps));
                        obj.SetProperty(String("diskWriteMBps"), JsonElement(value.DiskWriteMBps));
                        obj.SetProperty(String("networkUsageMbps"), JsonElement(value.NetworkUsageMbps));
                        obj.SetProperty(String("topProcesses"), JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::ProcessResourceDto>>::Write(value.TopProcesses));
                        obj.SetProperty(String("activeUserSessions"), JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserSessionDto>>::Write(value.ActiveUserSessions));
                        obj.SetProperty(String("expiredUserSessions"), JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserSessionDto>>::Write(value.ExpiredUserSessions));
                        obj.SetProperty(String("systemUsers"), JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserPrincipalDto>>::Write(value.SystemUsers));
                        obj.SetProperty(String("rdpSessions"), JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::RdpSessionDto>>::Write(value.RdpSessions));
                        return obj;
                    }
                    static SmartEventViewer::SystemMetricsResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::SystemMetricsResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("cpuUsagePercent"), prop)) dto.CpuUsagePercent = prop.GetDouble();
                        if (element.TryGetProperty(String("memoryUsagePercent"), prop)) dto.MemoryUsagePercent = prop.GetDouble();
                        if (element.TryGetProperty(String("memoryUsedMB"), prop)) dto.MemoryUsedMB = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("memoryTotalMB"), prop)) dto.MemoryTotalMB = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("diskUsagePercent"), prop)) dto.DiskUsagePercent = prop.GetDouble();
                        if (element.TryGetProperty(String("diskReadMBps"), prop)) dto.DiskReadMBps = prop.GetDouble();
                        if (element.TryGetProperty(String("diskWriteMBps"), prop)) dto.DiskWriteMBps = prop.GetDouble();
                        if (element.TryGetProperty(String("networkUsageMbps"), prop)) dto.NetworkUsageMbps = prop.GetDouble();
                        if (element.TryGetProperty(String("topProcesses"), prop)) dto.TopProcesses = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::ProcessResourceDto>>::Read(prop);
                        if (element.TryGetProperty(String("activeUserSessions"), prop)) dto.ActiveUserSessions = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserSessionDto>>::Read(prop);
                        if (element.TryGetProperty(String("expiredUserSessions"), prop)) dto.ExpiredUserSessions = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserSessionDto>>::Read(prop);
                        if (element.TryGetProperty(String("systemUsers"), prop)) dto.SystemUsers = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserPrincipalDto>>::Read(prop);
                        if (element.TryGetProperty(String("rdpSessions"), prop)) dto.RdpSessions = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::RdpSessionDto>>::Read(prop);
                        return dto;
                    }
                };
            }
        }
    }
}

namespace SmartEventViewer
{
    class TelemetryController : public DotNetDupe::WebAppCore::Controllers::ControllerBase
    {
    public:
        TelemetryController() = default;
        ~TelemetryController() override = default;

        // Specialized REST Endpoints
        SMARTEVENTVIEWER_API SystemMetricsResponseDto GetSummary();
        SMARTEVENTVIEWER_API SystemMetricsResponseDto GetProcesses();
        SMARTEVENTVIEWER_API SystemMetricsResponseDto GetSessions();

        // System Telemetry Metrics Endpoint (Full Legacy)
        SMARTEVENTVIEWER_API SystemMetricsResponseDto GetMetrics();
    };
}
