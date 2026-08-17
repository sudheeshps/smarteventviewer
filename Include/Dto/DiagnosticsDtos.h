#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"
#include "System/DateTime.h"
#include "System/Text/Json/JsonSerializer.h"
#include "Extensions/Logging/ILogger.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    struct LogColumnFormatDto {
        String Key{};
        String HeaderName{};
        String Type{}; // "timestamp", "level", "string", "number"
        int WidthPx{ 150 };

        LogColumnFormatDto() = default;
        LogColumnFormatDto(const String& k, const String& h, const String& t, int w)
            : Key(k), HeaderName(h), Type(t), WidthPx(w) {}
    };

    struct LogFormatResponseDto {
        DotNetDupe::System::Collections::Generic::List<LogColumnFormatDto> Columns{};
    };

    struct LogRecordDto {
        DotNetDupe::System::DateTime Timestamp{};
        DotNetDupe::Extensions::Logging::LogLevel Level{};
        int ProcessId{ 0 };
        int ThreadId{ 0 };
        String Category{};
        String Message{};

        LogRecordDto() = default;
        LogRecordDto(DotNetDupe::System::DateTime ts, DotNetDupe::Extensions::Logging::LogLevel lvl, int pid, int tid, const String& cat, const String& msg)
            : Timestamp(ts), Level(lvl), ProcessId(pid), ThreadId(tid), Category(cat), Message(msg) {}
    };

    struct ServerLogsResponseDto {
        DotNetDupe::System::Collections::Generic::List<LogRecordDto> Records{};
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
                        obj.SetProperty("key", JsonElement(value.Key));
                        obj.SetProperty("headerName", JsonElement(value.HeaderName));
                        obj.SetProperty("type", JsonElement(value.Type));
                        obj.SetProperty("widthPx", JsonElement(static_cast<double>(value.WidthPx)));
                        return obj;
                    }
                    static SmartEventViewer::LogColumnFormatDto Read(const JsonElement& element) {
                        SmartEventViewer::LogColumnFormatDto dto;
                        JsonElement p;
                        if (element.TryGetProperty("key", p)) dto.Key = p.GetString();
                        if (element.TryGetProperty("headerName", p)) dto.HeaderName = p.GetString();
                        if (element.TryGetProperty("type", p)) dto.Type = p.GetString();
                        if (element.TryGetProperty("widthPx", p)) dto.WidthPx = p.GetInt32();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::LogFormatResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::LogFormatResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("columns", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::LogColumnFormatDto>>::Write(value.Columns));
                        return obj;
                    }
                    static SmartEventViewer::LogFormatResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::LogFormatResponseDto dto;
                        JsonElement p;
                        if (element.TryGetProperty("columns", p)) dto.Columns = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::LogColumnFormatDto>>::Read(p);
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::LogRecordDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::LogRecordDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("timestamp", JsonElement(value.Timestamp.ToString()));
                        
                        String sLevel = "INFO";
                        if (value.Level == DotNetDupe::Extensions::Logging::LogLevel::Trace) sLevel = "TRACE";
                        else if (value.Level == DotNetDupe::Extensions::Logging::LogLevel::Debug) sLevel = "DEBUG";
                        else if (value.Level == DotNetDupe::Extensions::Logging::LogLevel::Warning) sLevel = "WARN";
                        else if (value.Level == DotNetDupe::Extensions::Logging::LogLevel::Error) sLevel = "ERROR";
                        else if (value.Level == DotNetDupe::Extensions::Logging::LogLevel::Critical) sLevel = "FATAL";
                        
                        obj.SetProperty("level", JsonElement(sLevel));
                        obj.SetProperty("processId", JsonElement(static_cast<double>(value.ProcessId)));
                        obj.SetProperty("threadId", JsonElement(static_cast<double>(value.ThreadId)));
                        obj.SetProperty("category", JsonElement(value.Category));
                        obj.SetProperty("message", JsonElement(value.Message));
                        return obj;
                    }
                    static SmartEventViewer::LogRecordDto Read(const JsonElement& element) {
                        SmartEventViewer::LogRecordDto dto;
                        JsonElement p;
                        if (element.TryGetProperty("timestamp", p)) {
                            dto.Timestamp = DotNetDupe::System::DateTime::Parse(p.GetString()); 
                        }
                        if (element.TryGetProperty("level", p)) {
                            String s = p.GetString();
                            if (s == "TRACE") dto.Level = DotNetDupe::Extensions::Logging::LogLevel::Trace;
                            else if (s == "DEBUG") dto.Level = DotNetDupe::Extensions::Logging::LogLevel::Debug;
                            else if (s == "WARN" || s == "Warning") dto.Level = DotNetDupe::Extensions::Logging::LogLevel::Warning;
                            else if (s == "ERROR" || s == "Error") dto.Level = DotNetDupe::Extensions::Logging::LogLevel::Error;
                            else if (s == "FATAL" || s == "Critical") dto.Level = DotNetDupe::Extensions::Logging::LogLevel::Critical;
                            else dto.Level = DotNetDupe::Extensions::Logging::LogLevel::Information;
                        }
                        if (element.TryGetProperty("processId", p)) dto.ProcessId = static_cast<int>(p.GetDouble());
                        if (element.TryGetProperty("threadId", p)) dto.ThreadId = static_cast<int>(p.GetDouble());
                        if (element.TryGetProperty("category", p)) dto.Category = p.GetString();
                        if (element.TryGetProperty("message", p)) dto.Message = p.GetString();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::ServerLogsResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::ServerLogsResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("records", JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::LogRecordDto>>::Write(value.Records));
                        return obj;
                    }
                    static SmartEventViewer::ServerLogsResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::ServerLogsResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("records", prop) || element.TryGetProperty("Records", prop)) dto.Records = JsonConverter<DotNetDupe::System::Collections::Generic::List<SmartEventViewer::LogRecordDto>>::Read(prop);
                        return dto;
                    }
                };
            }
        }
    }
}
