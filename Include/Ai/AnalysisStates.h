#pragma once

#include "ViewerCommon.h"
#include "Ai/IAnalysisState.h"
#include "Dto/EventDtos.h"
#include "Dto/TelemetryDtos.h"

namespace SmartEventViewer {

    class SMARTEVENTVIEWER_API ModelDownloadingState : public IAnalysisState {
    public:
        ModelDownloadingState() = default;
        ~ModelDownloadingState() override = default;

        String GetStateName() const override { return "ModelDownloading"; }
        String GetPublicStatus() const override { return "DOWNLOADING"; }
        void Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) override;
    };

    class SMARTEVENTVIEWER_API ModelInitializingState : public IAnalysisState {
    public:
        ModelInitializingState() = default;
        ~ModelInitializingState() override = default;

        String GetStateName() const override { return "ModelInitializing"; }
        String GetPublicStatus() const override { return "INITIALIZING"; }
        void Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) override;
    };

    class SMARTEVENTVIEWER_API EventIngestingState : public IAnalysisState {
    public:
        EventIngestingState() = default;
        ~EventIngestingState() override = default;

        String GetStateName() const override { return "EventIngesting"; }
        String GetPublicStatus() const override { return "PROCESSING"; }
        void Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) override;
    };

    class SMARTEVENTVIEWER_API PromptSetupState : public IAnalysisState {
    private:
        MultiChannelAnomaliesDto m_anomalies{};
        TelemetryPostureReportDto m_posture{};

    public:
        PromptSetupState() = default;
        PromptSetupState(const MultiChannelAnomaliesDto& anomalies, const TelemetryPostureReportDto& posture)
            : m_anomalies(anomalies), m_posture(posture) {}
        ~PromptSetupState() override = default;

        String GetStateName() const override { return "PromptSetup"; }
        String GetPublicStatus() const override { return "PROCESSING"; }
        void Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) override;
    };

    class SMARTEVENTVIEWER_API AnalysisExecutionState : public IAnalysisState {
    private:
        MultiChannelAnomaliesDto m_anomalies{};
        TelemetryPostureReportDto m_posture{};
        String m_sPrompt{};

    public:
        AnalysisExecutionState() = default;
        AnalysisExecutionState(
            const MultiChannelAnomaliesDto& anomalies,
            const TelemetryPostureReportDto& posture,
            const String& sPrompt)
            : m_anomalies(anomalies), m_posture(posture), m_sPrompt(sPrompt) {}
        ~AnalysisExecutionState() override = default;

        String GetStateName() const override { return "AnalysisExecution"; }
        String GetPublicStatus() const override { return "PROCESSING"; }
        void Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) override;
    };

    class SMARTEVENTVIEWER_API CompletedState : public IAnalysisState {
    private:
        String m_sReport{};
        size_t m_uTotalEvents{ 0 };

    public:
        CompletedState() = default;
        CompletedState(const String& sReport, size_t uTotalEvents)
            : m_sReport(sReport), m_uTotalEvents(uTotalEvents) {}
        ~CompletedState() override = default;

        String GetStateName() const override { return "Completed"; }
        String GetPublicStatus() const override { return "COMPLETED"; }
        bool IsTerminal() const override { return true; }
        void Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) override;
    };

    class SMARTEVENTVIEWER_API FailedState : public IAnalysisState {
    private:
        String m_sErrorMessage{};

    public:
        FailedState() : m_sErrorMessage("SIEM analysis failed.") {}
        explicit FailedState(const String& sErrorMessage) : m_sErrorMessage(sErrorMessage) {}
        ~FailedState() override = default;

        String GetStateName() const override { return "Failed"; }
        String GetPublicStatus() const override { return "FAILED"; }
        bool IsTerminal() const override { return true; }
        void Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) override;
    };
}
