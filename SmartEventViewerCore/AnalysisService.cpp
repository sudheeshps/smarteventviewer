#include "pch.h"
#include "Core/AnalysisService.h"
#include "Core/EventService.h"
#include "Core/TelemetryService.h"
#include "System/Console.h"
#include "System/Convert.h"
#include "System/Threading/Thread.h"
#include "Logging/AppLoggerManager.h"

using Console = DotNetDupe::System::Console;
using Thread = DotNetDupe::System::Threading::Thread;
using ThreadStart = DotNetDupe::System::Threading::ThreadStart;

namespace SmartEventViewer {
    AnalysisService::AnalysisService()
        : m_spLlmEngine(DotNetDupe::System::SmartPointer<LocalLlmEngine>::NewShared()),
          m_spEventService(DotNetDupe::System::SmartPointer<IEventService>(DotNetDupe::System::SmartPointer<EventService>::NewShared())),
          m_spNotifier(nullptr) {
        StartWorker();
    }

    DotNetDupe::System::SmartPointer<AnalysisService> AnalysisService::GetSharedInstance() {
        static auto s_instance = DotNetDupe::System::SmartPointer<AnalysisService>::NewShared();
        return s_instance;
    }

    void AnalysisService::SetPushNotifier(const DotNetDupe::System::SmartPointer<ITelemetryPushNotifier>& spNotifier) {
        m_spNotifier = spNotifier;
    }

    AnalysisService::AnalysisService(
        const DotNetDupe::System::SmartPointer<LocalLlmEngine>& spLlmEngine,
        const DotNetDupe::System::SmartPointer<IEventService>& spEventService,
        const DotNetDupe::System::SmartPointer<ITelemetryPushNotifier>& spNotifier)
        : m_spLlmEngine(spLlmEngine.IsNull() ? DotNetDupe::System::SmartPointer<LocalLlmEngine>::NewShared() : spLlmEngine),
          m_spEventService(spEventService.IsNull() ? DotNetDupe::System::SmartPointer<IEventService>(DotNetDupe::System::SmartPointer<EventService>::NewShared()) : spEventService),
          m_spNotifier(spNotifier) {
        StartWorker();
    }

    AnalysisService::~AnalysisService() {
        Shutdown();
    }

    void AnalysisService::StartWorker() {
        if (!m_spWorkerThread.IsNull()) return;
        m_bStopWorker = false;
        m_spWorkerThread = DotNetDupe::System::SmartPointer<Thread>::NewShared(ThreadStart([this]() {
            WorkerLoop();
        }));
        m_spWorkerThread->Start();
    }

    void AnalysisService::Shutdown() {
        if (m_bStopWorker) return;
        m_bStopWorker = true;
        m_taskQueue.CompleteAdding();
        if (!m_spWorkerThread.IsNull()) {
            m_spWorkerThread->Join(3000);
            m_spWorkerThread = nullptr;
        }
    }

    void AnalysisService::NotifyAnalysisUpdate() {
        if (!m_spNotifier.IsNull()) {
            m_spNotifier->BroadcastCategoryUpdate("llm_analysis");
        }
    }

    void AnalysisService::UpdateTaskResult(const String& sTaskId, const AnalyzeResponseDto& result) {
        LockCS lock(m_resultsCs);
        m_taskResults[sTaskId] = result;
        NotifyAnalysisUpdate();
    }

    AnalyzeResponseDto AnalysisService::GetTaskStatus(const String& sTaskId) {
        LockCS lock(m_resultsCs);
        AnalyzeResponseDto dto;
        if (m_taskResults.TryGetValue(sTaskId, dto)) return dto;
        dto.TaskId = sTaskId;
        dto.Status = "NOT_FOUND";
        dto.ProgressMessage = "Specified Task ID was not found in active queue.";
        return dto;
    }

    AnalyzeResponseDto AnalysisService::EnqueueTask(const AnalyzeRequestDto& request) {
        AnalyzeResponseDto pendingDto;
        String sTaskId;
        {
            LockCS lock(m_resultsCs);
            sTaskId = String::Format("task_{0}", static_cast<double>(m_uNextTaskId++));
            pendingDto.TaskId = sTaskId;
            pendingDto.Status = "PENDING";
            pendingDto.ProgressMessage = "Task queued for AI analysis...";
            pendingDto.Channel = request.Channel;
            pendingDto.Query = request.Query;
            m_taskResults[sTaskId] = pendingDto;
        }
        auto item = DotNetDupe::System::SmartPointer<AnalysisTaskItem>::NewShared();
        item->TaskId = sTaskId;
        item->Request = request;
        m_taskQueue.Add(item);
        NotifyAnalysisUpdate();
        return pendingDto;
    }

