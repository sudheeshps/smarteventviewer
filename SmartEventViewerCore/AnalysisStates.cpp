#include "pch.h"
#include "Ai/AnalysisStates.h"
#include "Core/AnalysisService.h"
#include "Core/TelemetryService.h"
#include "Logging/AppLoggerManager.h"
#include "System/Net/Http/FileDownloader.h"

namespace SmartEventViewer {

    void ModelDownloadingState::Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) {
        if (!context.GetLlmEngine().IsNull() && context.GetLlmEngine()->IsModelFilePresent()) {
            context.SetState(SmartPointer<ModelInitializingState>::NewShared());
            return;
        }
        AppLoggerManager::Info("AI_ENGINE", String::Format("[AnalysisPipeline] Task '{0}': Model missing. Starting download...", pItem->TaskId));
        context.RaiseStateChanged(AnalysisStateChangedEventArgs(pItem->TaskId, "None", "ModelDownloading", "DOWNLOADING", "Model not found. Downloading Qwen 1.5 GGUF weights..."));
        if (!context.GetLlmEngine().IsNull()) {
            context.GetLlmEngine()->DownloadModelWithProgress("", DotNetDupe::System::Action<double, double, long long, long long>(
                [&context, pItem](double pct, double rate, long long dl, long long total) {
                    auto spDetails = SmartPointer<DotNetDupe::System::Net::Http::DownloadProgressChangedEventArgs>::NewShared(
                        dl, total, pct, rate, DotNetDupe::System::Net::Http::DownloadStatus::Downloading);
                    long long dlMb = dl / (1024 * 1024);
                    long long totalMb = total / (1024 * 1024);
                    int nPct = static_cast<int>(pct);
                    String sMsg = String::Format("Downloading model: {0}% ({1} MB / {2} MB)", nPct, dlMb, totalMb);
                    context.RaiseProgressChanged(AnalysisProgressChangedEventArgs(pItem->TaskId, pct, sMsg, spDetails));
                }
            ));
        }
        if (!context.GetLlmEngine().IsNull() && context.GetLlmEngine()->IsModelFilePresent()) {
            context.SetState(SmartPointer<ModelInitializingState>::NewShared());
        } else {
            context.SetState(SmartPointer<FailedState>::NewShared("Model download failed or file is not accessible."));
        }
    }

    void ModelInitializingState::Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) {
        context.RaiseStateChanged(AnalysisStateChangedEventArgs(pItem->TaskId, "ModelDownloading", "ModelInitializing", "INITIALIZING", "Loading GGUF model weights into llama.cpp context..."));
        if (!context.GetLlmEngine().IsNull() && !context.GetLlmEngine()->IsModelLoaded()) {
            context.GetLlmEngine()->Initialize("models/Qwen1.5-4B-Chat-Q4_K_M.gguf");
        }
        context.SetState(SmartPointer<EventIngestingState>::NewShared());
    }

    void EventIngestingState::Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) {
        context.RaiseStateChanged(AnalysisStateChangedEventArgs(pItem->TaskId, "ModelInitializing", "EventIngesting", "PROCESSING", "Ingesting multi-channel events and host telemetry..."));
        MultiChannelAnomaliesDto anomalies{};
        if (!context.GetEventService().IsNull()) {
            try { anomalies = context.GetEventService()->GetCrossChannelAnomalies(15); } catch (const DotNetDupe::System::Exception&) {} catch (...) {}
        }
        TelemetryPostureReportDto posture{};
        if (TelemetryService::GetDefault()) {
            try { posture = TelemetryService::GetDefault()->GetPostureReport(); } catch (const DotNetDupe::System::Exception&) {} catch (...) {}
        }
        context.SetState(SmartPointer<PromptSetupState>::NewShared(anomalies, posture));
    }

    void PromptSetupState::Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) {
        context.RaiseStateChanged(AnalysisStateChangedEventArgs(pItem->TaskId, "EventIngesting", "PromptSetup", "PROCESSING", "Synthesizing SIEM security context and query framing..."));
        String sPrompt = LocalLlmEngine::FormatSiemContext(m_anomalies, m_posture);
        context.SetState(SmartPointer<AnalysisExecutionState>::NewShared(m_anomalies, m_posture, sPrompt));
    }

    static size_t SumChannelEvents(const MultiChannelAnomaliesDto& anomalies) {
        return anomalies.SecurityEvents.GetCount() + anomalies.SystemEvents.GetCount() +
               anomalies.ApplicationEvents.GetCount() + anomalies.SysmonEvents.GetCount();
    }

    void AnalysisExecutionState::Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) {
        context.RaiseStateChanged(AnalysisStateChangedEventArgs(pItem->TaskId, "PromptSetup", "AnalysisExecution", "PROCESSING", "Executing SIEM threat analysis inference..."));
        String sReport = LocalLlmEngine::FormatSiemThreatReport(pItem->Request.Query, m_anomalies, m_posture);
        size_t uTotalEvents = SumChannelEvents(m_anomalies);
        context.SetState(SmartPointer<CompletedState>::NewShared(sReport, uTotalEvents));
    }

    void CompletedState::Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) {
        AnalyzeResponseDto result;
        result.TaskId = pItem->TaskId;
        result.Channel = pItem->Request.Channel.IsEmpty() ? String("All Channels (SIEM)") : pItem->Request.Channel;
        result.Query = pItem->Request.Query;
        result.ProgressPercentage = 100.0;
        result.DownloadProgress = 100.0;
        result.EventsAnalyzed = m_uTotalEvents;
        result.Analysis = m_sReport;
        result.Status = "COMPLETED";
        result.ProgressMessage = "Full-spectrum SIEM threat analysis completed successfully.";
        context.RaiseStateChanged(AnalysisStateChangedEventArgs(pItem->TaskId, "AnalysisExecution", "Completed", "COMPLETED", result.ProgressMessage, result, true));
    }

    void FailedState::Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) {
        AnalyzeResponseDto result;
        result.TaskId = pItem->TaskId;
        result.Channel = pItem->Request.Channel;
        result.Query = pItem->Request.Query;
        result.ProgressPercentage = 0.0;
        result.Status = "FAILED";
        String sErrMsg = m_sErrorMessage.IsEmpty() ? String("SIEM analysis pipeline failed.") : m_sErrorMessage;
        result.ProgressMessage = String::Format("Inference failed: {0}", sErrMsg);
        context.RaiseStateChanged(AnalysisStateChangedEventArgs(pItem->TaskId, "Unknown", "Failed", "FAILED", result.ProgressMessage, result, true));
    }
}
