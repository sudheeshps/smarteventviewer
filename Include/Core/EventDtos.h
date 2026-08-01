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
        size_t Page{ 1 };
        size_t PageSize{ 20 };
        size_t TotalPages{ 0 };
        DotNetDupe::System::Collections::Generic::List<EventDto> Events{};
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
                        if (element.TryGetProperty(String("page"), prop)) dto.Page = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty(String("pageSize"), prop)) dto.PageSize = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty(String("totalPages"), prop)) dto.TotalPages = static_cast<size_t>(prop.GetDouble());
                        if (element.TryGetProperty(String("events"), prop)) dto.Events = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::EventDto>>::Read(prop);
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
                        obj.SetProperty(String("channel"), JsonElement(value.Channel));
                        obj.SetProperty(String("query"), JsonElement(value.Query));
                        obj.SetProperty(String("analysis"), JsonElement(value.Analysis));
                        obj.SetProperty(String("eventsAnalyzed"), JsonElement(static_cast<double>(value.EventsAnalyzed)));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeResponseDto dto;
                        JsonElement prop;
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
