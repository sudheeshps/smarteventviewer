#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #ifdef SMARTEVENTVIEWER_EXPORTS
        #define SMARTEVENTVIEWER_API __declspec(dllexport)
    #else
        #define SMARTEVENTVIEWER_API __declspec(dllimport)
    #endif
#else
    #define SMARTEVENTVIEWER_API __attribute__((visibility("default")))
#endif

namespace SmartEventViewer
{
    enum class EventLevel
    {
        LogAlways = 0,
        Critical = 1,
        Error = 2,
        Warning = 3,
        Informational = 4,
        Verbose = 5
    };

    enum class RiskLevel
    {
        Low = 0,
        Medium = 1,
        High = 2,
        Critical = 3
    };
}
