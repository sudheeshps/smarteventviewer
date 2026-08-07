#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include "System/Collections/Generic/List.h"
#include "Extensions/Logging/ILogger.h"
#include "Extensions/Logging/LoggerFactory.h"
#include "Extensions/Logging/FileLoggerProvider.h"
#include "Extensions/Logging/ConsoleLoggerProvider.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using ILogger = DotNetDupe::Extensions::Logging::ILogger;
    using LogLevel = DotNetDupe::Extensions::Logging::LogLevel;

    class SMARTEVENTVIEWER_API AppLoggerManager
    {
    private:
        static DotNetDupe::System::SmartPointer<DotNetDupe::Extensions::Logging::LoggerFactory> s_loggerFactory;
        static String s_logFilePath;
        static bool s_isInitialized;

    public:
        static void Initialize(const String& logFilePath = "logs/SmartEventViewerServer.log", LogLevel minLevel = LogLevel::Trace);
        static DotNetDupe::System::SmartPointer<ILogger> GetLogger(const String& categoryName);

        static void Info(const String& category, const String& message);
        static void Warning(const String& category, const String& message);
        static void Error(const String& category, const String& message);
        static void Debug(const String& category, const String& message);

        static DotNetDupe::System::Collections::Generic::List<String> GetRecentLogLines(size_t maxLines = 200);
    };
}
