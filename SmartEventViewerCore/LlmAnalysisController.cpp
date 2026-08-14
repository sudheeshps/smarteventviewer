#include "pch.h"
#include "Core/LlmAnalysisController.h"
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "System/Diagnostics/EtwLogReader.h"
#include "System/Diagnostics/Process.h"
#include "System/Threading/Tasks/Task.h"
#include "System/Threading/ManualResetEvent.h"
#include "System/Threading/ManualResetEvent.h"
#include "System/Diagnostics/Stopwatch.h"
#include "Logging/AppLoggerManager.h"
#include "System/Console.h"
#include "System/Convert.h"
#include "Core/EventsController.h"
#include "Core/TelemetryWebSocketHandler.h"

using Console = DotNetDupe::System::Console;
using Convert = DotNetDupe::System::Convert;
using namespace DotNetDupe::System::Threading;

namespace SmartEventViewer {
    DotNetDupe::System::SmartPointer<LocalLlmEngine> LlmAnalysisController::s_spLlmEngine = nullptr;
    CriticalSection LlmAnalysisController::s_analysisQueueCs{};
    CriticalSection LlmAnalysisController::s_analysisResultsCs{};
    AutoResetEvent LlmAnalysisController::s_analysisEvent{ false };
    AutoResetEvent LlmAnalysisController::s_shutdownEvent{ false };
    BlockingCollection<SmartPointer<AnalysisTaskItem>> LlmAnalysisController::s_analysisQueue{};
    Dictionary<String, AnalyzeResponseDto> LlmAnalysisController::s_analysisResults{};
    bool LlmAnalysisController::s_bStopWorker = false;
    unsigned long long LlmAnalysisController::s_uNextTaskId = 1000;
    SmartPointer<Thread> LlmAnalysisController::s_spAnalysisWorkerThread = nullptr;
    bool LlmAnalysisController::s_bWorkerInitialized = false;

    static List<String> GetTargetChannelsList(const String& sTargetChannel) {
        List<String> channelsToScan;
        channelsToScan.Add("Security");
        channelsToScan.Add("Microsoft-Windows-Sysmon/Operational");
        channelsToScan.Add("System");
        channelsToScan.Add("Application");
        channelsToScan.Add("Microsoft-Windows-PowerShell/Operational");
        channelsToScan.Add("Microsoft-Windows-TerminalServices-LocalSessionManager/Operational");
        channelsToScan.Add("Microsoft-Windows-TerminalServices-RemoteConnectionManager/Operational");
        channelsToScan.Add("Microsoft-Windows-TaskScheduler/Operational");
        channelsToScan.Add("Microsoft-Windows-CodeIntegrity/Operational");
        channelsToScan.Add("Microsoft-Windows-Windows Defender/Operational");
        channelsToScan.Add("Microsoft-Windows-AppLocker/EXE and DLL");
        channelsToScan.Add("Microsoft-Windows-WMI-Activity/Operational");
        if (!sTargetChannel.IsEmpty() && sTargetChannel != "ALL" && !channelsToScan.Contains(sTargetChannel)) {
            channelsToScan.Add(sTargetChannel);
        }
        return channelsToScan;
    }

    static void ScanChannelEvents(const String& sChannel, List<EventRecord>& eventList) {
        auto rawEvents = DotNetDupe::System::Diagnostics::EtwLogReader::ReadEvents(sChannel, 15, 0, true);
        for (int i = 0; i < rawEvents.GetCount(); ++i) {
            EventRecord evt = EventRecord::FromEtwEvent(rawEvents[i], sChannel);
            EventLevel lvl = evt.GetLevel();
            RiskLevel risk = AnomalyEngine::EvaluateRisk(evt);

            if (lvl == EventLevel::Critical || lvl == EventLevel::Error || risk == RiskLevel::Critical || risk == RiskLevel::High || risk == RiskLevel::Medium) {
                eventList.Add(evt);
            } else if (eventList.GetCount() < 10) {
                eventList.Add(evt);
            }
        }
    }

    LlmAnalysisController::LlmAnalysisController() {
        EnsureWorkerStarted();
    }

    LlmAnalysisController::LlmAnalysisController(const SmartPointer<LocalLlmEngine>& spLlmEngine)
        : m_spLlmEngine(spLlmEngine) {
        if (!spLlmEngine.IsNull()) {
            s_spLlmEngine = spLlmEngine;
        }
        EnsureWorkerStarted();
    }

