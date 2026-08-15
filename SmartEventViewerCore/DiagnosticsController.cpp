#include "pch.h"
#include "Core/DiagnosticsController.h"
#include "Core/EventsController.h"
#include "Logging/AppLoggerManager.h"

#include "System/Text/StringBuilder.h"

namespace SmartEventViewer {
    LogFormatResponseDto DiagnosticsController::GetLogFormat() {
        LogFormatResponseDto dto;
        dto.Columns.Add(LogColumnFormatDto("timestamp", "Timestamp", "timestamp", 180));
        dto.Columns.Add(LogColumnFormatDto("level", "Level", "level", 80));
        dto.Columns.Add(LogColumnFormatDto("processId", "Process ID", "number", 90));
        dto.Columns.Add(LogColumnFormatDto("threadId", "Thread ID", "number", 90));
        dto.Columns.Add(LogColumnFormatDto("category", "Category", "string", 140));
        dto.Columns.Add(LogColumnFormatDto("message", "Message", "string", 450));
        return dto;
    }

    static bool IsJsonEscapeChar(char c) {
        return (c == '"' || c == '\\' || c == '/' || c == 'b' ||
                c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'u');
    }

    static void AppendEscapedChar(DotNetDupe::System::Text::StringBuilder& sb, const String& input, int& i) {
        if (input[i] == '\\') {
            if (i + 1 < input.GetLength() && IsJsonEscapeChar(input[i + 1])) {
                sb.Append('\\');
                sb.Append(input[i + 1]);
                i++;
                return;
            }
            sb.Append("\\\\");
        } else {
            sb.Append(input[i]);
        }
    }

    static String SanitizeJsonEscapes(const String& input) {
        DotNetDupe::System::Text::StringBuilder sb(input.GetLength() + 16);
        for (int i = 0; i < input.GetLength(); ++i) {
            AppendEscapedChar(sb, input, i);
        }
        return sb.ToString();
    }

    static LogRecordDto ParseLogRecord(const String& rawLine) {
        String sanitized = SanitizeJsonEscapes(rawLine);
        try {
            return DotNetDupe::System::Text::Json::JsonSerializer::Deserialize<LogRecordDto>(sanitized);
        } catch (const DotNetDupe::System::Exception&) {
            return LogRecordDto(DotNetDupe::System::DateTime::Now(), DotNetDupe::Extensions::Logging::LogLevel::Information, 0, 0, "SERVER", rawLine);
        } catch (...) {
            return LogRecordDto(DotNetDupe::System::DateTime::Now(), DotNetDupe::Extensions::Logging::LogLevel::Information, 0, 0, "SERVER", rawLine);
        }
    }

    ServerLogsResponseDto DiagnosticsController::GetServerLogs() {
        ServerLogsResponseDto dto;
        auto rawLogs = AppLoggerManager::GetRecentLogLines(500);

        for (int i = rawLogs.GetCount() - 1; i >= 0; --i) {
            String line = rawLogs[i].Trim();
            if (!line.IsEmpty()) {
                dto.Records.Add(ParseLogRecord(line));
            }
        }
        return dto;
    }
}
