#pragma once

#include "Common.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "System/Text/Json/JsonSerializer.h"
#include "Ai/LocalLlmEngine.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/AutoResetEvent.h"
#include "System/Threading/Thread.h"
#include "System/Collections/Generic/Dictionary.h"
#include "System/Collections/Concurrent/BlockingCollection.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;

    struct AnalyzeRequestDto {
        String Channel{};
        String Query{};

        AnalyzeRequestDto() = default;
        AnalyzeRequestDto(const AnalyzeRequestDto&) = default;
        AnalyzeRequestDto& operator=(const AnalyzeRequestDto&) = default;
    };

    struct AnalyzeResponseDto {
        String TaskId{};
        String Status{}; // "PENDING", "PROCESSING", "DOWNLOADING", "COMPLETED", "FAILED"
        String ProgressMessage{};
        double DownloadProgress{ 0.0 }; // 0.0 to 100.0
        double DownloadRateBytesPerSec{ 0.0 };
        unsigned long long DownloadedBytes{ 0 };
        unsigned long long TotalBytes{ 0 };
        String Channel{};
        String Query{};
        String Analysis{};
        unsigned long long EventsAnalyzed{ 0 };

        AnalyzeResponseDto() = default;
        AnalyzeResponseDto(const AnalyzeResponseDto&) = default;
        AnalyzeResponseDto& operator=(const AnalyzeResponseDto&) = default;
    };

    struct AnalysisTaskItem {
        DotNetDupe::System::String TaskId;
        AnalyzeRequestDto Request;
    };

    class SMARTEVENTVIEWER_API LlmAnalysisController : public DotNetDupe::WebAppCore::Controllers::ControllerBase {
    private:
        DotNetDupe::System::SmartPointer<LocalLlmEngine> m_spLlmEngine{ nullptr };
        static DotNetDupe::System::SmartPointer<LocalLlmEngine> s_spLlmEngine;

        static DotNetDupe::System::Threading::CriticalSection s_analysisQueueCs;
        static DotNetDupe::System::Threading::CriticalSection s_analysisResultsCs;
        static DotNetDupe::System::Threading::AutoResetEvent s_analysisEvent;
        static DotNetDupe::System::Collections::Concurrent::BlockingCollection<DotNetDupe::System::SmartPointer<AnalysisTaskItem>> s_analysisQueue;
        static DotNetDupe::System::Collections::Generic::Dictionary<DotNetDupe::System::String, AnalyzeResponseDto> s_analysisResults;
        static unsigned long long s_uNextTaskId;
        static DotNetDupe::System::SmartPointer<DotNetDupe::System::Threading::Thread> s_spAnalysisWorkerThread;
        static bool s_bWorkerInitialized;
        static void EnsureWorkerStarted();
        static void AnalysisWorkerLoop();
        static bool EnsureModelFileAvailable(const DotNetDupe::System::String& sTaskId);
        static DotNetDupe::System::Collections::Generic::List<EventRecord> AggregateTaskEvents(const DotNetDupe::System::String& sTaskId, const DotNetDupe::System::String& sChannel);
        static void UpdateDownloadProgressStatus(const DotNetDupe::System::String& sTaskId, double progressPct, double rateBytesPerSec, long long downloadedBytes, long long totalBytes);
        static String RunEngineInference(const DotNetDupe::System::String& sTaskId, const AnalyzeRequestDto& request, const DotNetDupe::System::Collections::Generic::List<EventRecord>& events);
        static AnalyzeResponseDto ProcessAnalysisTask(const DotNetDupe::System::String& sTaskId, const AnalyzeRequestDto& request);
        static AnalyzeResponseDto CreatePendingResponse(const DotNetDupe::System::String& sTaskId, const AnalyzeRequestDto& request);

    public:
        static DotNetDupe::System::Threading::AutoResetEvent s_shutdownEvent;
        static bool s_bStopWorker;
        LlmAnalysisController();
        explicit LlmAnalysisController(const DotNetDupe::System::SmartPointer<LocalLlmEngine>& spLlmEngine);
        ~LlmAnalysisController() override = default;

        AnalyzeResponseDto AnalyzeEvents(const AnalyzeRequestDto& request);
        AnalyzeResponseDto GetAnalyzeStatus(const DotNetDupe::System::String& sTaskId);
        static void Shutdown();
    };
}

namespace DotNetDupe {
    namespace System {
        namespace Text {
            namespace Json {
                template <typename Enable>
                struct JsonConverter<SmartEventViewer::AnalyzeRequestDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::AnalyzeRequestDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("channel", JsonElement(value.Channel));
                        obj.SetProperty("query", JsonElement(value.Query));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeRequestDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeRequestDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("channel", prop) || element.TryGetProperty("Channel", prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty("query", prop) || element.TryGetProperty("Query", prop)) dto.Query = prop.GetString();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::AnalyzeResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::AnalyzeResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("taskId", JsonElement(value.TaskId));
                        obj.SetProperty("status", JsonElement(value.Status));
                        obj.SetProperty("progressMessage", JsonElement(value.ProgressMessage));
                        obj.SetProperty("downloadProgress", JsonElement(value.DownloadProgress));
                        obj.SetProperty("downloadRateBytesPerSec", JsonElement(value.DownloadRateBytesPerSec));
                        obj.SetProperty("downloadedBytes", JsonElement(static_cast<double>(value.DownloadedBytes)));
                        obj.SetProperty("totalBytes", JsonElement(static_cast<double>(value.TotalBytes)));
                        obj.SetProperty("channel", JsonElement(value.Channel));
                        obj.SetProperty("query", JsonElement(value.Query));
                        obj.SetProperty("analysis", JsonElement(value.Analysis));
                        obj.SetProperty("eventsAnalyzed", JsonElement(static_cast<double>(value.EventsAnalyzed)));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("taskId", prop)) dto.TaskId = prop.GetString();
                        if (element.TryGetProperty("status", prop)) dto.Status = prop.GetString();
                        if (element.TryGetProperty("progressMessage", prop)) dto.ProgressMessage = prop.GetString();
                        if (element.TryGetProperty("downloadProgress", prop)) dto.DownloadProgress = prop.GetDouble();
                        if (element.TryGetProperty("downloadRateBytesPerSec", prop)) dto.DownloadRateBytesPerSec = prop.GetDouble();
                        if (element.TryGetProperty("downloadedBytes", prop)) dto.DownloadedBytes = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("totalBytes", prop)) dto.TotalBytes = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("channel", prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty("query", prop)) dto.Query = prop.GetString();
                        if (element.TryGetProperty("analysis", prop)) dto.Analysis = prop.GetString();
                        if (element.TryGetProperty("eventsAnalyzed", prop)) dto.EventsAnalyzed = static_cast<unsigned long long>(prop.GetDouble());
                        return dto;
                    }
                };
            }
        }
    }
}
