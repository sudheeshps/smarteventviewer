#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <wtsapi32.h>
#include <iphlpapi.h>
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#endif

#include "EventsController.h"
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "System/Console.h"
#include "System/Convert.h"
#include "System/String.h"
#include <future>

using Console = DotNetDupe::System::Console;

namespace SmartEventViewer
{
    ChannelsResponseDto EventsController::GetChannels()
    {
        Console::WriteLine("[SERVER] Executing EventsController::GetChannels() -> Enumerating event sources...");
        ChannelsResponseDto dto;
        m_logReader.GetEventSources(dto.Channels);
        Console::WriteLine("[SERVER] Successfully enumerated channels count: {0}", static_cast<unsigned long long>(dto.Channels.GetCount()));
        return dto;
    }

    static String UrlDecodeChannel(const String& input)
    {
        std::string str = input.GetRawString();
        std::string decoded;
        decoded.reserve(str.length());
        for (size_t i = 0; i < str.length(); ++i)
        {
            if (str[i] == '%')
            {
                if (i + 2 < str.length())
                {
                    int hexVal = 0;
                    std::istringstream iss(str.substr(i + 1, 2));
                    if (iss >> std::hex >> hexVal)
                    {
                        decoded += static_cast<char>(hexVal);
                        i += 2;
                        continue;
                    }
                }
            }
            else if (str[i] == '+')
            {
                decoded += ' ';
                continue;
            }
            decoded += str[i];
        }
        return String(decoded.c_str());
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName, size_t page, size_t pageSize)
    {
        String sRawChannel = channelName.IsEmpty() ? String("Application") : channelName;
        String sTargetChannel = UrlDecodeChannel(sRawChannel);
        if (page < 1) page = 1;
        if (pageSize < 1) pageSize = 20;

        Console::WriteLine("[SERVER] Executing EventsController::GetEvents() for channel: {0} (Page: {1}, PageSize: {2})", sTargetChannel, static_cast<unsigned long long>(page), static_cast<unsigned long long>(pageSize));

        unsigned long long uTotalCount = m_logReader.GetChannelEventCount(sTargetChannel);
        auto levelCounts = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventLevelCounts(sTargetChannel);
        Console::WriteLine("[SERVER] Total log records: {0} (Critical: {1}, Error: {2}, Warning: {3}, Info: {4})", uTotalCount, levelCounts.uCriticalCount, levelCounts.uErrorCount, levelCounts.uWarningCount, levelCounts.uInfoCount);

        size_t totalPages = (uTotalCount == 0) ? 0 : static_cast<size_t>((uTotalCount + pageSize - 1) / pageSize);

        EventLogResponseDto responseDto;
        responseDto.Channel = sTargetChannel;
        responseDto.TotalCount = uTotalCount;
        responseDto.CriticalCount = levelCounts.uCriticalCount;
        responseDto.ErrorCount = levelCounts.uErrorCount;
        responseDto.WarningCount = levelCounts.uWarningCount;
        responseDto.InfoCount = levelCounts.uInfoCount;
        responseDto.VerboseCount = levelCounts.uVerboseCount;
        responseDto.Page = page;
        responseDto.PageSize = pageSize;
        responseDto.TotalPages = totalPages;

        size_t startIndex = (page - 1) * pageSize;

        if (m_logReader.OpenLogPaged(sTargetChannel, static_cast<int>(pageSize), static_cast<int>(startIndex)))
        {
            EventRecord evt;
            size_t currentItemIdx = startIndex + 1;

            unsigned long long pagedCritical = 0;
            unsigned long long pagedError = 0;
            unsigned long long pagedWarning = 0;
            unsigned long long pagedInfo = 0;

            while (m_logReader.ReadNextEvent(evt))
            {
                if (evt.GetLevel() == EventLevel::Critical) pagedCritical++;
                else if (evt.GetLevel() == EventLevel::Error) pagedError++;
                else if (evt.GetLevel() == EventLevel::Warning) pagedWarning++;
                else pagedInfo++;

                EventDto dto;
                dto.Index = currentItemIdx++;
                dto.Id = evt.GetEventId();
                dto.Level = (evt.GetLevel() == EventLevel::Critical ? "Critical" : (evt.GetLevel() == EventLevel::Error ? "Error" : (evt.GetLevel() == EventLevel::Warning ? "Warning" : "Information")));
                dto.Risk = (evt.GetRiskLevel() == RiskLevel::Critical ? "Critical" : (evt.GetRiskLevel() == RiskLevel::High ? "High" : (evt.GetRiskLevel() == RiskLevel::Medium ? "Medium" : "Low")));
                dto.Provider = evt.GetProviderName();
                dto.Time = evt.GetTimeCreated();
                dto.Message = evt.GetEventMessage();
                dto.RawXml = evt.GetRawXml();

                responseDto.Events.Add(dto);
            }
            m_logReader.Close();
            Console::WriteLine("[SERVER] Streamed DTO response: {0} events (Level Counts -> Crit: {1}, Err: {2}, Warn: {3}, Info: {4})", static_cast<unsigned long long>(responseDto.Events.GetCount()), responseDto.CriticalCount, responseDto.ErrorCount, responseDto.WarningCount, responseDto.InfoCount);
        }
        else
        {
            Console::WriteLine("[SERVER] [WARNING] Failed to open channel log: {0}", sTargetChannel);
        }

        return responseDto;
    }

