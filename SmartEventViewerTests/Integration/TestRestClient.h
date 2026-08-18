#pragma once

#include "Common.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include "System/Convert.h"
#include "System/Net/Http/RestClient.h"
#include "Controllers/EventsController.h"
#include "Controllers/TelemetryController.h"
#include "Controllers/DiagnosticsController.h"
#include "Controllers/LlmAnalysisController.h"

namespace SmartEventViewer {
namespace IntegrationTests {

    using String = DotNetDupe::System::String;
    using namespace DotNetDupe::System::Net::Http;

    class TestRestClient : public DotNetDupe::System::Object {
    private:
        String m_sBaseUrl;

        String BuildEventsQuery(const String& sChannel, const String& sLevel, size_t nPage, size_t nPageSize) const {
            String sQuery = "events?channel=" + sChannel;
            sQuery = sQuery + "&level=" + sLevel;
            sQuery = sQuery + "&page=" + DotNetDupe::System::Convert::ToString(static_cast<unsigned long long>(nPage));
            sQuery = sQuery + "&pageSize=" + DotNetDupe::System::Convert::ToString(static_cast<unsigned long long>(nPageSize));
            return sQuery;
        }

    public:
        explicit TestRestClient(const String& sBaseUrl = "http://127.0.0.1:8080") : m_sBaseUrl(sBaseUrl) {}
        ~TestRestClient() override = default;

        ChannelsResponseDto GetChannels() {
            RestClient<ChannelsResponseDto> restClient(m_sBaseUrl + "/api");
            return restClient.Get("channels");
        }

        EventSummaryResponseDto GetEventSummary(const String& sChannel = "") {
            RestClient<EventSummaryResponseDto> restClient(m_sBaseUrl + "/api/events");
            if (sChannel.IsEmpty()) {
                return restClient.Get("summary");
            }
            return restClient.Get("summary?channel=" + sChannel);
        }

        EventLogResponseDto GetEvents(const String& sChannel, const String& sLevel = "ALL", size_t nPage = 1, size_t nPageSize = 20) {
            RestClient<EventLogResponseDto> restClient(m_sBaseUrl + "/api");
            String sEndpoint = BuildEventsQuery(sChannel, sLevel, nPage, nPageSize);
            return restClient.Get(sEndpoint);
        }

        MultiChannelAnomaliesDto GetAnomalies(size_t limit = 15) {
            RestClient<MultiChannelAnomaliesDto> restClient(m_sBaseUrl + "/api/events");
            return restClient.Get("anomalies?limit=" + String::Format("{0}", static_cast<double>(limit)));
        }

        SystemMetricsResponseDto GetMetricsSummary() {
            RestClient<SystemMetricsResponseDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("summary");
        }

        SystemMetricsResponseDto GetCpuMetrics() {
            RestClient<SystemMetricsResponseDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("cpu");
        }

        SystemMetricsResponseDto GetMemoryMetrics() {
            RestClient<SystemMetricsResponseDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("memory");
        }

        SystemMetricsResponseDto GetDiskMetrics() {
            RestClient<SystemMetricsResponseDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("disk");
        }

        SystemMetricsResponseDto GetNetworkMetrics() {
            RestClient<SystemMetricsResponseDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("network");
        }

        SystemMetricsResponseDto GetProcesses() {
            RestClient<SystemMetricsResponseDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("processes");
        }

        SystemMetricsResponseDto GetSessions() {
            RestClient<SystemMetricsResponseDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("sessions");
        }

        ServicesResponseDto GetServices() {
            RestClient<ServicesResponseDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("services");
        }

        TelemetryPostureReportDto GetPosture() {
            RestClient<TelemetryPostureReportDto> restClient(m_sBaseUrl + "/api/metrics");
            return restClient.Get("posture");
        }

        LogFormatResponseDto GetLogFormat() {
            RestClient<LogFormatResponseDto> restClient(m_sBaseUrl + "/api/logs");
            return restClient.Get("format");
        }

        ServerLogsResponseDto GetServerLogs() {
            RestClient<ServerLogsResponseDto> restClient(m_sBaseUrl + "/api");
            return restClient.Get("logs");
        }

        AnalyzeResponseDto AnalyzeEvents(const AnalyzeRequestDto& requestDto) {
            RestClient<AnalyzeRequestDto> restClient(m_sBaseUrl + "/api/analyze");
            return restClient.PostAndReturn<AnalyzeResponseDto>(requestDto);
        }

        AnalyzeResponseDto GetAnalyzeStatus(const String& sTaskId) {
            RestClient<AnalyzeResponseDto> restClient(m_sBaseUrl + "/api/analyze");
            return restClient.Get("status?taskId=" + sTaskId);
        }
    };

}
}
