#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include "System/Action.h"
#include "System/Collections/Generic/List.h"
#include "System/Collections/Generic/Dictionary.h"
#include "System/Collections/Concurrent/BlockingCollection.h"
#include "System/Threading/Thread.h"
#include "Ai/ILlamaModelProvider.h"
#include "Ai/LlamaMessageDtos.h"
#include "Dto/EventDtos.h"
#include "Dto/TelemetryDtos.h"

// Forward declaration of internal llama context structures
struct llama_model;
struct llama_context;

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;
    template<typename T>
    using List = DotNetDupe::System::Collections::Generic::List<T>;
    template<typename T>
    using SmartPointer = DotNetDupe::System::SmartPointer<T>;

    class EventRecord;

    class LocalLlmEngine : public DotNetDupe::System::Object {
    private:
        bool m_bIsLoaded{ false };
        StringList m_listConversationHistory{};

        // Abstracted llama.cpp model provider interface
        SmartPointer<ILlamaModelProvider> m_spModelProvider{ nullptr };

        // Channel event count cache map for high-speed ingestion bypass
        DotNetDupe::System::Collections::Generic::Dictionary<String, unsigned long long> m_mapChannelEventCounts{};

        // Dual-thread BlockingCollection queues for Producer-Consumer architecture
        DotNetDupe::System::Collections::Concurrent::BlockingCollection<SmartPointer<LlamaRequest>> m_requestQueue{};
        DotNetDupe::System::Collections::Concurrent::BlockingCollection<SmartPointer<LlamaResponse>> m_responseQueue{};

        SmartPointer<DotNetDupe::System::Threading::Thread> m_spWorkerThread{ nullptr };
        SmartPointer<DotNetDupe::System::Threading::Thread> m_spNotifierThread{ nullptr };
        bool m_bStopEngine{ false };

        void StartBackgroundThreads();
        void WorkerThreadLoop();
        void NotifierThreadLoop();
        SmartPointer<LlamaResponse> HandleRequest(const SmartPointer<LlamaRequest>& pReq);

        bool ExecuteFileDownloader(const String& sUrl, const String& sTargetPath, DotNetDupe::System::Action<double, double, long long, long long> progressCb);

    public:
        SMARTEVENTVIEWER_API static void CountRiskMetrics(const List<EventRecord>& events, unsigned int& crit, unsigned int& high, unsigned int& err, unsigned int& warn);
        SMARTEVENTVIEWER_API static int ComputeThreatScore(unsigned int crit, unsigned int high, unsigned int err, unsigned int warn, unsigned int total);
        SMARTEVENTVIEWER_API static String GetSeverityLabel(int score);
        SMARTEVENTVIEWER_API static String FormatAnomaliesSection(const List<EventRecord>& events);
        SMARTEVENTVIEWER_API static String FormatThreatAnalysisResponse(const List<EventRecord>& events);
        SMARTEVENTVIEWER_API static String BuildLlamaSystemPrompt();
        SMARTEVENTVIEWER_API static String FormatEventContextForLlama(const List<EventRecord>& events);

        SMARTEVENTVIEWER_API static String FormatSiemContext(
            const MultiChannelAnomaliesDto& anomalies,
            const TelemetryPostureReportDto& posture);
        SMARTEVENTVIEWER_API static String FormatSiemThreatReport(
            const String& sUserQuery,
            const MultiChannelAnomaliesDto& anomalies,
            const TelemetryPostureReportDto& posture);

        SMARTEVENTVIEWER_API LocalLlmEngine();
        SMARTEVENTVIEWER_API explicit LocalLlmEngine(const SmartPointer<ILlamaModelProvider>& spProvider);
        SMARTEVENTVIEWER_API ~LocalLlmEngine();

        // Non-copyable for safe memory management of in-process native llama handles
        LocalLlmEngine(const LocalLlmEngine&) = delete;
        LocalLlmEngine& operator=(const LocalLlmEngine&) = delete;

        SMARTEVENTVIEWER_API bool IsModelFilePresent(const String& sModelPath = "") const;
        SMARTEVENTVIEWER_API void DownloadModelFromUrl(const String& sDownloadUrl, const String& sModelPath, DotNetDupe::System::Action<double, double, long long, long long> progressCallback = nullptr);
        SMARTEVENTVIEWER_API void DownloadModelWithProgress(const String& sModelPath, DotNetDupe::System::Action<double, double, long long, long long> progressCallback = nullptr);

        SMARTEVENTVIEWER_API void Initialize(const String& sModelPath);
        SMARTEVENTVIEWER_API void Unload();

        SMARTEVENTVIEWER_API void EnqueueRequest(const SmartPointer<LlamaRequest>& pRequest);
        SMARTEVENTVIEWER_API SmartPointer<LlamaResponse> TakeResponse();

        SMARTEVENTVIEWER_API void ProcessQueryAsync(const String& sNaturalLanguageQuery, const EventRecord* pContextEvents, unsigned int uEventCount, DotNetDupe::System::Action<const String&, const String&, double> callback);
        SMARTEVENTVIEWER_API void ProcessFollowupQueryAsync(const String& sFollowupQuery, const EventRecord* pContextEvents, unsigned int uEventCount, DotNetDupe::System::Action<const String&, const String&, double> callback);
        SMARTEVENTVIEWER_API void ClearConversationHistory();
        SMARTEVENTVIEWER_API size_t GetHistoryCount() const;
        SMARTEVENTVIEWER_API bool IsModelLoaded() const { return m_bIsLoaded; }
    };
}