    using namespace DotNetDupe::System::Threading;

    // Define process-wide static analysis members
    LocalLlmEngine EventsController::s_llmEngine{};
    CriticalSection EventsController::s_analysisQueueCs{};
    CriticalSection EventsController::s_analysisResultsCs{};
    AutoResetEvent EventsController::s_analysisEvent{ false };
    std::queue<std::shared_ptr<AnalysisTaskItem>> EventsController::s_analysisQueue{};
    std::map<std::string, AnalyzeResponseDto> EventsController::s_analysisResults{};
    bool EventsController::s_bStopWorker = false;
    unsigned long long EventsController::s_uNextTaskId = 1000;
    DotNetDupe::System::SmartPointer<Thread> EventsController::s_spAnalysisWorkerThread = nullptr;
    bool EventsController::s_bWorkerInitialized = false;

    EventsController::EventsController()
    {
        EnsureWorkerStarted();
    }

    EventsController::~EventsController() = default;

    void EventsController::EnsureWorkerStarted()
    {
        Lock<CriticalSection> lock(s_analysisQueueCs);
        if (!s_bWorkerInitialized)
        {
            s_llmEngine.Initialize(String("models/Llama-3-8B-Instruct.Q4_K_M.gguf"));
            s_bStopWorker = false;
            s_spAnalysisWorkerThread = DotNetDupe::System::SmartPointer<Thread>(new Thread([]() {
                AnalysisWorkerLoop();
            }));
            s_spAnalysisWorkerThread->Start();
            s_bWorkerInitialized = true;
        }
    }

    void EventsController::AnalysisWorkerLoop()
    {
        while (true)
        {
            std::shared_ptr<AnalysisTaskItem> taskItem = nullptr;
            {
                Lock<CriticalSection> lock(s_analysisQueueCs);
                if (s_bStopWorker && s_analysisQueue.empty())
                {
                    break;
                }
                if (!s_analysisQueue.empty())
                {
                    taskItem = s_analysisQueue.front();
                    s_analysisQueue.pop();
                }
            }

            if (taskItem)
            {
                AnalyzeResponseDto completedResponse = ProcessAnalysisTask(taskItem->TaskId, taskItem->Request);
                {
                    Lock<CriticalSection> lock(s_analysisResultsCs);
                    s_analysisResults[taskItem->TaskId.GetRawString()] = completedResponse;
                }
            }
            else
            {
                s_analysisEvent.WaitOne(500);
            }
        }
    }