    void LlmAnalysisController::EnsureWorkerStarted() {
        Lock<CriticalSection> lock(s_analysisQueueCs);
        if (!s_bWorkerInitialized) {
            s_bStopWorker = false;
            s_spAnalysisWorkerThread = SmartPointer<Thread>::NewShared(AnalysisWorkerLoop);
            s_spAnalysisWorkerThread->Start();
            s_bWorkerInitialized = true;
            AppLoggerManager::Info("SERVER", "[LLM_CONTROLLER] Started background AI analysis worker thread.");
        }
    }

    static const String s_sStatusRunning("RUNNING");
    static const String s_sStatusDownloading("DOWNLOADING");
    static const String s_sStatusCompleted("COMPLETED");
    static const String s_sMsgNoEvents("No relevant security event records found in specified channels to perform AI analysis.");
    static const String s_sMsgZeroEvaluated("Analysis complete (0 events evaluated).");

    List<EventRecord> LlmAnalysisController::AggregateTaskEvents(const String& sTaskId, const String& sChannel) {
        List<String> channelsToScan = GetTargetChannelsList(sChannel);
        List<EventRecord> aggregatedEvents;
        for (int i = 0; i < channelsToScan.GetCount(); ++i) { {
                Lock<CriticalSection> lock(s_analysisResultsCs);
                AnalyzeResponseDto currentStatus;
                if (s_analysisResults.TryGetValue(sTaskId, currentStatus)) {
                    currentStatus.Status = s_sStatusRunning;
                    currentStatus.ProgressMessage = String::Format("Scanning channel [{0}/{1}] {2}...", i + 1, channelsToScan.GetCount(), channelsToScan[i]);
                    s_analysisResults[sTaskId] = currentStatus;
                }
            }
            AppLoggerManager::Info("SERVER", String::Format("[LLM_ANALYZER] Scanning channel [{0}/{1}]: {2}", i + 1, channelsToScan.GetCount(), channelsToScan[i]));
            ScanChannelEvents(channelsToScan[i], aggregatedEvents);
        }
        AppLoggerManager::Info("SERVER", String::Format("[LLM_ANALYZER] Aggregated {0} security event records for analysis", aggregatedEvents.GetCount()));
        return aggregatedEvents;
    }

    void LlmAnalysisController::UpdateDownloadProgressStatus(const String& sTaskId, double progressPct, double rateBytesPerSec, long long downloadedBytes, long long totalBytes) {
        Lock<CriticalSection> lock(s_analysisResultsCs);
        AnalyzeResponseDto currentStatus;
        if (s_analysisResults.TryGetValue(sTaskId, currentStatus)) {
            double rateMBps = rateBytesPerSec / (1024.0 * 1024.0);
            double downloadedMB = downloadedBytes / (1024.0 * 1024.0);
            double totalMB = totalBytes / (1024.0 * 1024.0);
            int ipct = static_cast<int>(progressPct);
            int irate = static_cast<int>(rateMBps);
            int idownloaded = static_cast<int>(downloadedMB);
            int itotal = static_cast<int>(totalMB);
            currentStatus.Status = s_sStatusDownloading;
            currentStatus.DownloadProgress = progressPct;
            currentStatus.DownloadRateBytesPerSec = rateBytesPerSec;
            currentStatus.DownloadedBytes = static_cast<unsigned long long>(downloadedBytes);
            currentStatus.TotalBytes = static_cast<unsigned long long>(totalBytes);
            currentStatus.ProgressMessage = String::Format("Downloading AI GGUF model weights ({0}% - {1} MB/s [{2}/{3} MB])...", ipct, irate, idownloaded, itotal);
            s_analysisResults[sTaskId] = currentStatus;
        }
        TelemetryWebSocketHandler::GetInstance()->BroadcastCategoryUpdate("llm_analysis");
        AppLoggerManager::Info("SERVER", String::Format("[LLM_ANALYZER] Model weights download progress: {0}% for Task #{1}", static_cast<int>(progressPct), sTaskId));
    }

