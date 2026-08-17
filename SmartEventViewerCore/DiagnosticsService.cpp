#include "pch.h"
#include "Core/DiagnosticsService.h"
#include "Logging/AppLoggerManager.h"
#include "System/Text/StringBuilder.h"
#include "System/Text/Json/JsonSerializer.h"

namespace SmartEventViewer {
    LogFormatResponseDto DiagnosticsService::GetLogFormat() {
        LogFormatResponseDto dto;
        dto.Columns.Add(LogColumnFormatDto("timestamp", "Timestamp", "timestamp", 180));
        dto.Columns.Add(LogColumnFormatDto("level", "Level", "level", 80));
        dto.Columns.Add(LogColumnFormatDto("processId", "Process ID", "number", 90));
        dto.Columns.Add(LogColumnFormatDto("threadId", "Thread ID", "number", 90));
        dto.Columns.Add(LogColumnFormatDto("category", "Category", "string", 140));
        dto.Columns.Add(LogColumnFormatDto("message", "Message", "string", 450));
        return dto;
    }

    static bool IsJsonEscape(char c) {
        return (c == '"' || c == '\\' || c == '/' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'u');
    }

    static void AppendEscaped(DotNetDupe::System::Text::StringBuilder& sb, const String& input, int& i) {
        if (input[i] == '\\' && i + 1 < input.GetLength() && IsJsonEscape(input[i + 1])) {
            sb.Append('\\');
            sb.Append(input[i + 1]);
            i++;
        } else if (input[i] == '\\') {
            sb.Append("\\\\");
        } else {
            sb.Append(input[i]);
        }
    }

    static LogRecordDto ParseSingleLogLine(const String& rawLine) {
        DotNetDupe::System::Text::StringBuilder sb(rawLine.GetLength() + 16);
        for (int i = 0; i < rawLine.GetLength(); ++i) {
            AppendEscaped(sb, rawLine, i);
        }
        try {
            return DotNetDupe::System::Text::Json::JsonSerializer::Deserialize<LogRecordDto>(sb.ToString());
        } catch (...) {
            return LogRecordDto(DotNetDupe::System::DateTime::Now(), DotNetDupe::Extensions::Logging::LogLevel::Information, 0, 0, "SERVER", rawLine);
        }
    }

    ServerLogsResponseDto DiagnosticsService::GetServerLogs(int nMaxLines) {
        ServerLogsResponseDto dto;
        auto rawLogs = AppLoggerManager::GetRecentLogLines(nMaxLines);
        for (int i = rawLogs.GetCount() - 1; i >= 0; --i) {
            String line = rawLogs[i].Trim();
            if (!line.IsEmpty()) {
                dto.Records.Add(ParseSingleLogLine(line));
            }
        }
        return dto;
    }
}
