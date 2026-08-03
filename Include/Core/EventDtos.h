#pragma once

#include "../Common.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"
#include "System/Text/Json/JsonSerializer.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    struct EventDto
    {
        size_t Index{ 0 };
        unsigned int Id{ 0 };
        String Level{};
        String Risk{};
        String Provider{};
        String Time{};
        String Message{};
        String RawXml{};

        EventDto() = default;
        EventDto(const EventDto&) = default;
        EventDto& operator=(const EventDto&) = default;
    };

    struct ChannelsResponseDto
    {
        StringList Channels{};
    };

    struct EventLogResponseDto
    {
        String Channel{};
        unsigned long long TotalCount{ 0 };
        unsigned long long CriticalCount{ 0 };
        unsigned long long ErrorCount{ 0 };
        unsigned long long WarningCount{ 0 };
        unsigned long long InfoCount{ 0 };
        unsigned long long VerboseCount{ 0 };
        size_t Page{ 1 };
        size_t PageSize{ 20 };
        size_t TotalPages{ 0 };
        DotNetDupe::System::Collections::Generic::List<EventDto> Events{};
    };

    struct ProcessResourceDto
    {
        unsigned long ProcessId{ 0 };
        String Name{};
        String Path{};
        String CommandLine{};
        double CpuUsagePercent{ 0.0 };
        unsigned long long MemoryUsageMB{ 0 };
        double DiskIoKBps{ 0.0 };
        double DiskReadKBps{ 0.0 };
        double DiskWriteKBps{ 0.0 };

        ProcessResourceDto() = default;
        ProcessResourceDto(const ProcessResourceDto&) = default;
        ProcessResourceDto& operator=(const ProcessResourceDto&) = default;
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
        String UserClass{}; // "Admin", "Normal", "Guest", "System"
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
    };

    struct AnalyzeRequestDto
    {
        String Channel{};
        String Query{};

        AnalyzeRequestDto() = default;
        AnalyzeRequestDto(const AnalyzeRequestDto&) = default;
        AnalyzeRequestDto& operator=(const AnalyzeRequestDto&) = default;
    };

    struct AnalyzeResponseDto
    {
        String TaskId{};
        String Status{}; // "PENDING", "PROCESSING", "COMPLETED", "FAILED"
        String ProgressMessage{}; // Push notification status: e.g. "Starting analysis...", "Reading logs from channel Security...", "Ingesting logs...", "Analyzing threat vectors..."
        String Channel{};
        String Query{};
        String Analysis{};
        unsigned long long EventsAnalyzed{ 0 };

        AnalyzeResponseDto() = default;
        AnalyzeResponseDto(const AnalyzeResponseDto&) = default;
        AnalyzeResponseDto& operator=(const AnalyzeResponseDto&) = default;
    };
}