    String LlmAnalysisController::RunEngineInference(const String& sTaskId, const AnalyzeRequestDto& request, const List<EventRecord>& events) {
        DotNetDupe::System::Array<EventRecord> eventArray(events.GetCount());
        for (int i = 0; i < events.GetCount(); ++i) {
            eventArray[i] = events[i];
        }
        AppLoggerManager::Info("SERVER", String::Format("[LLM_ANALYZER] Invoking LocalLlmEngine ProcessQueryAsync for Task #{0}...", sTaskId));
        
        DotNetDupe::System::Threading::ManualResetEvent completion(false);
        String sFinalResult = "";
        SmartPointer<LocalLlmEngine> spEngine = s_spLlmEngine;
        
        if (!spEngine.IsNull()) {
            spEngine->ProcessQueryAsync(
                request.Query,
                eventArray.GetData(),
                static_cast<unsigned int>(eventArray.GetLength()),
                [sTaskId, &completion, &sFinalResult](const String& status, const String& result, double pct) {
                    if (status == "DOWNLOADING") {
                        UpdateDownloadProgressStatus(sTaskId, pct, 0.0, 0, 0);
                    } else if (status == "COMPLETED" || status == "ERROR") {
                        sFinalResult = result;
                        completion.Set();
                    } else {
                        UpdateDownloadProgressStatus(sTaskId, pct, 0.0, 0, 0);
                    }
                }
            );
            completion.WaitOne(-1);
        }
        return sFinalResult;
    }

    bool LlmAnalysisController::EnsureModelFileAvailable(const String& sTaskId) {
        return true; // Download is now handled inside ProcessQueryAsync
    }

    AnalyzeResponseDto LlmAnalysisController::ProcessAnalysisTask(const String& sTaskId, const AnalyzeRequestDto& request) {
        AppLoggerManager::Info("SERVER", String::Format("[LLM_ANALYZER] Analysis triggered for Task #{0} | Channel: {1} | Query: '{2}'", sTaskId, request.Channel, request.Query));
        AnalyzeResponseDto responseDto;
        responseDto.TaskId = sTaskId;
        responseDto.Channel = request.Channel;
        responseDto.Query = request.Query;

        if (!EnsureModelFileAvailable(sTaskId)) {
            // Deprecated path, EnsureModelFileAvailable returns true now.
        }

        List<EventRecord> aggregatedEvents = AggregateTaskEvents(sTaskId, request.Channel);
        responseDto.EventsAnalyzed = aggregatedEvents.GetCount();

        if (aggregatedEvents.GetCount() > 0) {
            String result = RunEngineInference(sTaskId, request, aggregatedEvents);
            responseDto.Analysis = result;
            if (result.StartsWith("[ERROR]")) {
                responseDto.Status = "FAILED";
                responseDto.ProgressMessage = "Model download failed.";
                AppLoggerManager::Error("SERVER", String::Format("[LLM_ANALYZER] Task #{0} failed during engine inference.", sTaskId));
            } else {
                responseDto.Status = s_sStatusCompleted;
                responseDto.ProgressMessage = String::Format("Analysis complete. Analyzed {0} security events.", aggregatedEvents.GetCount());
                AppLoggerManager::Info("SERVER", String::Format("[LLM_ANALYZER] Analysis Task #{0} completed successfully. Analyzed {1} events.", sTaskId, aggregatedEvents.GetCount()));
            }
        } else {
            responseDto.Analysis = s_sMsgNoEvents;
            responseDto.Status = s_sStatusCompleted;
            responseDto.ProgressMessage = s_sMsgZeroEvaluated;
            AppLoggerManager::Info("SERVER", String::Format("[LLM_ANALYZER] Task #{0} finished: 0 security events evaluated.", sTaskId));
        }
        return responseDto;
    }

