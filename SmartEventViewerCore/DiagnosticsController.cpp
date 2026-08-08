#include "pch.h"
#include "Core/DiagnosticsController.h"
#include "Core/EventsController.h"
#include "Logging/AppLoggerManager.h"

namespace SmartEventViewer
{
    LogFormatResponseDto DiagnosticsController::GetLogFormat()
    {
        LogFormatResponseDto dto;
        dto.Columns.Add(LogColumnFormatDto("timestamp", "Timestamp", "timestamp", 180));
        dto.Columns.Add(LogColumnFormatDto("level", "Level", "level", 80));
        dto.Columns.Add(LogColumnFormatDto("processId", "Process ID", "number", 90));
        dto.Columns.Add(LogColumnFormatDto("threadId", "Thread ID", "number", 90));
        dto.Columns.Add(LogColumnFormatDto("category", "Category", "string", 140));
        dto.Columns.Add(LogColumnFormatDto("message", "Message", "string", 450));
        return dto;
    }

    ServerLogsResponseDto DiagnosticsController::GetServerLogs()
    {
        ServerLogsResponseDto dto;
        auto rawLogs = EventsController::GetServerLogList();
        if (rawLogs.GetCount() == 0)
        {
            rawLogs = AppLoggerManager::GetRecentLogLines(200);
        }

#if defined(_WIN32) || defined(_WIN64)
        SYSTEMTIME st;
        ::GetLocalTime(&st);
        char szBuf[64];
        snprintf(szBuf, sizeof(szBuf), "%04d-%02d-%02d %02d:%02d:%02d",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        String currentTs = szBuf;

        snprintf(szBuf, sizeof(szBuf), "%lu", ::GetCurrentProcessId());
        String currentPid = szBuf;

        snprintf(szBuf, sizeof(szBuf), "%lu", ::GetCurrentThreadId());
        String currentTid = szBuf;
#else
        String currentTs = "2026-08-08 18:50:00";
        String currentPid = "1000";
        String currentTid = "1";
#endif

        for (int i = 0; i < rawLogs.GetCount(); ++i)
        {
            const String& line = rawLogs[i];
            if (line.IsEmpty()) continue;

            String ts = currentTs;
            String lvl = "INFO";
            String pid = currentPid;
            String tid = currentTid;
            String cat = "SERVER";
            String msg = line;

            int firstBracketStart = line.IndexOf("[");
            int firstBracketEnd = line.IndexOf("]");

            // Extract ISO timestamp if present at beginning (e.g. 2026-08-08 18:45:10 [PID:123] [TID:456] [SERVER] message)
            if (line.GetLength() >= 19 && (line[4] == '-' || line[4] == '/') && line[10] == ' ')
            {
                ts = line.Substring(0, 19);
                msg = line.Substring(19).Trim();
            }

            int pidPos = line.IndexOf("[PID:");
            if (pidPos != -1)
            {
                int pidEnd = line.IndexOf("]", pidPos);
                if (pidEnd != -1)
                {
                    pid = line.Substring(pidPos + 5, pidEnd - (pidPos + 5));
                }
            }

            int tidPos = line.IndexOf("[TID:");
            if (tidPos != -1)
            {
                int tidEnd = line.IndexOf("]", tidPos);
                if (tidEnd != -1)
                {
                    tid = line.Substring(tidPos + 5, tidEnd - (tidPos + 5));
                }
            }

            if (firstBracketStart == 0 && firstBracketEnd != -1)
            {
                String token = line.Substring(1, firstBracketEnd - 1);
                cat = token;
                msg = line.Substring(firstBracketEnd + 1).Trim();
            }

            if (cat.Contains("ERROR") || cat.Contains("FAIL") || msg.Contains("ERROR") || msg.Contains("FAIL")) lvl = "ERROR";
            else if (cat.Contains("WARN") || msg.Contains("WARN")) lvl = "WARN";
            else if (cat.Contains("AI_ENGINE") || cat.Contains("LLM")) lvl = "DEBUG";

            dto.Records.Add(LogRecordDto(ts, lvl, pid, tid, cat, msg));
        }
        return dto;
    }
}