    static std::vector<String> GetTargetChannelsList(const String& sTargetChannel)
    {
        std::vector<String> channelsToScan;
        if (sTargetChannel == String("ALL") || sTargetChannel.IsEmpty())
        {
            channelsToScan.push_back(String("Security"));
            channelsToScan.push_back(String("Microsoft-Windows-Sysmon/Operational"));
            channelsToScan.push_back(String("System"));
            channelsToScan.push_back(String("Application"));
            channelsToScan.push_back(String("Microsoft-Windows-PowerShell/Operational"));
            channelsToScan.push_back(String("Microsoft-Windows-TerminalServices-LocalSessionManager/Operational"));
            channelsToScan.push_back(String("Microsoft-Windows-TaskScheduler/Operational"));
            channelsToScan.push_back(String("Microsoft-Windows-CodeIntegrity/Operational"));
            channelsToScan.push_back(String("Microsoft-Windows-Windows Defender/Operational"));
            channelsToScan.push_back(String("Microsoft-Windows-AppLocker/EXE and DLL"));
        }
        else
        {
            channelsToScan.push_back(sTargetChannel);
        }
        return channelsToScan;
    }

    static void ScanChannelEvents(const String& sChannel, DotNetDupe::System::Collections::Generic::List<EventRecord>& eventList)
    {
        WinEventLogReader logReader;
        if (!logReader.OpenLog(sChannel)) return;

        EventRecord evt;
        size_t countForChannel = 0;
        while (logReader.ReadNextEvent(evt) && countForChannel < 15)
        {
            EventLevel lvl = evt.GetLevel();
            RiskLevel risk = AnomalyEngine::EvaluateRisk(evt);

            if (lvl == EventLevel::Critical || lvl == EventLevel::Error || risk == RiskLevel::Critical || risk == RiskLevel::High || risk == RiskLevel::Medium)
            {
                eventList.Add(evt);
                countForChannel++;
            }
            else if (eventList.GetCount() < 10)
            {
                eventList.Add(evt);
                countForChannel++;
            }
        }
        logReader.Close();
    }

    AnalyzeResponseDto EventsController::ProcessAnalysisTask(const String& sTaskId, const AnalyzeRequestDto& request)
    {
        String sRawChannel = request.Channel.IsEmpty() ? String("ALL") : request.Channel;
        String sTargetChannel = UrlDecodeChannel(sRawChannel);
        String sQuery = request.Query.IsEmpty() ? String("Analyze suspicious event activity across system channels") : request.Query;

        Console::WriteLine("[SERVER] Background worker processing queued task #{0} for channel scope: {1}", sTaskId, sTargetChannel);

        // Step 1: Starting analysis...
        {
            Lock<CriticalSection> lock(s_analysisResultsCs);
            auto& res = s_analysisResults[sTaskId.GetRawString()];
            res.Status = String("PROCESSING");
            res.ProgressMessage = String("Starting analysis...");
        }

        DotNetDupe::System::Collections::Generic::List<EventRecord> eventList;
        std::vector<String> channelsToScan = GetTargetChannelsList(sTargetChannel);

        for (const auto& ch : channelsToScan)
        {
            // Push Notification Update: Reading logs from channel...
            {
                Lock<CriticalSection> lock(s_analysisResultsCs);
                auto& res = s_analysisResults[sTaskId.GetRawString()];
                res.ProgressMessage = String("Reading logs from channel ") + ch + String("...");
            }
            ScanChannelEvents(ch, eventList);
            DotNetDupe::System::Threading::Thread::Sleep(150);
        }

        // Push Notification Update: Ingesting logs into RAG vector store...
        {
            Lock<CriticalSection> lock(s_analysisResultsCs);
            auto& res = s_analysisResults[sTaskId.GetRawString()];
            res.ProgressMessage = String("Ingesting ") + Convert::ToString(static_cast<int>(eventList.GetCount())) + String(" logs into RAG vector store...");
        }
        DotNetDupe::System::Threading::Thread::Sleep(200);

        std::vector<EventRecord> eventBuffer;
        for (int i = 0; i < eventList.GetCount(); ++i) eventBuffer.push_back(eventList[i]);

        // Push Notification Update: Analyzing threat vectors...
        {
            Lock<CriticalSection> lock(s_analysisResultsCs);
            auto& res = s_analysisResults[sTaskId.GetRawString()];
            res.ProgressMessage = String("Analyzing threat vectors with embedded AI engine...");
        }
        DotNetDupe::System::Threading::Thread::Sleep(300);

        AnalyzeResponseDto responseDto;
        responseDto.TaskId = sTaskId;
        responseDto.Status = String("COMPLETED");
        responseDto.ProgressMessage = String("Analysis complete.");
        responseDto.Channel = sTargetChannel;
        responseDto.Query = sQuery;
        responseDto.EventsAnalyzed = static_cast<unsigned long long>(eventList.GetCount());
        responseDto.Analysis = s_llmEngine.ProcessQuery(sQuery, eventBuffer.empty() ? nullptr : eventBuffer.data(), static_cast<unsigned int>(eventBuffer.size()));

        {
            Lock<CriticalSection> lock(s_analysisResultsCs);
            s_analysisResults[sTaskId.GetRawString()] = responseDto;
        }

        Console::WriteLine("[SERVER] Background worker task #{0} completed RAG analysis for {1} events", sTaskId, static_cast<unsigned long long>(responseDto.EventsAnalyzed));
        return responseDto;
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName)
    {
        return GetEvents(channelName, 1, 20);
    }

