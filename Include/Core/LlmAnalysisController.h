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

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;

    struct AnalyzeRequestDto
    {
        String Channel{};
        String Query{};

        AnalyzeRequestDto() = default;
        AnalyzeRequestDto(const AnalyzeRequestDto&) = default;
        AnalyzeRequestDto& operator=(const AnalyzeRequestDto&) = default;
    };

    struct AnalyzeResponseDto
    {
        String TaskId{};
        String Status{}; // "PENDING", "PROCESSING", "COMPLETED", "FAILED"
        String ProgressMessage{};
        String Channel{};
        String Query{};
        String Analysis{};
        unsigned long long EventsAnalyzed{ 0 };

        AnalyzeResponseDto() = default;
        AnalyzeResponseDto(const AnalyzeResponseDto&) = default;
        AnalyzeResponseDto& operator=(const AnalyzeResponseDto&) = default;
    };

    struct AnalysisTaskItem
    {
        DotNetDupe::System::String TaskId;
        AnalyzeRequestDto Request;
    };

    class SMARTEVENTVIEWER_API LlmAnalysisController : public DotNetDupe::WebAppCore::Controllers::ControllerBase
    {
    private:
        DotNetDupe::System::SmartPointer<LocalLlmEngine> m_spLlmEngine{ nullptr };
        static DotNetDupe::System::SmartPointer<LocalLlmEngine> s_spLlmEngine;

        static DotNetDupe::System::Threading::CriticalSection s_analysisQueueCs;
        static DotNetDupe::System::Threading::CriticalSection s_analysisResultsCs;
        static DotNetDupe::System::Threading::AutoResetEvent s_analysisEvent;
        static DotNetDupe::System::Collections::Concurrent::BlockingCollection<DotNetDupe::System::SmartPointer<AnalysisTaskItem>> s_analysisQueue;
        static DotNetDupe::System::Collections::Generic::Dictionary<DotNetDupe::System::String, AnalyzeResponseDto> s_analysisResults;
        static bool s_bStopWorker;
        static unsigned long long s_uNextTaskId;
        static DotNetDupe::System::SmartPointer<DotNetDupe::System::Threading::Thread> s_spAnalysisWorkerThread;
        static bool s_bWorkerInitialized;

        static void EnsureWorkerStarted();
        static void AnalysisWorkerLoop();
        static AnalyzeResponseDto ProcessAnalysisTask(const DotNetDupe::System::String& sTaskId, const AnalyzeRequestDto& request);

    public:
        LlmAnalysisController();
        explicit LlmAnalysisController(const DotNetDupe::System::SmartPointer<LocalLlmEngine>& spLlmEngine);
        ~LlmAnalysisController() override = default;

        AnalyzeResponseDto AnalyzeEvents(const AnalyzeRequestDto& request);
        AnalyzeResponseDto GetAnalyzeStatus(const DotNetDupe::System::String& sTaskId);
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
                        obj.SetProperty(String("channel"), JsonElement(value.Channel));
                        obj.SetProperty(String("query"), JsonElement(value.Query));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeRequestDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeRequestDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("channel"), prop) || element.TryGetProperty(String("Channel"), prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty(String("query"), prop) || element.TryGetProperty(String("Query"), prop)) dto.Query = prop.GetString();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::AnalyzeResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::AnalyzeResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("taskId"), JsonElement(value.TaskId));
                        obj.SetProperty(String("status"), JsonElement(value.Status));
                        obj.SetProperty(String("progressMessage"), JsonElement(value.ProgressMessage));
                        obj.SetProperty(String("channel"), JsonElement(value.Channel));
                        obj.SetProperty(String("query"), JsonElement(value.Query));
                        obj.SetProperty(String("analysis"), JsonElement(value.Analysis));
                        obj.SetProperty(String("eventsAnalyzed"), JsonElement(static_cast<double>(value.EventsAnalyzed)));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("taskId"), prop)) dto.TaskId = prop.GetString();
                        if (element.TryGetProperty(String("status"), prop)) dto.Status = prop.GetString();
                        if (element.TryGetProperty(String("progressMessage"), prop)) dto.ProgressMessage = prop.GetString();
                        if (element.TryGetProperty(String("channel"), prop)) dto.Channel = prop.GetString();
                        if (element.TryGetProperty(String("query"), prop)) dto.Query = prop.GetString();
                        if (element.TryGetProperty(String("analysis"), prop)) dto.Analysis = prop.GetString();
                        if (element.TryGetProperty(String("eventsAnalyzed"), prop)) dto.EventsAnalyzed = static_cast<unsigned long long>(prop.GetDouble());
                        return dto;
                    }
                };
            }
        }
    }
}
