#include "pch.h"
#include "Logging/Logger.h"

namespace SmartEventViewer
{
    CriticalSection Logger::s_logCs;
    String Logger::s_logFilePath = "logs/SmartEventViewerServer.log";
    size_t Logger::s_maxFileSizeBytes = 5 * 1024 * 1024;
    size_t Logger::s_maxRolledFiles = 10;
    bool Logger::s_isInitialized = false;
}