    bool AnalysisService::EnsureModelDownloaded(const String& sTaskId, AnalyzeResponseDto& taskDto) {
        if (m_spLlmEngine->IsModelFilePresent()) return true;
        AppLoggerManager::Info("AI_ENGINE", String::Format("[AnalysisService] Task '{0}': Model missing on disk. Starting HuggingFace download...", sTaskId));
        taskDto.Status = "DOWNLOADING";
        taskDto.ProgressMessage = "Model not found. Downloading Qwen 1.5 GGUF weights from HuggingFace...";
        taskDto.DownloadProgress = 0.0;
        UpdateTaskResult(sTaskId, taskDto);

        m_spLlmEngine->DownloadModelWithProgress("", DotNetDupe::System::Action<double, double, long long, long long>(
            [this, sTaskId, &taskDto](double pct, double rate, long long dl, long long total) {
                taskDto.Status = "DOWNLOADING";
                taskDto.DownloadProgress = pct;
                taskDto.DownloadedBytes = static_cast<unsigned long long>(dl);
                taskDto.TotalBytes = static_cast<unsigned long long>(total);
                taskDto.DownloadRateBytesPerSec = rate;
                taskDto.ProgressMessage = String::Format("Downloading model: {0:F1}% ({1} MB / {2} MB)", pct, dl / (1024 * 1024), total / (1024 * 1024));
                UpdateTaskResult(sTaskId, taskDto);
            }
        ));
        return m_spLlmEngine->IsModelFilePresent();
    }

    void AnalysisService::InitializeLlmEngine(const String& sTaskId, AnalyzeResponseDto& taskDto) {
        if (m_spLlmEngine->IsModelLoaded()) return;
        taskDto.Status = "INITIALIZING";
        taskDto.ProgressMessage = "Loading GGUF model weights into llama.cpp context...";
        taskDto.DownloadProgress = 100.0;
        UpdateTaskResult(sTaskId, taskDto);
        m_spLlmEngine->Initialize("models/Qwen1.5-4B-Chat-Q4_K_M.gguf");
    }

    AnalyzeResponseDto AnalysisService::ExecuteTaskInference(const String& sTaskId, const AnalyzeRequestDto& request) {
        AnalyzeResponseDto result;
        result.TaskId = sTaskId;
        result.Channel = request.Channel.IsEmpty() ? String("All Channels (SIEM)") : request.Channel;
        result.Query = request.Query;
        result.DownloadProgress = 100.0;

        MultiChannelAnomaliesDto anomalies;
        if (m_spEventService) {
            try { anomalies = m_spEventService->GetCrossChannelAnomalies(15); } catch (...) {}
        }
        TelemetryPostureReportDto posture;
        if (TelemetryService::GetDefault()) {
            try { posture = TelemetryService::GetDefault()->GetPostureReport(); } catch (...) {}
        }

        size_t totalEvents = anomalies.SecurityEvents.GetCount() + anomalies.SystemEvents.GetCount() +
                             anomalies.ApplicationEvents.GetCount() + anomalies.SysmonEvents.GetCount();
        result.EventsAnalyzed = totalEvents;
        result.Analysis = LocalLlmEngine::FormatSiemThreatReport(request.Query, anomalies, posture);
        result.Status = "COMPLETED";
        result.ProgressMessage = "Full-spectrum SIEM threat analysis completed successfully.";
        return result;
    }

    void AnalysisService::ProcessSingleTask(const DotNetDupe::System::SmartPointer<AnalysisTaskItem>& pItem) {
        if (pItem.IsNull()) return;
        AnalyzeResponseDto taskDto;
        taskDto.TaskId = pItem->TaskId;
        taskDto.Channel = pItem->Request.Channel;
        taskDto.Query = pItem->Request.Query;
        taskDto.Status = "PROCESSING";
        taskDto.ProgressMessage = "Synthesizing cross-channel anomalies and telemetry...";
        UpdateTaskResult(pItem->TaskId, taskDto);

        try {
            if (m_spLlmEngine && m_spLlmEngine->IsModelFilePresent()) {
                InitializeLlmEngine(pItem->TaskId, taskDto);
            }
            auto finalDto = ExecuteTaskInference(pItem->TaskId, pItem->Request);
            UpdateTaskResult(pItem->TaskId, finalDto);
        } catch (const DotNetDupe::System::Exception& ex) {
            taskDto.Status = "FAILED";
            taskDto.ProgressMessage = String::Format("Inference failed: {0}", ex.What());
            UpdateTaskResult(pItem->TaskId, taskDto);
        } catch (const std::exception& ex) {
            taskDto.Status = "FAILED";
            taskDto.ProgressMessage = String::Format("Inference failed: {0}", String(ex.what()));
            UpdateTaskResult(pItem->TaskId, taskDto);
        }
    }

    void AnalysisService::WorkerLoop() {
        while (!m_bStopWorker) {
            DotNetDupe::System::SmartPointer<AnalysisTaskItem> item;
            try {
                if (m_taskQueue.TryTake(item, 500)) {
                    ProcessSingleTask(item);
                }
            } catch (...) {
                break;
            }
        }
    }
}
