#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"
#include "System/Text/Json/JsonSerializer.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    struct EventDto {
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

    struct ChannelsResponseDto {
        StringList Channels{};
    };

    struct EventSummaryResponseDto {
        String Channel{};
        unsigned long long TotalCount{ 0 };
        unsigned long long CriticalCount{ 0 };
        unsigned long long ErrorCount{ 0 };
        unsigned long long WarningCount{ 0 };
        unsigned long long InfoCount{ 0 };
        unsigned long long VerboseCount{ 0 };
    };

    struct EventLogResponseDto {
        String Channel{};
        unsigned long long TotalCount{ 0 };
        size_t Page{ 1 };
        size_t PageSize{ 20 };
        size_t TotalPages{ 0 };
        DotNetDupe::System::Collections::Generic::List<EventDto> Events{};
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
                        obj.SetProperty("index", JsonElement(static_cast<double>(value.Index)));
                        obj.SetProperty("id", JsonElement(static_cast<double>(value.Id)));
                        obj.SetProperty("level", JsonElement(value.Level));
                        obj.SetProperty("risk", JsonElement(value.Risk));
                        obj.SetProperty("provider", JsonElement(value.Provider));
                        obj.SetProperty("time", JsonElement(value.Time));
                        obj.SetProperty("message", JsonElement(value.Message));
                        obj.SetProperty("rawXml", JsonElement(value.RawXml));
                        return obj;
                    }
                    static SmartEventViewer::EventDto Read(const JsonElement& element) {
                        SmartEventViewer::EventDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("index", prop)) dto.Index = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty("id", prop)) dto.Id = static_cast<unsigned int>(prop.GetDouble());
                        if (element.TryGetProperty("level", prop)) dto.Level = prop.GetString();
                        if (element.TryGetProperty("risk", prop)) dto.Risk = prop.GetString();
                        if (element.TryGetProperty("provider", prop)) dto.Provider = prop.GetString();
                        if (element.TryGetProperty("time", prop)) dto.Time = prop.GetString();
                        if (element.TryGetProperty("message", prop)) dto.Message = prop.GetString();
                        if (element.TryGetProperty("rawXml", prop)) dto.RawXml = prop.GetString();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::ChannelsResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::ChannelsResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("channels", JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Write(value.Channels));
                        return obj;
                    }
                    static SmartEventViewer::ChannelsResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::ChannelsResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("channels", prop) || element.TryGetProperty("Channels", prop)) dto.Channels = JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Read(prop);
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::EventSummaryResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::EventSummaryResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("channel", JsonElement(value.Channel));
                        obj.SetProperty("totalCount", JsonElement(static_cast<double>(value.TotalCount)));
                        obj.SetProperty("criticalCount", JsonElement(static_cast<double>(value.CriticalCount)));
                        obj.SetProperty("errorCount", JsonElement(static_cast<double>(value.ErrorCount)));
                        obj.SetProperty("warningCount", JsonElement(static_cast<double>(value.WarningCount)));
                        obj.SetProperty("infoCount", JsonElement(static_cast<double>(value.InfoCount)));
                        obj.SetProperty("verboseCount", JsonElement(static_cast<double>(value.VerboseCount)));
                        return obj;
                    }
                    static SmartEventViewer::EventSummaryResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::EventSummaryResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("channel", prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty("totalCount", prop)) dto.TotalCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("criticalCount", prop)) dto.CriticalCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("errorCount", prop)) dto.ErrorCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("warningCount", prop)) dto.WarningCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("infoCount", prop)) dto.InfoCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("verboseCount", prop)) dto.VerboseCount = static_cast<unsigned long long>(prop.GetDouble());
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::EventLogResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::EventLogResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("channel", JsonElement(value.Channel));
                        obj.SetProperty("totalCount", JsonElement(static_cast<double>(value.TotalCount)));
                        obj.SetProperty("page", JsonElement(static_cast<double>(value.Page)));
                        obj.SetProperty("pageSize", JsonElement(static_cast<double>(value.PageSize)));
                        obj.SetProperty("totalPages", JsonElement(static_cast<double>(value.TotalPages)));
                        obj.SetProperty("events", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::EventDto>>::Write(value.Events));
                        return obj;
                    }
                    static SmartEventViewer::EventLogResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::EventLogResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("channel", prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty("totalCount", prop)) dto.TotalCount = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("page", prop)) dto.Page = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty("pageSize", prop)) dto.PageSize = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty("totalPages", prop)) dto.TotalPages = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty("events", prop)) dto.Events = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::EventDto>>::Read(prop);
                        return dto;
                    }
                };
            }
        }
    }
}
