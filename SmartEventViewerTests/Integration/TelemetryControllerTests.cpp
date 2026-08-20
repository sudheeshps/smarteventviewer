#include <gtest/gtest.h>
#include "Controllers/TelemetryController.h"
#include "System/Console.h"

using namespace DotNetDupe::System;
using namespace SmartEventViewer;

static void LogAndAssertProcessResource(const ProcessResourceDto& dto) {
    Console::WriteLine("  [PROCESS_DTO] PID={0} | Name={1} | CPU={2}% | Mem={3}MB | NetRead={4} | NetWrite={5}",
        dto.ProcessId, dto.Name, dto.CpuUsagePercent, dto.MemoryUsageMB, dto.NetworkReadBytes, dto.NetworkWriteBytes);
    EXPECT_GT(dto.ProcessId, 0UL);
    EXPECT_FALSE(dto.Name.IsEmpty());
    EXPECT_GE(dto.CpuUsagePercent, 0.0);
    EXPECT_LE(dto.CpuUsagePercent, 100.0);
    EXPECT_GE(dto.MemoryUsageMB, 0ULL);
}

static void LogAndAssertUserPrincipal(const UserPrincipalDto& dto) {
    Console::WriteLine("  [USER_DTO] User={0} | Sid={1} | Class={2} | Groups={3} | Perms={4}",
        dto.Username, dto.SidOrUid, dto.UserClass, dto.Groups.GetCount(), dto.Permissions.GetCount());
    EXPECT_FALSE(dto.Username.IsEmpty());
    EXPECT_FALSE(dto.SidOrUid.IsEmpty());
    EXPECT_FALSE(dto.UserClass.IsEmpty());
}

static void LogAndAssertServiceInfo(const ServiceInfoDto& dto) {
    Console::WriteLine("  [SERVICE_DTO] Name={0} | Display={1} | Status={2} | StartType={3} | PID={4}",
        dto.ServiceName, dto.DisplayName, dto.Status, dto.StartType, dto.ProcessId);
    EXPECT_FALSE(dto.ServiceName.IsEmpty());
    EXPECT_FALSE(dto.DisplayName.IsEmpty());
    EXPECT_FALSE(dto.Status.IsEmpty());
    EXPECT_FALSE(dto.StartType.IsEmpty());
    EXPECT_GE(dto.ProcessId, 0);
}

static void LogAndAssertSystemMetrics(const SystemMetricsResponseDto& resp) {
    Console::WriteLine("[METRICS_DTO] CPU={0}% | Mem={1}% ({2}/{3} MB) | Disk={4}% (R:{5} MB/s, W:{6} MB/s) | Net={7} Mbps",
        resp.CpuUsagePercent, resp.MemoryUsagePercent, resp.MemoryUsedMB, resp.MemoryTotalMB,
        resp.DiskUsagePercent, resp.DiskReadMBps, resp.DiskWriteMBps, resp.NetworkUsageMbps);
    EXPECT_GE(resp.CpuUsagePercent, 0.0);
    EXPECT_LE(resp.CpuUsagePercent, 100.0);
    EXPECT_GE(resp.MemoryUsagePercent, 0.0);
    EXPECT_LE(resp.MemoryUsagePercent, 100.0);
    EXPECT_GT(resp.MemoryTotalMB, 0ULL);
    EXPECT_LE(resp.MemoryUsedMB, resp.MemoryTotalMB);
}

TEST(TelemetryControllerTests, GivenController_WhenGetMetricsSummaryCalled_ThenReturnsTelemetryData) {
    TelemetryController controller;
    SystemMetricsResponseDto response = controller.GetSummary();
    LogAndAssertSystemMetrics(response);
    EXPECT_GE(response.TopProcesses.GetCount(), 0);
    EXPECT_GE(response.ActiveUserSessions.GetCount(), 0);
    EXPECT_GE(response.SystemUsers.GetCount(), 0);
    EXPECT_GE(response.RdpSessions.GetCount(), 0);
}

TEST(TelemetryControllerTests, GivenController_WhenGetCpuMetricsCalled_ThenReturnsCpuUsage) {
    TelemetryController controller;
    SystemMetricsResponseDto response = controller.GetCpuUsage();
    EXPECT_GE(response.CpuUsagePercent, 0.0);
    EXPECT_LE(response.CpuUsagePercent, 100.0);
}

TEST(TelemetryControllerTests, GivenController_WhenGetMemoryMetricsCalled_ThenReturnsMemoryUsage) {
    TelemetryController controller;
    SystemMetricsResponseDto response = controller.GetMemoryUsage();
    EXPECT_GE(response.MemoryUsagePercent, 0.0);
    EXPECT_LE(response.MemoryUsagePercent, 100.0);
    EXPECT_GT(response.MemoryTotalMB, 0ULL);
}

TEST(TelemetryControllerTests, GivenController_WhenGetDiskMetricsCalled_ThenReturnsDiskMetrics) {
    TelemetryController controller;
    SystemMetricsResponseDto response = controller.GetDiskUsage();
    EXPECT_GE(response.DiskUsagePercent, 0.0);
    EXPECT_LE(response.DiskUsagePercent, 100.0);
    EXPECT_GE(response.DiskReadMBps, 0.0);
    EXPECT_GE(response.DiskWriteMBps, 0.0);
}

TEST(TelemetryControllerTests, GivenController_WhenGetNetworkMetricsCalled_ThenReturnsNetworkMetrics) {
    TelemetryController controller;
    SystemMetricsResponseDto response = controller.GetNetworkUsage();
    EXPECT_GE(response.NetworkUsageMbps, 0.0);
}

TEST(TelemetryControllerTests, GivenController_WhenGetProcessesCalled_ThenReturnsTopProcessList) {
    TelemetryController controller;
    SystemMetricsResponseDto response = controller.GetProcesses();
    EXPECT_GE(response.TopProcesses.GetCount(), 0);
    for (int i = 0; i < response.TopProcesses.GetCount(); ++i) {
        LogAndAssertProcessResource(response.TopProcesses[i]);
    }
}

TEST(TelemetryControllerTests, GivenController_WhenGetSessionsCalled_ThenReturnsSessionDetails) {
    TelemetryController controller;
    SystemMetricsResponseDto response = controller.GetSessions();
    EXPECT_GE(response.ActiveUserSessions.GetCount(), 0);
    EXPECT_GE(response.RdpSessions.GetCount(), 0);
    for (int i = 0; i < response.SystemUsers.GetCount(); ++i) {
        LogAndAssertUserPrincipal(response.SystemUsers[i]);
    }
}

TEST(TelemetryControllerTests, GivenController_WhenGetServicesCalled_ThenReturnsWindowsServices) {
    TelemetryController controller;
    ServicesResponseDto response = controller.GetServices();
    EXPECT_GE(response.Services.GetCount(), 0);
    for (int i = 0; i < response.Services.GetCount() && i < 5; ++i) {
        LogAndAssertServiceInfo(response.Services[i]);
    }
}

TEST(TelemetryControllerTests, GivenController_WhenGetPostureCalled_ThenReturnsPostureReport) {
    TelemetryController controller;
    TelemetryPostureReportDto response = controller.GetPosture();
    EXPECT_GE(response.ThreatScore, 0);
    EXPECT_LE(response.ThreatScore, 100);
    EXPECT_FALSE(response.OverallRisk.IsEmpty());
}
