#pragma once

#include "ViewerCommon.h"
#include "Core/IAnalysisService.h"
#include "Ai/IAnalysisState.h"
#include "Ai/AnalysisEvents.h"
#include "Ai/LocalLlmEngine.h"
#include "Core/IEventService.h"
#include "Core/ITelemetryPushNotifier.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/Threading/Thread.h"
#include "System/EventHandler.h"
#include "System/Collections/Generic/Dictionary.h"
#include "System/Collections/Concurrent/BlockingCollection.h"
#include "System/SmartPointer.h"

namespace SmartEventViewer {
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    class SMARTEVENTVIEWER_API AnalysisService : public IAnalysisService {
    private:
        DotNetDupe::System::SmartPointer<IAnalysisState> m_spCurrentState{ nullptr };
        DotNetDupe::System::SmartPointer<LocalLlmEngine> m_spLlmEngine{ nullptr };
        DotNetDupe::System::SmartPointer<IEventService> m_spEventService{ nullptr };
        DotNetDupe::System::SmartPointer<ITelemetryPushNotifier> m_spNotifier{ nullptr };

        DotNetDupe::System::Collections::Concurrent::BlockingCollection<DotNetDupe::System::SmartPointer<AnalysisTaskItem>> m_taskQueue{};
        DotNetDupe::System::Collections::Generic::Dictionary<String, AnalyzeResponseDto> m_taskResults{};
        mutable CriticalSection m_resultsCs{};
        unsigned long long m_uNextTaskId{ 1 };

        DotNetDupe::System::SmartPointer<DotNetDupe::System::Threading::Thread> m_spWorkerThread{ nullptr };
        bool m_bStopWorker{ false };

        DotNetDupe::System::EventHandler<AnalysisStateChangedEventArgs> m_stateChanged{};
        DotNetDupe::System::EventHandler<AnalysisProgressChangedEventArgs> m_progressChanged{};

        void StartWorker();
        void WorkerLoop();
        void ProcessSingleTask(const DotNetDupe::System::SmartPointer<AnalysisTaskItem>& pItem);
        void NotifyAnalysisUpdate();

    public:
        DotNetDupe::System::EventHandler<AnalysisStateChangedEventArgs>& StateChanged;
        DotNetDupe::System::EventHandler<AnalysisProgressChangedEventArgs>& ProgressChanged;

        AnalysisService();
        explicit AnalysisService(
            const DotNetDupe::System::SmartPointer<LocalLlmEngine>& spLlmEngine,
            const DotNetDupe::System::SmartPointer<IEventService>& spEventService = nullptr,
            const DotNetDupe::System::SmartPointer<ITelemetryPushNotifier>& spNotifier = nullptr);
        ~AnalysisService() override;

        static DotNetDupe::System::SmartPointer<AnalysisService> GetSharedInstance();
        void SetPushNotifier(const DotNetDupe::System::SmartPointer<ITelemetryPushNotifier>& spNotifier);
        void SetLlmEngine(const DotNetDupe::System::SmartPointer<LocalLlmEngine>& spLlmEngine);

        void SetState(const DotNetDupe::System::SmartPointer<IAnalysisState>& spNextState);
        DotNetDupe::System::SmartPointer<IAnalysisState> GetCurrentState() const;

        DotNetDupe::System::SmartPointer<LocalLlmEngine> GetLlmEngine() const;
        DotNetDupe::System::SmartPointer<IEventService> GetEventService() const;
        DotNetDupe::System::SmartPointer<ITelemetryPushNotifier> GetNotifier() const;

        void RaiseStateChanged(const AnalysisStateChangedEventArgs& e);
        void RaiseProgressChanged(const AnalysisProgressChangedEventArgs& e);

        AnalyzeResponseDto EnqueueTask(const AnalyzeRequestDto& request) override;
        AnalyzeResponseDto GetTaskStatus(const String& sTaskId) override;
        void Shutdown() override;
    };
}
