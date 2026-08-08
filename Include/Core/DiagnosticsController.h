#pragma once

#include "ViewerCommon.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "System/Text/Json/JsonSerializer.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    struct LogColumnFormatDto
    {
        String Key{};
        String HeaderName{};
        String Type{}; // "timestamp", "level", "string", "number"
        int WidthPx{ 150 };

        LogColumnFormatDto() = default;
        LogColumnFormatDto(const String& k, const String& h, const String& t, int w)
            : Key(k), HeaderName(h), Type(t), WidthPx(w) {}
    };

    struct LogFormatResponseDto
    {
        DotNetDupe::System::Collections::Generic::List<LogColumnFormatDto> Columns{};
    };

    struct LogRecordDto
    {
        String Timestamp{};
        String Level{};
        String ProcessId{};
        String ThreadId{};
        String Category{};
        String Message{};

        LogRecordDto() = default;
        LogRecordDto(const String& ts, const String& lvl, const String& pid, const String& tid, const String& cat, const String& msg)
            : Timestamp(ts), Level(lvl), ProcessId(pid), ThreadId(tid), Category(cat), Message(msg) {}
    };

    struct ServerLogsResponseDto
    {
        DotNetDupe::System::Collections::Generic::List<LogRecordDto> Records{};
    };

    class SMARTEVENTVIEWER_API DiagnosticsController : public DotNetDupe::WebAppCore::Controllers::ControllerBase
    {
    public:
        DiagnosticsController() = default;
        ~DiagnosticsController() override = default;

        // Log Endpoints
        LogFormatResponseDto GetLogFormat();
        ServerLogsResponseDto GetServerLogs();
    };
}

namespace DotNetDupe {
    namespace System {
        namespace Text {
            namespace Json {
                template <typename Enable>
                struct JsonConverter<SmartEventViewer::LogColumnFormatDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::LogColumnFormatDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("key"), JsonElement(value.Key));
                        obj.SetProperty(String("headerName"), JsonElement(value.HeaderName));
                        obj.SetProperty(String("type"), JsonElement(value.Type));
                        obj.SetProperty(String("widthPx"), JsonElement(static_cast<double>(value.WidthPx)));
                        return obj;
                    }
                    static SmartEventViewer::LogColumnFormatDto Read(const JsonElement& element) {
                        SmartEventViewer::LogColumnFormatDto dto;
                        JsonElement p;
                        if (element.TryGetProperty(String("key"), p)) dto.Key = p.GetString();
                        if (element.TryGetProperty(String("headerName"), p)) dto.HeaderName = p.GetString();
                        if (element.TryGetProperty(String("type"), p)) dto.Type = p.GetString();
                        if (element.TryGetProperty(String("widthPx"), p)) dto.WidthPx = p.GetInt32();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::LogFormatResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::LogFormatResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("columns"), JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::LogColumnFormatDto>>::Write(value.Columns));
                        return obj;
                    }
                    static SmartEventViewer::LogFormatResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::LogFormatResponseDto dto;
                        JsonElement p;
                        if (element.TryGetProperty(String("columns"), p)) dto.Columns = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::LogColumnFormatDto>>::Read(p);
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::LogRecordDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::LogRecordDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("timestamp"), JsonElement(value.Timestamp));
                        obj.SetProperty(String("level"), JsonElement(value.Level));
                        obj.SetProperty(String("processId"), JsonElement(value.ProcessId));
                        obj.SetProperty(String("threadId"), JsonElement(value.ThreadId));
                        obj.SetProperty(String("category"), JsonElement(value.Category));
                        obj.SetProperty(String("message"), JsonElement(value.Message));
                        return obj;
                    }
                    static SmartEventViewer::LogRecordDto Read(const JsonElement& element) {
                        SmartEventViewer::LogRecordDto dto;
                        JsonElement p;
                        if (element.TryGetProperty(String("timestamp"), p)) dto.Timestamp = p.GetString();
                        if (element.TryGetProperty(String("level"), p)) dto.Level = p.GetString();
                        if (element.TryGetProperty(String("processId"), p)) dto.ProcessId = p.GetString();
                        if (element.TryGetProperty(String("threadId"), p)) dto.ThreadId = p.GetString();
                        if (element.TryGetProperty(String("category"), p)) dto.Category = p.GetString();
                        if (element.TryGetProperty(String("message"), p)) dto.Message = p.GetString();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::ServerLogsResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::ServerLogsResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("records"), JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::LogRecordDto>>::Write(value.Records));
                        return obj;
                    }
                    static SmartEventViewer::ServerLogsResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::ServerLogsResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("records"), prop) || element.TryGetProperty(String("Records"), prop)) dto.Records = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::LogRecordDto>>::Read(prop);
                        return dto;
                    }
                };
            }
        }
    }
}
