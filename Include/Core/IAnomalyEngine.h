#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "Core/EventRecord.h"
#include "Dto/TelemetryDtos.h"

namespace SmartEventViewer {
    class IAnomalyEngine : public virtual DotNetDupe::System::Object {
    public:
        virtual ~IAnomalyEngine() = default;

        virtual RiskLevel EvaluateRisk(const EventRecord& eventRec) = 0;
        virtual bool EvaluateProcess(const ProcessResourceDto& proc, ProcessAnomalyDto& outAnomaly) = 0;
        virtual bool EvaluateSession(const RdpSessionDto& rdp, SessionAnomalyDto& outAnomaly) = 0;
        virtual bool EvaluateUser(const UserPrincipalDto& user, UserAnomalyDto& outAnomaly) = 0;
        virtual bool EvaluateService(const ServiceInfoDto& service, ServiceAnomalyDto& outAnomaly) = 0;
        virtual TelemetryPostureReportDto EvaluatePosture(const SystemMetricsResponseDto& metrics, const ServicesResponseDto& services) = 0;
    };
}
