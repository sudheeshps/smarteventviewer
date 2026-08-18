#pragma once

#include "ViewerCommon.h"
#include "Core/EventRecord.h"
#include "Core/IAnomalyEngine.h"

namespace SmartEventViewer {
    class SMARTEVENTVIEWER_API AnomalyEngine : public IAnomalyEngine {
    public:
        AnomalyEngine() = default;
        ~AnomalyEngine() override = default;

        RiskLevel EvaluateRisk(const EventRecord& eventRec) override;
        bool EvaluateProcess(const ProcessResourceDto& proc, ProcessAnomalyDto& outAnomaly) override;
        bool EvaluateSession(const RdpSessionDto& rdp, SessionAnomalyDto& outAnomaly) override;
        bool EvaluateUser(const UserPrincipalDto& user, UserAnomalyDto& outAnomaly) override;
        bool EvaluateService(const ServiceInfoDto& service, ServiceAnomalyDto& outAnomaly) override;
        TelemetryPostureReportDto EvaluatePosture(const SystemMetricsResponseDto& metrics, const ServicesResponseDto& services) override;

        static RiskLevel StaticEvaluateRisk(const EventRecord& eventRec);
        static bool StaticEvaluateProcess(const ProcessResourceDto& proc, ProcessAnomalyDto& outAnomaly);
        static bool StaticEvaluateSession(const RdpSessionDto& rdp, SessionAnomalyDto& outAnomaly);
        static bool StaticEvaluateUser(const UserPrincipalDto& user, UserAnomalyDto& outAnomaly);
        static bool StaticEvaluateService(const ServiceInfoDto& service, ServiceAnomalyDto& outAnomaly);
        static TelemetryPostureReportDto StaticEvaluatePosture(const SystemMetricsResponseDto& metrics, const ServicesResponseDto& services);
    };
}
