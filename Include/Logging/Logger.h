#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/Console.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/IO/File.h"
#include "System/IO/Path.h"

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#endif

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using Console = DotNetDupe::System::Console;
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    class SMARTEVENTVIEWER_API Logger {
    private:
        static CriticalSection s_logCs;
        static String s_logFilePath;
        static size_t s_maxFileSizeBytes;
        static size_t s_maxRolledFiles;
        static bool s_isInitialized;

        static String LogLevelToString(LogLevel level) {
            switch (level) {
                case LogLevel::Trace:   return "TRACE";
                case LogLevel::Debug:   return "DEBUG";
                case LogLevel::Info:    return "INFO";
                case LogLevel::Warning: return "WARN";
                case LogLevel::Error:   return "ERROR";
                case LogLevel::Fatal:   return "FATAL";
                default:                return "INFO";
            }
        }

        static unsigned long GetCurrentThreadIdNative() {
#if defined(_WIN32) || defined(_WIN64)
            return ::GetCurrentThreadId();
#else
            return 0;
#endif
        }

        static String GetTimestampIso() {
#if defined(_WIN32) || defined(_WIN64)
            SYSTEMTIME st;
            ::GetLocalTime(&st);
            return String::Format("{0:D4}-{1:D2}-{2:D2} {3:D2}:{4:D2}:{5:D2}.{6:D3}",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
            return "2026-08-06 00:00:00.000";
#endif
        }

        static long long GetFileSize(const String& sPath) {
#if defined(_WIN32) || defined(_WIN64)
            WIN32_FILE_ATTRIBUTE_DATA fad;
            if (::GetFileAttributesExA(sPath.GetRawString(), GetFileExInfoStandard, &fad)) {
                LARGE_INTEGER size;
                size.HighPart = fad.nFileSizeHigh;
                size.LowPart = fad.nFileSizeLow;
                return size.QuadPart;
            }
#endif
            return 0;
        }

        static void EnsureDirectoryCreated(const String& sDir) {
            if (sDir.IsEmpty()) return;
#if defined(_WIN32) || defined(_WIN64)
            ::_mkdir(sDir.GetRawString());
#endif
        }

        static void CheckAndRollFile() {
            if (!DotNetDupe::System::IO::File::Exists(s_logFilePath)) return;

            long long fileBytes = GetFileSize(s_logFilePath);
            if (fileBytes >= static_cast<long long>(s_maxFileSizeBytes)) {
                String directory = DotNetDupe::System::IO::Path::GetDirectoryName(s_logFilePath);
                String fileNameNoExt = DotNetDupe::System::IO::Path::GetFileNameWithoutExtension(s_logFilePath);
                String ext = DotNetDupe::System::IO::Path::GetExtension(s_logFilePath);

#if defined(_WIN32) || defined(_WIN64)
                SYSTEMTIME st;
                ::GetLocalTime(&st);
                String timestampSuffix = String::Format("_{0:D4}{1:D2}{2:D2}_{3:D2}{4:D2}{5:D2}",
                    st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
#else
                String timestampSuffix = "_rolled";
#endif
                String rolledPath = DotNetDupe::System::IO::Path::Combine({ directory, fileNameNoExt + timestampSuffix + ext });
                DotNetDupe::System::IO::File::Move(s_logFilePath, rolledPath);
            }
        }

    public:
        static void Initialize(const String& logFilePath = "logs/SmartEventViewerServer.log", size_t maxSizeBytes = 5 * 1024 * 1024, size_t maxRolledFiles = 10) {
            LockCS lock(s_logCs);
            s_logFilePath = logFilePath;
            s_maxFileSizeBytes = maxSizeBytes;
            s_maxRolledFiles = maxRolledFiles;

            String directory = DotNetDupe::System::IO::Path::GetDirectoryName(s_logFilePath);
            EnsureDirectoryCreated(directory);
            s_isInitialized = true;
        }

        static void Log(LogLevel level, const String& category, const String& message, double elapsedMs = -1.0) {
            LockCS lock(s_logCs);
            if (!s_isInitialized) {
                s_logFilePath = "logs/SmartEventViewerServer.log";
                String directory = DotNetDupe::System::IO::Path::GetDirectoryName(s_logFilePath);
                EnsureDirectoryCreated(directory);
                s_isInitialized = true;
            }

            CheckAndRollFile();

            String timestamp = GetTimestampIso();
            unsigned long threadId = GetCurrentThreadIdNative();
            String sLevel = LogLevelToString(level);

            String formattedLog;
            if (elapsedMs >= 0.0) {
                formattedLog = String::Format("[{0}] [{1}] [TID:{2}] [{3}] {4} (Elapsed: {5:F2} ms)",
                    timestamp, sLevel, threadId, category, message, elapsedMs);
            }
            else {
                formattedLog = String::Format("[{0}] [{1}] [TID:{2}] [{3}] {4}",
                    timestamp, sLevel, threadId, category, message);
            }

            Console::WriteLine(formattedLog);
            DotNetDupe::System::IO::File::AppendAllText(s_logFilePath, formattedLog + "\n");
        }

        static DotNetDupe::System::Collections::Generic::List<String> GetRecentLogLines(size_t maxLines = 200) {
            LockCS lock(s_logCs);
            DotNetDupe::System::Collections::Generic::List<String> result;
            if (!DotNetDupe::System::IO::File::Exists(s_logFilePath)) {
                return result;
            }

            try {
                auto allLines = DotNetDupe::System::IO::File::ReadAllLines(s_logFilePath);
                int totalCount = allLines.GetLength();
                int startIndex = (totalCount > static_cast<int>(maxLines)) ? (totalCount - static_cast<int>(maxLines)) : 0;

                for (int i = startIndex; i < totalCount; ++i) {
                    result.Add(allLines[i]);
                }
            }
            catch (...) {
            }
            return result;
        }

        static void Info(const String& category, const String& message, double elapsedMs = -1.0) {
            Log(LogLevel::Info, category, message, elapsedMs);
        }

        static void Warning(const String& category, const String& message, double elapsedMs = -1.0) {
            Log(LogLevel::Warning, category, message, elapsedMs);
        }

        static void Error(const String& category, const String& message, double elapsedMs = -1.0) {
            Log(LogLevel::Error, category, message, elapsedMs);
        }

        static void Debug(const String& category, const String& message, double elapsedMs = -1.0) {
            Log(LogLevel::Debug, category, message, elapsedMs);
        }
    };
}
