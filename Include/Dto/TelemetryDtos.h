#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"
#include "System/Text/Json/JsonSerializer.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    struct ProcessResourceDto {
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

    struct RdpSessionDto {
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

    struct UserSessionDto {
        String Username{};
        String Privilege{};
        String LoginTimestamp{};
        String LogoutTimestamp{};
        bool IsActive{ true };

        UserSessionDto() = default;
        UserSessionDto(const UserSessionDto&) = default;
        UserSessionDto& operator=(const UserSessionDto&) = default;
    };

    struct UserPrincipalDto {
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

    struct ServiceInfoDto {
        String ServiceName{};
        String DisplayName{};
        String Status{};
        String StartType{};
        int ProcessId{ 0 };

        ServiceInfoDto() = default;
        ServiceInfoDto(const String& name, const String& display, const String& status, const String& startType, int pid)
            : ServiceName(name), DisplayName(display), Status(status), StartType(startType), ProcessId(pid) {}
    };

    struct ServicesResponseDto {
        DotNetDupe::System::Collections::Generic::List<ServiceInfoDto> Services{};
    };

    struct SystemMetricsResponseDto {
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
                        obj.SetProperty("processId", JsonElement(static_cast<double>(value.ProcessId)));
                        obj.SetProperty("name", JsonElement(value.Name));
                        obj.SetProperty("path", JsonElement(value.Path));
                        obj.SetProperty("commandLine", JsonElement(value.CommandLine));
                        obj.SetProperty("cpuUsagePercent", JsonElement(value.CpuUsagePercent));
                        obj.SetProperty("memoryUsageMB", JsonElement(static_cast<double>(value.MemoryUsageMB)));
                        obj.SetProperty("networkReadBytes", JsonElement(static_cast<double>(value.NetworkReadBytes)));
                        obj.SetProperty("networkWriteBytes", JsonElement(static_cast<double>(value.NetworkWriteBytes)));
                        obj.SetProperty("openPorts", JsonElement(value.OpenPorts));
                        obj.SetProperty("connectionEstablished", JsonElement(value.ConnectionEstablished));
                        return obj;
                    }
                    static SmartEventViewer::ProcessResourceDto Read(const JsonElement& element) {
                        SmartEventViewer::ProcessResourceDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("processId", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.ProcessId = static_cast<unsigned long>(prop.GetDouble());
                        if (element.TryGetProperty("name", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Name = prop.GetString();
                        if (element.TryGetProperty("path", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Path = prop.GetString();
                        if (element.TryGetProperty("commandLine", prop) && prop.GetValueKind() == JsonValueKind::String) dto.CommandLine = prop.GetString();
                        if (element.TryGetProperty("cpuUsagePercent", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.CpuUsagePercent = prop.GetDouble();
                        if (element.TryGetProperty("memoryUsageMB", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.MemoryUsageMB = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("networkReadBytes", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.NetworkReadBytes = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("networkWriteBytes", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.NetworkWriteBytes = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("openPorts", prop) && prop.GetValueKind() == JsonValueKind::String) dto.OpenPorts = prop.GetString();
                        if (element.TryGetProperty("connectionEstablished", prop) && (prop.GetValueKind() == JsonValueKind::True || prop.GetValueKind() == JsonValueKind::False)) dto.ConnectionEstablished = prop.GetBoolean();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::UserSessionDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::UserSessionDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("username", JsonElement(value.Username));
                        obj.SetProperty("privilege", JsonElement(value.Privilege));
                        obj.SetProperty("loginTimestamp", JsonElement(value.LoginTimestamp));
                        obj.SetProperty("logoutTimestamp", JsonElement(value.LogoutTimestamp));
                        obj.SetProperty("isActive", JsonElement(value.IsActive));
                        return obj;
                    }
                    static SmartEventViewer::UserSessionDto Read(const JsonElement& element) {
                        SmartEventViewer::UserSessionDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("username", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Username = prop.GetString();
                        if (element.TryGetProperty("privilege", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Privilege = prop.GetString();
                        if (element.TryGetProperty("loginTimestamp", prop) && prop.GetValueKind() == JsonValueKind::String) dto.LoginTimestamp = prop.GetString();
                        if (element.TryGetProperty("logoutTimestamp", prop) && prop.GetValueKind() == JsonValueKind::String) dto.LogoutTimestamp = prop.GetString();
                        if (element.TryGetProperty("isActive", prop) && (prop.GetValueKind() == JsonValueKind::True || prop.GetValueKind() == JsonValueKind::False)) dto.IsActive = prop.GetBoolean();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::UserPrincipalDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::UserPrincipalDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("username", JsonElement(value.Username));
                        obj.SetProperty("domain", JsonElement(value.Domain));
                        obj.SetProperty("sidOrUid", JsonElement(value.SidOrUid));
                        obj.SetProperty("userClass", JsonElement(value.UserClass));
                        obj.SetProperty("isDisabled", JsonElement(value.IsDisabled));
                        obj.SetProperty("isAccountLocked", JsonElement(value.IsAccountLocked));
                        obj.SetProperty("groups", JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Write(value.Groups));
                        obj.SetProperty("permissions", JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Write(value.Permissions));
                        return obj;
                    }
                    static SmartEventViewer::UserPrincipalDto Read(const JsonElement& element) {
                        SmartEventViewer::UserPrincipalDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("username", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Username = prop.GetString();
                        if (element.TryGetProperty("domain", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Domain = prop.GetString();
                        if (element.TryGetProperty("sidOrUid", prop) && prop.GetValueKind() == JsonValueKind::String) dto.SidOrUid = prop.GetString();
                        if (element.TryGetProperty("userClass", prop) && prop.GetValueKind() == JsonValueKind::String) dto.UserClass = prop.GetString();
                        if (element.TryGetProperty("isDisabled", prop) && (prop.GetValueKind() == JsonValueKind::True || prop.GetValueKind() == JsonValueKind::False)) dto.IsDisabled = prop.GetBoolean();
                        if (element.TryGetProperty("isAccountLocked", prop) && (prop.GetValueKind() == JsonValueKind::True || prop.GetValueKind() == JsonValueKind::False)) dto.IsAccountLocked = prop.GetBoolean();
                        if (element.TryGetProperty("groups", prop) && prop.GetValueKind() == JsonValueKind::Array) dto.Groups = JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Read(prop);
                        if (element.TryGetProperty("permissions", prop) && prop.GetValueKind() == JsonValueKind::Array) dto.Permissions = JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Read(prop);
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::RdpSessionDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::RdpSessionDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("sessionId", JsonElement(static_cast<double>(value.SessionId)));
                        obj.SetProperty("sessionName", JsonElement(value.SessionName));
                        obj.SetProperty("userName", JsonElement(value.UserName));
                        obj.SetProperty("domainName", JsonElement(value.DomainName));
                        obj.SetProperty("clientName", JsonElement(value.ClientName));
                        obj.SetProperty("clientIpAddress", JsonElement(value.ClientIpAddress));
                        obj.SetProperty("state", JsonElement(value.State));
                        obj.SetProperty("isRdpSession", JsonElement(value.IsRdpSession));
                        return obj;
                    }
                    static SmartEventViewer::RdpSessionDto Read(const JsonElement& element) {
                        SmartEventViewer::RdpSessionDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("sessionId", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.SessionId = static_cast<unsigned long>(prop.GetDouble());
                        if (element.TryGetProperty("sessionName", prop) && prop.GetValueKind() == JsonValueKind::String) dto.SessionName = prop.GetString();
                        if (element.TryGetProperty("userName", prop) && prop.GetValueKind() == JsonValueKind::String) dto.UserName = prop.GetString();
                        if (element.TryGetProperty("domainName", prop) && prop.GetValueKind() == JsonValueKind::String) dto.DomainName = prop.GetString();
                        if (element.TryGetProperty("clientName", prop) && prop.GetValueKind() == JsonValueKind::String) dto.ClientName = prop.GetString();
                        if (element.TryGetProperty("clientIpAddress", prop) && prop.GetValueKind() == JsonValueKind::String) dto.ClientIpAddress = prop.GetString();
                        if (element.TryGetProperty("state", prop) && prop.GetValueKind() == JsonValueKind::String) dto.State = prop.GetString();
                        if (element.TryGetProperty("isRdpSession", prop) && (prop.GetValueKind() == JsonValueKind::True || prop.GetValueKind() == JsonValueKind::False)) dto.IsRdpSession = prop.GetBoolean();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::SystemMetricsResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::SystemMetricsResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("cpuUsagePercent", JsonElement(value.CpuUsagePercent));
                        obj.SetProperty("memoryUsagePercent", JsonElement(value.MemoryUsagePercent));
                        obj.SetProperty("memoryUsedMB", JsonElement(static_cast<double>(value.MemoryUsedMB)));
                        obj.SetProperty("memoryTotalMB", JsonElement(static_cast<double>(value.MemoryTotalMB)));
                        obj.SetProperty("diskUsagePercent", JsonElement(value.DiskUsagePercent));
                        obj.SetProperty("diskReadMBps", JsonElement(value.DiskReadMBps));
                        obj.SetProperty("diskWriteMBps", JsonElement(value.DiskWriteMBps));
                        obj.SetProperty("networkUsageMbps", JsonElement(value.NetworkUsageMbps));
                        obj.SetProperty("topProcesses", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::ProcessResourceDto>>::Write(value.TopProcesses));
                        obj.SetProperty("activeUserSessions", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserSessionDto>>::Write(value.ActiveUserSessions));
                        obj.SetProperty("expiredUserSessions", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserSessionDto>>::Write(value.ExpiredUserSessions));
                        obj.SetProperty("systemUsers", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserPrincipalDto>>::Write(value.SystemUsers));
                        obj.SetProperty("rdpSessions", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::RdpSessionDto>>::Write(value.RdpSessions));
                        return obj;
                    }
                    static SmartEventViewer::SystemMetricsResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::SystemMetricsResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("cpuUsagePercent", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.CpuUsagePercent = prop.GetDouble();
                        if (element.TryGetProperty("memoryUsagePercent", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.MemoryUsagePercent = prop.GetDouble();
                        if (element.TryGetProperty("memoryUsedMB", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.MemoryUsedMB = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("memoryTotalMB", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.MemoryTotalMB = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("diskUsagePercent", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.DiskUsagePercent = prop.GetDouble();
                        if (element.TryGetProperty("diskReadMBps", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.DiskReadMBps = prop.GetDouble();
                        if (element.TryGetProperty("diskWriteMBps", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.DiskWriteMBps = prop.GetDouble();
                        if (element.TryGetProperty("networkUsageMbps", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.NetworkUsageMbps = prop.GetDouble();
                        if (element.TryGetProperty("topProcesses", prop) && prop.GetValueKind() == JsonValueKind::Array) dto.TopProcesses = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::ProcessResourceDto>>::Read(prop);
                        if (element.TryGetProperty("activeUserSessions", prop) && prop.GetValueKind() == JsonValueKind::Array) dto.ActiveUserSessions = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserSessionDto>>::Read(prop);
                        if (element.TryGetProperty("expiredUserSessions", prop) && prop.GetValueKind() == JsonValueKind::Array) dto.ExpiredUserSessions = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserSessionDto>>::Read(prop);
                        if (element.TryGetProperty("systemUsers", prop) && prop.GetValueKind() == JsonValueKind::Array) dto.SystemUsers = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::UserPrincipalDto>>::Read(prop);
                        if (element.TryGetProperty("rdpSessions", prop) && prop.GetValueKind() == JsonValueKind::Array) dto.RdpSessions = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::RdpSessionDto>>::Read(prop);
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::ServiceInfoDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::ServiceInfoDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("serviceName", JsonElement(value.ServiceName));
                        obj.SetProperty("displayName", JsonElement(value.DisplayName));
                        obj.SetProperty("status", JsonElement(value.Status));
                        obj.SetProperty("startType", JsonElement(value.StartType));
                        obj.SetProperty("processId", JsonElement(static_cast<double>(value.ProcessId)));
                        return obj;
                    }
                    static SmartEventViewer::ServiceInfoDto Read(const JsonElement& element) {
                        SmartEventViewer::ServiceInfoDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("serviceName", prop) && prop.GetValueKind() == JsonValueKind::String) dto.ServiceName = prop.GetString();
                        if (element.TryGetProperty("displayName", prop) && prop.GetValueKind() == JsonValueKind::String) dto.DisplayName = prop.GetString();
                        if (element.TryGetProperty("status", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Status = prop.GetString();
                        if (element.TryGetProperty("startType", prop) && prop.GetValueKind() == JsonValueKind::String) dto.StartType = prop.GetString();
                        if (element.TryGetProperty("processId", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.ProcessId = static_cast<int>(prop.GetDouble());
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::ServicesResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::ServicesResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("services", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::ServiceInfoDto>>::Write(value.Services));
                        return obj;
                    }
                    static SmartEventViewer::ServicesResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::ServicesResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("services", prop) && prop.GetValueKind() == JsonValueKind::Array) dto.Services = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::ServiceInfoDto>>::Read(prop);
                        return dto;
                    }
                };
            }
        }
    }
}