    void LlmAnalysisController::AnalysisWorkerLoop() {
        while (!s_bStopWorker) {
            s_analysisEvent.WaitOne();
            if (s_bStopWorker) break;

            SmartPointer<AnalysisTaskItem> taskItem;
            while (s_analysisQueue.TryTake(taskItem, 10)) {
                if (taskItem.IsNull()) continue;
                String sTaskId = taskItem->TaskId;
                AnalyzeResponseDto result;
                try {
                    result = ProcessAnalysisTask(sTaskId, taskItem->Request);
                } catch (const DotNetDupe::System::Exception& ex) {
                    result.TaskId = sTaskId;
                    result.Status = "FAILED";
                    result.ProgressMessage = "Analysis failed due to DotNetDupe exception.";
                    result.Analysis = "[ERROR] Model download or analysis interrupted by an unexpected error.";
                    AppLoggerManager::Error("SERVER", String::Format("[LLM_ANALYZER] Task #{0} worker DotNetDupe exception caught: {1}", sTaskId, ex.What()));
                } catch (const std::exception& ex) {
                    result.TaskId = sTaskId;
                    result.Status = "FAILED";
                    result.ProgressMessage = "Analysis failed due to std::exception.";
                    result.Analysis = "[ERROR] Model download or analysis interrupted by an unexpected error.";
                    AppLoggerManager::Error("SERVER", String::Format("[LLM_ANALYZER] Task #{0} worker std::exception caught: {1}", sTaskId, DotNetDupe::System::String(ex.what())));
                } catch (...) {
                    result.TaskId = sTaskId;
                    result.Status = "FAILED";
                    result.ProgressMessage = "Analysis failed due to unhandled exception.";
                    result.Analysis = "[ERROR] Model download or analysis interrupted by an unexpected error.";
                    AppLoggerManager::Error("SERVER", String::Format("[LLM_ANALYZER] Task #{0} worker exception caught.", sTaskId));
                }

                {
                    Lock<CriticalSection> lock(s_analysisResultsCs);
                    s_analysisResults[sTaskId] = result;
                }
            }
        }
    }

    AnalyzeResponseDto LlmAnalysisController::CreatePendingResponse(const String& sTaskId, const AnalyzeRequestDto& request) {
        AnalyzeResponseDto pendingResponse;
        pendingResponse.TaskId = sTaskId;
        pendingResponse.Status = "PENDING";
        pendingResponse.ProgressMessage = "Initializing analysis...";
        pendingResponse.Channel = request.Channel;
        pendingResponse.Query = request.Query;
        pendingResponse.Analysis = "";
        pendingResponse.EventsAnalyzed = 0;
        return pendingResponse;
    }

    AnalyzeResponseDto LlmAnalysisController::AnalyzeEvents(const AnalyzeRequestDto& request) {
        String sTaskId;
        {
            Lock<CriticalSection> lock(s_analysisQueueCs);
            sTaskId = "TASK_" + Convert::ToString(static_cast<uint64_t>(++s_uNextTaskId));
        }

        AnalyzeResponseDto initialResponse = CreatePendingResponse(sTaskId, request);
        static const String sModelPath("models/Llama-3-8B-Instruct.Q4_K_M.gguf");
        SmartPointer<LocalLlmEngine> spEngine = m_spLlmEngine.IsNull() ? s_spLlmEngine : m_spLlmEngine;
        if (!spEngine.IsNull() && !spEngine->IsModelFilePresent(sModelPath)) {
            initialResponse.Status = s_sStatusDownloading;
            initialResponse.ProgressMessage = "Starting download of AI GGUF model weights...";
            initialResponse.DownloadProgress = 0.0;
        }

        {
            Lock<CriticalSection> lock(s_analysisResultsCs);
            s_analysisResults[sTaskId] = initialResponse;
        }
        TelemetryWebSocketHandler::GetInstance()->BroadcastCategoryUpdate("llm_analysis");

        EnsureWorkerStarted();

        SmartPointer<AnalysisTaskItem> taskItem = SmartPointer<AnalysisTaskItem>::NewShared();
        taskItem->TaskId = sTaskId;
        taskItem->Request = request;

        s_analysisQueue.Add(taskItem);
        s_analysisEvent.Set();

        return initialResponse;
    }

    AnalyzeResponseDto LlmAnalysisController::GetAnalyzeStatus(const String& sTaskId) {
        Lock<CriticalSection> lock(s_analysisResultsCs);
        AnalyzeResponseDto result;
        if (s_analysisResults.TryGetValue(sTaskId, result)) {
            return result;
        }

        result.TaskId = sTaskId;
        result.Status = "NOT_FOUND";
        result.ProgressMessage = "Specified Task ID was not found in active queue.";
        return result;
    }

    void LlmAnalysisController::Shutdown() {
        s_bStopWorker = true;
        s_shutdownEvent.Set();
        s_analysisEvent.Set();
    }
}
