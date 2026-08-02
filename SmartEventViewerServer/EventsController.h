#pragma once

#include "WebAppCore/Controllers/ControllerBase.h"
#include "WebAppCore/Controllers/ControllerRouteBuilder.h"
#include "Platform/WinEventLogReader.h"
#include "Platform/SystemTelemetryProvider.h"
#include "Core/EventDtos.h"

#include "Ai/LocalLlmEngine.h"

#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/Threading/Thread.h"
#include "System/Threading/AutoResetEvent.h"
#include <queue>
#include <map>

using namespace DotNetDupe::System;
using namespace DotNetDupe::WebAppCore::Controllers;

namespace SmartEventViewer
{
    struct AnalysisTaskItem
    {
        String TaskId;
        AnalyzeRequestDto Request;
    };

    class EventsController : public ControllerBase
    {
    private:
        WinEventLogReader m_logReader{};
        static LocalLlmEngine s_llmEngine;

        // Process-wide background worker thread and thread-safe task queue for async analysis
        static DotNetDupe::System::Threading::CriticalSection s_analysisCs;
        static DotNetDupe::System::Threading::AutoResetEvent s_analysisEvent;
        static std::queue<std::shared_ptr<AnalysisTaskItem>> s_analysisQueue;
        static std::map<std::string, AnalyzeResponseDto> s_analysisResults;
        static bool s_bStopWorker;
        static unsigned long long s_uNextTaskId;
        static DotNetDupe::System::SmartPointer<DotNetDupe::System::Threading::Thread> s_spAnalysisWorkerThread;
        static bool s_bWorkerInitialized;

        static void EnsureWorkerStarted();
        static void AnalysisWorkerLoop();
        static AnalyzeResponseDto ProcessAnalysisTask(const String& sTaskId, const AnalyzeRequestDto& request);

    public:
        EventsController();
        ~EventsController() override;

        // Returns strongly typed ChannelsResponseDto payload
        ChannelsResponseDto GetChannels();

        // Returns strongly typed EventLogResponseDto payload with pagination
        EventLogResponseDto GetEvents(const String& channelName, size_t page, size_t pageSize);
        EventLogResponseDto GetEvents(const String& channelName);

        // System Telemetry & User Sessions Endpoint
        SystemMetricsResponseDto GetMetrics();

        // Natural Language AI Threat Analysis Endpoint (Async enqueue & non-blocking return)
        AnalyzeResponseDto AnalyzeEvents(const AnalyzeRequestDto& request);

        // Asynchronous Status Polling Endpoint
        AnalyzeResponseDto GetAnalyzeStatus(const String& sTaskId);
    };
}
