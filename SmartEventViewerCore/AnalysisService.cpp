#include "pch.h"
#include "Core/AnalysisService.h"
#include "Ai/AnalysisStates.h"
#include "Core/EventService.h"
#include "Core/TelemetryService.h"
#include "System/Console.h"
#include "System/Convert.h"
#include "System/Threading/Thread.h"
#include "System/Net/Http/FileDownloader.h"
#include "Logging/AppLoggerManager.h"
#include <exception>

using Console = DotNetDupe::System::Console;
using Thread = DotNetDupe::System::Threading::Thread;
using ThreadStart = DotNetDupe::System::Threading::ThreadStart;

namespace SmartEventViewer {
    AnalysisService::AnalysisService()
        : m_spLlmEngine(DotNetDupe::System::SmartPointer<LocalLlmEngine>::NewShared()),
          m_spEventService(DotNetDupe::System::SmartPointer<IEventService>(DotNetDupe::System::SmartPointer<EventService>::NewShared())),
          m_spNotifier(nullptr),
          m_stateChanged(),
          m_progressChanged(),
          StateChanged(m_stateChanged),
          ProgressChanged(m_progressChanged) {
        StartWorker();
    }

    AnalysisService::AnalysisService(
        const DotNetDupe::System::SmartPointer<LocalLlmEngine>& spLlmEngine,
        const DotNetDupe::System::SmartPointer<IEventService>& spEventService,
        const DotNetDupe::System::SmartPointer<ITelemetryPushNotifier>& spNotifier)
        : m_spLlmEngine(spLlmEngine.IsNull() ? DotNetDupe::System::SmartPointer<LocalLlmEngine>::NewShared() : spLlmEngine),
          m_spEventService(spEventService.IsNull() ? DotNetDupe::System::SmartPointer<IEventService>(DotNetDupe::System::SmartPointer<EventService>::NewShared()) : spEventService),
          m_spNotifier(spNotifier),
          m_stateChanged(),
          m_progressChanged(),
          StateChanged(m_stateChanged),
          ProgressChanged(m_progressChanged) {
        StartWorker();
    }

    AnalysisService::~AnalysisService() {
        Shutdown();
    }

    DotNetDupe::System::SmartPointer<AnalysisService> AnalysisService::GetSharedInstance() {
        static auto s_instance = DotNetDupe::System::SmartPointer<AnalysisService>::NewShared();
        return s_instance;
    }

    void AnalysisService::SetPushNotifier(const DotNetDupe::System::SmartPointer<ITelemetryPushNotifier>& spNotifier) {
        m_spNotifier = spNotifier;
    }

    void AnalysisService::SetState(const DotNetDupe::System::SmartPointer<IAnalysisState>& spNextState) {
        m_spCurrentState = spNextState;
    }

    DotNetDupe::System::SmartPointer<IAnalysisState> AnalysisService::GetCurrentState() const {
        return m_spCurrentState;
    }

    DotNetDupe::System::SmartPointer<LocalLlmEngine> AnalysisService::GetLlmEngine() const {
        return m_spLlmEngine;
    }

    DotNetDupe::System::SmartPointer<IEventService> AnalysisService::GetEventService() const {
        return m_spEventService;
    }

    DotNetDupe::System::SmartPointer<ITelemetryPushNotifier> AnalysisService::GetNotifier() const {
        return m_spNotifier;
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

    void AnalysisService::RaiseStateChanged(const AnalysisStateChangedEventArgs& e) {
        {
            LockCS lock(m_resultsCs);
            if (m_taskResults.ContainsKey(e.GetTaskId())) {
                if (e.IsTerminal()) {
                    m_taskResults[e.GetTaskId()] = e.GetResponse();
                } else {
                    m_taskResults[e.GetTaskId()].Status = e.GetStatus();
                    m_taskResults[e.GetTaskId()].ProgressMessage = e.GetProgressMessage();
                }
            }
        }
        m_stateChanged.Invoke(this, e);
        NotifyAnalysisUpdate();
    }

    static void UpdateDownloadDetails(AnalyzeResponseDto& dto, const AnalysisProgressChangedEventArgs& e) {
        auto spDl = e.GetDetailsAs<DotNetDupe::System::Net::Http::DownloadProgressChangedEventArgs>();
        if (!spDl.IsNull()) {
            dto.DownloadProgress = e.GetProgressPercentage();
            dto.DownloadedBytes = static_cast<unsigned long long>(spDl->GetBytesReceived());
            dto.TotalBytes = static_cast<unsigned long long>(spDl->GetTotalBytesToReceive());
            dto.DownloadRateBytesPerSec = spDl->GetDownloadRateBytesPerSec();
        }
    }

    void AnalysisService::RaiseProgressChanged(const AnalysisProgressChangedEventArgs& e) {
        {
            LockCS lock(m_resultsCs);
            if (m_taskResults.ContainsKey(e.GetTaskId())) {
                auto& taskDto = m_taskResults[e.GetTaskId()];
                taskDto.ProgressPercentage = e.GetProgressPercentage();
                taskDto.ProgressMessage = e.GetProgressMessage();
                if (e.HasDetails()) {
                    UpdateDownloadDetails(taskDto, e);
                }
            }
        }
        m_progressChanged.Invoke(this, e);
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

    static void HandlePipelineFailure(AnalysisService& context, const DotNetDupe::System::SmartPointer<AnalysisTaskItem>& pItem, const String& sError) {
        context.SetState(DotNetDupe::System::SmartPointer<FailedState>::NewShared(sError));
        if (!context.GetCurrentState().IsNull()) {
            context.GetCurrentState()->Execute(context, pItem);
        }
    }

    void AnalysisService::ProcessSingleTask(const DotNetDupe::System::SmartPointer<AnalysisTaskItem>& pItem) {
        if (pItem.IsNull()) return;
        SetState(DotNetDupe::System::SmartPointer<ModelDownloadingState>::NewShared());
        try {
            while (!m_spCurrentState.IsNull()) {
                auto spActiveState = m_spCurrentState;
                spActiveState->Execute(*this, pItem);
                if (spActiveState->IsTerminal()) {
                    break;
                }
            }
        } catch (const DotNetDupe::System::Exception& ex) {
            HandlePipelineFailure(*this, pItem, ex.What());
        } catch (const std::exception& ex) {
            HandlePipelineFailure(*this, pItem, String(ex.what()));
        } catch (...) {
            HandlePipelineFailure(*this, pItem, "Unknown pipeline exception.");
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
