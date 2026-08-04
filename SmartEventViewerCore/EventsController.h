#pragma once

#include "Common.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "WebAppCore/Controllers/ControllerRouteBuilder.h"
#include "Platform/WinEventLogReader.h"
#include "SystemTelemetryProvider.h"
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
                        if (element.TryGetProperty(String("channels"), prop) || element.TryGetProperty(String("Channels"), prop)) dto.Channels = JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Read(prop);
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
            }
        }
    }
}

#include "Ai/LocalLlmEngine.h"

#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/Threading/Thread.h"
#include "System/Threading/AutoResetEvent.h"
#include "System/Collections/Generic/Dictionary.h"
#include "System/Collections/Concurrent/BlockingCollection.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::Collections::Generic;
using namespace DotNetDupe::System::Collections::Concurrent;
using namespace DotNetDupe::WebAppCore::Controllers;

namespace SmartEventViewer
{
    struct ChannelEventCache
    {
        unsigned long long LastEventCount{ 0 };
        EventLogResponseDto CachedResponse{};
    };

    class SMARTEVENTVIEWER_API EventsController : public ControllerBase
    {
    private:
        WinEventLogReader m_logReader{};

        // Thread-safe event channel cache
        static DotNetDupe::System::Threading::CriticalSection s_eventsCacheCs;
        static Dictionary<String, ChannelEventCache> s_eventsCache;

        static DotNetDupe::System::Collections::Generic::List<String> s_serverLogs;
        static DotNetDupe::System::Threading::CriticalSection s_serverLogsCs;

    public:
        EventsController();
        ~EventsController() override;

        static void Log(const String& sMessage);
        static DotNetDupe::System::Collections::Generic::List<String> GetServerLogList();

        // Returns strongly typed ChannelsResponseDto payload
        ChannelsResponseDto GetChannels();

        // Returns strongly typed EventLogResponseDto payload with pagination
        EventLogResponseDto GetEvents(const String& channelName, size_t page, size_t pageSize);
        EventLogResponseDto GetEvents(const String& channelName);
    };
}