namespace DotNetDupe {
    namespace System {
        namespace Text {
            namespace Json {
                template <typename Enable>
                struct JsonConverter<SmartEventViewer::EventDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::EventDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("index"), JsonElement(static_cast<double>(value.Index)));
                        obj.SetProperty(String("id"), JsonElement(static_cast<double>(value.Id)));
                        obj.SetProperty(String("level"), JsonElement(value.Level));
                        obj.SetProperty(String("risk"), JsonElement(value.Risk));
                        obj.SetProperty(String("provider"), JsonElement(value.Provider));
                        obj.SetProperty(String("time"), JsonElement(value.Time));
                        obj.SetProperty(String("message"), JsonElement(value.Message));
                        obj.SetProperty(String("rawXml"), JsonElement(value.RawXml));
                        return obj;
                    }
                    static SmartEventViewer::EventDto Read(const JsonElement& element) {
                        SmartEventViewer::EventDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("index"), prop)) dto.Index = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty(String("id"), prop)) dto.Id = static_cast<unsigned int>(prop.GetDouble());
                        if (element.TryGetProperty(String("level"), prop)) dto.Level = prop.GetString();
                        if (element.TryGetProperty(String("risk"), prop)) dto.Risk = prop.GetString();
                        if (element.TryGetProperty(String("provider"), prop)) dto.Provider = prop.GetString();
                        if (element.TryGetProperty(String("time"), prop)) dto.Time = prop.GetString();
                        if (element.TryGetProperty(String("message"), prop)) dto.Message = prop.GetString();
                        if (element.TryGetProperty(String("rawXml"), prop)) dto.RawXml = prop.GetString();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::ChannelsResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::ChannelsResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("channels"), JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Write(value.Channels));
                        return obj;
                    }
                    static SmartEventViewer::ChannelsResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::ChannelsResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("channels"), prop)) dto.Channels = JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Read(prop);
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::EventLogResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::EventLogResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("channel"), JsonElement(value.Channel));
                        obj.SetProperty(String("totalCount"), JsonElement(static_cast<double>(value.TotalCount)));
                        obj.SetProperty(String("criticalCount"), JsonElement(static_cast<double>(value.CriticalCount)));
                        obj.SetProperty(String("errorCount"), JsonElement(static_cast<double>(value.ErrorCount)));
                        obj.SetProperty(String("warningCount"), JsonElement(static_cast<double>(value.WarningCount)));
                        obj.SetProperty(String("infoCount"), JsonElement(static_cast<double>(value.InfoCount)));
                        obj.SetProperty(String("verboseCount"), JsonElement(static_cast<double>(value.VerboseCount)));
                        obj.SetProperty(String("page"), JsonElement(static_cast<double>(value.Page)));
                        obj.SetProperty(String("pageSize"), JsonElement(static_cast<double>(value.PageSize)));
                        obj.SetProperty(String("totalPages"), JsonElement(static_cast<double>(value.TotalPages)));
                        obj.SetProperty(String("events"), JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::EventDto>>::Write(value.Events));
                        return obj;
                    }
                    static SmartEventViewer::EventLogResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::EventLogResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("channel"), prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty(String("totalCount"), prop)) dto.TotalCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("criticalCount"), prop)) dto.CriticalCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("errorCount"), prop)) dto.ErrorCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("warningCount"), prop)) dto.WarningCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("infoCount"), prop)) dto.InfoCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("verboseCount"), prop)) dto.VerboseCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty(String("page"), prop)) dto.Page = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty(String("pageSize"), prop)) dto.PageSize = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty(String("totalPages"), prop)) dto.TotalPages = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty(String("events"), prop)) dto.Events = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::EventDto>>::Read(prop);
                        return dto;
                    }
                };

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
                        obj.SetProperty(String("diskIoKBps"), JsonElement(value.DiskIoKBps));
                        obj.SetProperty(String("diskReadKBps"), JsonElement(value.DiskReadKBps));
                        obj.SetProperty(String("diskWriteKBps"), JsonElement(value.DiskWriteKBps));
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
                        if (element.TryGetProperty(String("diskIoKBps"), prop)) dto.DiskIoKBps = prop.GetDouble();
                        if (element.TryGetProperty(String("diskReadKBps"), prop)) dto.DiskReadKBps = prop.GetDouble();
                        if (element.TryGetProperty(String("diskWriteKBps"), prop)) dto.DiskWriteKBps = prop.GetDouble();
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
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::AnalyzeRequestDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::AnalyzeRequestDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("channel"), JsonElement(value.Channel));
                        obj.SetProperty(String("query"), JsonElement(value.Query));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeRequestDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeRequestDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("channel"), prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty(String("query"), prop)) dto.Query = prop.GetString();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::AnalyzeResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::AnalyzeResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("taskId"), JsonElement(value.TaskId));
                        obj.SetProperty(String("status"), JsonElement(value.Status));
                        obj.SetProperty(String("progressMessage"), JsonElement(value.ProgressMessage));
                        obj.SetProperty(String("channel"), JsonElement(value.Channel));
                        obj.SetProperty(String("query"), JsonElement(value.Query));
                        obj.SetProperty(String("analysis"), JsonElement(value.Analysis));
                        obj.SetProperty(String("eventsAnalyzed"), JsonElement(static_cast<double>(value.EventsAnalyzed)));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("taskId"), prop)) dto.TaskId = prop.GetString();
                        if (element.TryGetProperty(String("status"), prop)) dto.Status = prop.GetString();
                        if (element.TryGetProperty(String("progressMessage"), prop)) dto.ProgressMessage = prop.GetString();
                        if (element.TryGetProperty(String("channel"), prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty(String("query"), prop)) dto.Query = prop.GetString();
                        if (element.TryGetProperty(String("analysis"), prop)) dto.Analysis = prop.GetString();
                        if (element.TryGetProperty(String("eventsAnalyzed"), prop)) dto.EventsAnalyzed = static_cast<unsigned long long>(prop.GetDouble());
                        return dto;
                    }
                };
            }
        }
    }
}
