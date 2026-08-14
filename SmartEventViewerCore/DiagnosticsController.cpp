#include "pch.h"
#include "Core/DiagnosticsController.h"
#include "Core/EventsController.h"
#include "Logging/AppLoggerManager.h"

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

    static String SanitizeJsonEscapes(const String& input) {
        std::string s = input.GetRawString();
        std::string out;
        out.reserve(s.size() + 16);
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\') {
                if (i + 1 < s.size()) {
                    char next = s[i + 1];
                    if (next == '"' || next == '\\' || next == '/' || next == 'b' ||
                        next == 'f' || next == 'n' || next == 'r' || next == 't' || next == 'u') {
                        out.push_back('\\');
                        out.push_back(next);
                        i++;
                        continue;
                    }
                }
                out.push_back('\\');
                out.push_back('\\');
            } else {
                out.push_back(s[i]);
            }
        }
        return String(out.c_str());
    }

    static LogRecordDto ParseLogRecord(const String& rawLine) {
        String sanitized = SanitizeJsonEscapes(rawLine);
        try {
            return DotNetDupe::System::Text::Json::JsonSerializer::Deserialize<LogRecordDto>(sanitized);
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
