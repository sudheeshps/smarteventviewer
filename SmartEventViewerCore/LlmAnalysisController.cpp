#include "pch.h"
#include "LlmAnalysisController.h"
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "Platform/WinEventLogReader.h"
#include "System/Console.h"
#include "System/Convert.h"
#include "EventsController.h"

using Console = DotNetDupe::System::Console;
using Convert = DotNetDupe::System::Convert;
using namespace DotNetDupe::System::Threading;

namespace SmartEventViewer
{
    DotNetDupe::System::SmartPointer<LocalLlmEngine> LlmAnalysisController::s_spLlmEngine = nullptr;
    CriticalSection LlmAnalysisController::s_analysisQueueCs{};
    CriticalSection LlmAnalysisController::s_analysisResultsCs{};
    AutoResetEvent LlmAnalysisController::s_analysisEvent{ false };
    BlockingCollection<SmartPointer<AnalysisTaskItem>> LlmAnalysisController::s_analysisQueue{};
    Dictionary<String, AnalyzeResponseDto> LlmAnalysisController::s_analysisResults{};
    bool LlmAnalysisController::s_bStopWorker = false;
    unsigned long long LlmAnalysisController::s_uNextTaskId = 1000;
    SmartPointer<Thread> LlmAnalysisController::s_spAnalysisWorkerThread = nullptr;
    bool LlmAnalysisController::s_bWorkerInitialized = false;

    static List<String> GetTargetChannelsList(const String& sTargetChannel)
    {
        List<String> channelsToScan;
        if (sTargetChannel == String("ALL") || sTargetChannel.IsEmpty())
        {
            channelsToScan.Add(String("Security"));
            channelsToScan.Add(String("Microsoft-Windows-Sysmon/Operational"));
            channelsToScan.Add(String("System"));
            channelsToScan.Add(String("Application"));
            channelsToScan.Add(String("Microsoft-Windows-PowerShell/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-TerminalServices-LocalSessionManager/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-TaskScheduler/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-CodeIntegrity/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-Windows Defender/Operational"));
            channelsToScan.Add(String("Microsoft-Windows-AppLocker/EXE and DLL"));
        }
        else
        {
            channelsToScan.Add(sTargetChannel);
        }
        return channelsToScan;
    }

    static void ScanChannelEvents(const String& sChannel, List<EventRecord>& eventList)
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

    LlmAnalysisController::LlmAnalysisController()
    {
        EnsureWorkerStarted();
    }

    LlmAnalysisController::LlmAnalysisController(const SmartPointer<LocalLlmEngine>& spLlmEngine)
        : m_spLlmEngine(spLlmEngine)
    {
        if (!spLlmEngine.IsNull())
        {
            s_spLlmEngine = spLlmEngine;
        }
        EnsureWorkerStarted();
    }

    void LlmAnalysisController::EnsureWorkerStarted()
    {
        Lock<CriticalSection> lock(s_analysisQueueCs);
        if (!s_bWorkerInitialized)
        {
            s_bStopWorker = false;
            s_spAnalysisWorkerThread = SmartPointer<Thread>::NewShared(AnalysisWorkerLoop);
            s_spAnalysisWorkerThread->Start();
            s_bWorkerInitialized = true;
            EventsController::Log("[LLM_CONTROLLER] Started background AI analysis worker thread.");
        }
    }

    AnalyzeResponseDto LlmAnalysisController::ProcessAnalysisTask(const String& sTaskId, const AnalyzeRequestDto& request)
    {
        EventsController::Log(String::Format("[LLM_CONTROLLER] Processing AI threat analysis for Task #{0}...", sTaskId));
        List<String> channelsToScan = GetTargetChannelsList(request.Channel);
        List<EventRecord> aggregatedEvents;

        for (int i = 0; i < channelsToScan.GetCount(); ++i)
        {
            ScanChannelEvents(channelsToScan[i], aggregatedEvents);
        }

        AnalyzeResponseDto responseDto;
        responseDto.TaskId = sTaskId;
        responseDto.Channel = request.Channel;
        responseDto.Query = request.Query;
        responseDto.EventsAnalyzed = aggregatedEvents.GetCount();

        DotNetDupe::System::Array<EventRecord> eventArray(aggregatedEvents.GetCount());
        for (int i = 0; i < aggregatedEvents.GetCount(); ++i)
        {
            eventArray[i] = aggregatedEvents[i];
        }

        if (LocalLlmEngine::GetInstance().IsModelLoaded() || aggregatedEvents.GetCount() > 0)
        {
            String llmResponse = LocalLlmEngine::GetInstance().ProcessQuery(request.Query, eventArray.GetData(), static_cast<unsigned int>(eventArray.GetLength()));
            responseDto.Analysis = llmResponse;
            responseDto.Status = String("COMPLETED");
            responseDto.ProgressMessage = String::Format("Analysis complete. Analyzed {0} security events.", aggregatedEvents.GetCount());
        }
        else
        {
            responseDto.Analysis = String("No relevant security event records found in specified channels to perform AI analysis.");
            responseDto.Status = String("COMPLETED");
            responseDto.ProgressMessage = String("Analysis complete (0 events evaluated).");
        }

        return responseDto;
    }

    void LlmAnalysisController::AnalysisWorkerLoop()
    {
        while (!s_bStopWorker)
        {
            s_analysisEvent.WaitOne();
            if (s_bStopWorker) break;

            SmartPointer<AnalysisTaskItem> taskItem;
            while (s_analysisQueue.TryTake(taskItem, 10))
            {
                if (taskItem.IsNull()) continue;
                String sTaskId = taskItem->TaskId;
                AnalyzeResponseDto result = ProcessAnalysisTask(sTaskId, taskItem->Request);

                {
                    Lock<CriticalSection> lock(s_analysisResultsCs);
                    s_analysisResults[sTaskId] = result;
                }
            }
        }
    }

    AnalyzeResponseDto LlmAnalysisController::AnalyzeEvents(const AnalyzeRequestDto& request)
    {
        EnsureWorkerStarted();

        String sTaskId;
        {
            Lock<CriticalSection> lock(s_analysisQueueCs);
            s_uNextTaskId++;
            sTaskId = String("TASK_") + Convert::ToString(static_cast<uint64_t>(s_uNextTaskId));
        }

        SmartPointer<AnalysisTaskItem> taskItem = SmartPointer<AnalysisTaskItem>::NewShared();
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

        s_analysisQueue.Add(taskItem);

        {
            Lock<CriticalSection> lock(s_analysisResultsCs);
            s_analysisResults[sTaskId] = pendingResponse;
        }

        s_analysisEvent.Set();
        return pendingResponse;
    }

    AnalyzeResponseDto LlmAnalysisController::GetAnalyzeStatus(const String& sTaskId)
    {
        Lock<CriticalSection> lock(s_analysisResultsCs);
        AnalyzeResponseDto result;
        if (s_analysisResults.TryGetValue(sTaskId, result))
        {
            return result;
        }

        result.TaskId = sTaskId;
        result.Status = String("NOT_FOUND");
        result.ProgressMessage = String("Specified Task ID was not found in active queue.");
        return result;
    }
}