    AnalyzeResponseDto EventsController::AnalyzeEvents(const AnalyzeRequestDto& request)
    {
        EnsureWorkerStarted();

        String sTaskId;
        {
            Lock<CriticalSection> lock(s_analysisQueueCs);
            s_uNextTaskId++;
            sTaskId = String("TASK_") + String(std::to_string(s_uNextTaskId).c_str());
        }

        Console::WriteLine("[SERVER] Enqueuing AnalyzeEvents request into task queue as #{0}...", sTaskId);

        auto taskItem = std::make_shared<AnalysisTaskItem>();
        taskItem->TaskId = sTaskId;
        taskItem->Request = request;

        AnalyzeResponseDto pendingResponse;
        pendingResponse.TaskId = sTaskId;
        pendingResponse.Status = String("PENDING");
        pendingResponse.ProgressMessage = String("Enqueued for analysis...");
        pendingResponse.Channel = request.Channel;
        pendingResponse.Query = request.Query;
        pendingResponse.Analysis = String("Task enqueued for processing.");
        pendingResponse.EventsAnalyzed = 0;

        {
            Lock<CriticalSection> lock(s_analysisQueueCs);
            s_analysisQueue.push(taskItem);
        }

        {
            Lock<CriticalSection> lock(s_analysisResultsCs);
            s_analysisResults[sTaskId.GetRawString()] = pendingResponse;
        }

        s_analysisEvent.Set();

        // Immediately return non-blocking 202-style payload
        return pendingResponse;
    }

    AnalyzeResponseDto EventsController::GetAnalyzeStatus(const String& sTaskId)
    {
        EnsureWorkerStarted();

        Lock<CriticalSection> lock(s_analysisResultsCs);
        auto it = s_analysisResults.find(sTaskId.GetRawString());
        if (it != s_analysisResults.end())
        {
            return it->second;
        }

        AnalyzeResponseDto notFound;
        notFound.TaskId = sTaskId;
        notFound.Status = String("NOT_FOUND");
        notFound.Analysis = String("Specified analysis task ID not found.");
        return notFound;
    }

    SystemMetricsResponseDto EventsController::GetMetrics()
    {
        return SystemTelemetryProvider::QuerySystemMetrics();
    }
}
