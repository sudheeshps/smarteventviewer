#include "pch.h"
#include "Logging/AppLoggerManager.h"
#include "System/IO/File.h"
#include "System/IO/Path.h"
#include "System/IO/Directory.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::IO;
using namespace DotNetDupe::System::Threading;
using namespace DotNetDupe::Extensions::Logging;

namespace SmartEventViewer {
    DotNetDupe::System::SmartPointer<LoggerFactory> AppLoggerManager::s_loggerFactory = nullptr;
    String AppLoggerManager::s_logFilePath = "logs/SmartEventViewerServer.log";
    bool AppLoggerManager::s_isInitialized = false;
    static CriticalSection s_logFileCs;

    void AppLoggerManager::Initialize(const String& logFilePath, LogLevel minLevel) {
        s_logFilePath = Path::GetFullPath(logFilePath);
        String directory = Path::GetDirectoryName(s_logFilePath);
        if (!directory.IsEmpty()) {
            try {
                if (!Directory::Exists(directory)) {
                    Directory::CreateDirectory(directory);
                }
            } catch (...) {
            }
        }

        s_loggerFactory = DotNetDupe::System::SmartPointer<LoggerFactory>::NewShared();

        // Register FileLoggerProvider from DotNetDupe framework with absolute file path
        try {
            LoggerConfiguration config;
            config.MinLevel = minLevel;
            config.IsJsonFormat = false;
            config.PlainTextFormat = "{Timestamp} [PID:{ProcessId}] [TID:{ThreadId}] [{Level}] [{Category}] {Message}";
            config.TimestampFormat = "%Y-%m-%d %H:%M:%S";
            config.FilePath = s_logFilePath;
            config.Rollover.EnableRollover = true;
            config.Rollover.MaxFileSizeInBytes = 5 * 1024 * 1024;
            config.Rollover.MaxBackupFiles = 5;

            auto fileProvider = DotNetDupe::System::SmartPointer<FileLoggerProvider>::NewShared(config);
            s_loggerFactory->AddProvider(fileProvider);
        } catch (...) {
        }

        s_isInitialized = true;
    }

    DotNetDupe::System::SmartPointer<ILogger> AppLoggerManager::GetLogger(const String& categoryName) {
        if (!s_isInitialized || s_loggerFactory.IsNull()) {
            Initialize();
        }
        return s_loggerFactory->CreateLogger(categoryName);
    }

    void AppLoggerManager::Info(const String& category, const String& message) {
        auto logger = GetLogger(category);
        if (!logger.IsNull()) {
            logger->Log(LogLevel::Information, message);
        }
    }

    void AppLoggerManager::Warning(const String& category, const String& message) {
        auto logger = GetLogger(category);
        if (!logger.IsNull()) {
            logger->Log(LogLevel::Warning, message);
        }
    }

    void AppLoggerManager::Error(const String& category, const String& message) {
        auto logger = GetLogger(category);
        if (!logger.IsNull()) {
            logger->Log(LogLevel::Error, message);
        }
    }

    void AppLoggerManager::Debug(const String& category, const String& message) {
        auto logger = GetLogger(category);
        if (!logger.IsNull()) {
            logger->Log(LogLevel::Debug, message);
        }
    }

    DotNetDupe::System::Collections::Generic::List<String> AppLoggerManager::GetRecentLogLines(size_t maxLines) {
        Lock<CriticalSection> lock(s_logFileCs);
        DotNetDupe::System::Collections::Generic::List<String> result;
        if (!File::Exists(s_logFilePath)) {
            return result;
        }

        try {
            auto allLines = File::ReadAllLines(s_logFilePath);
            int totalCount = allLines.GetLength();
            int startIndex = (totalCount > static_cast<int>(maxLines)) ? (totalCount - static_cast<int>(maxLines)) : 0;

            for (int i = startIndex; i < totalCount; ++i) {
                result.Add(allLines[i]);
            }
        } catch (...) {
        }
        return result;
    }
}
