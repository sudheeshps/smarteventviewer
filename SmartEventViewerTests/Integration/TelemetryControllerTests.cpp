#include <gtest/gtest.h>
#include "TestRestClient.h"
#include "System/Console.h"

using namespace DotNetDupe::System;
using namespace SmartEventViewer;
using namespace SmartEventViewer::IntegrationTests;

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
    Console::WriteLine("[ASSERT] CPU in [0, 100], Mem in [0, 100], MemUsed <= MemTotal, Disk in [0, 100]");
    EXPECT_GE(resp.CpuUsagePercent, 0.0);
    EXPECT_LE(resp.CpuUsagePercent, 100.0);
    EXPECT_GE(resp.MemoryUsagePercent, 0.0);
    EXPECT_LE(resp.MemoryUsagePercent, 100.0);
    EXPECT_GT(resp.MemoryTotalMB, 0ULL);
    EXPECT_LE(resp.MemoryUsedMB, resp.MemoryTotalMB);
}

TEST(TelemetryControllerTests, GivenRunningServer_WhenGetMetricsSummaryCalled_ThenReturnsTelemetryData) {
    Console::WriteLine("[TEST] Invoking GET /api/metrics/summary...");
    TestRestClient client;
    SystemMetricsResponseDto response = client.GetMetricsSummary();
    LogAndAssertSystemMetrics(response);
    Console::WriteLine("[ASSERT] Processes={0}, ActiveSessions={1}, SystemUsers={2}, RdpSessions={3}",
        response.TopProcesses.GetCount(), response.ActiveUserSessions.GetCount(), response.SystemUsers.GetCount(), response.RdpSessions.GetCount());
    EXPECT_GE(response.TopProcesses.GetCount(), 0);
    EXPECT_GE(response.ActiveUserSessions.GetCount(), 0);
    EXPECT_GE(response.SystemUsers.GetCount(), 0);
    EXPECT_GE(response.RdpSessions.GetCount(), 0);
}

TEST(TelemetryControllerTests, GivenRunningServer_WhenGetCpuMetricsCalled_ThenReturnsCpuUsage) {
    Console::WriteLine("[TEST] Invoking GET /api/metrics/cpu...");
    TestRestClient client;
    SystemMetricsResponseDto response = client.GetCpuMetrics();
    Console::WriteLine("[CPU_DTO] CpuUsagePercent={0}%", response.CpuUsagePercent);
    Console::WriteLine("[ASSERT] CpuUsagePercent >= 0.0 && CpuUsagePercent <= 100.0");
    EXPECT_GE(response.CpuUsagePercent, 0.0);
    EXPECT_LE(response.CpuUsagePercent, 100.0);
}

TEST(TelemetryControllerTests, GivenRunningServer_WhenGetMemoryMetricsCalled_ThenReturnsMemoryUsage) {
    Console::WriteLine("[TEST] Invoking GET /api/metrics/memory...");
    TestRestClient client;
    SystemMetricsResponseDto response = client.GetMemoryMetrics();
    Console::WriteLine("[MEM_DTO] MemoryUsagePercent={0}% | Used={1}MB | Total={2}MB",
        response.MemoryUsagePercent, response.MemoryUsedMB, response.MemoryTotalMB);
    Console::WriteLine("[ASSERT] MemoryUsagePercent in [0, 100] && MemoryTotalMB > 0");
    EXPECT_GE(response.MemoryUsagePercent, 0.0);
    EXPECT_LE(response.MemoryUsagePercent, 100.0);
    EXPECT_GT(response.MemoryTotalMB, 0ULL);
}

TEST(TelemetryControllerTests, GivenRunningServer_WhenGetDiskMetricsCalled_ThenReturnsDiskMetrics) {
    Console::WriteLine("[TEST] Invoking GET /api/metrics/disk...");
    TestRestClient client;
    SystemMetricsResponseDto response = client.GetDiskMetrics();
    Console::WriteLine("[DISK_DTO] DiskUsage={0}% | Read={1} MB/s | Write={2} MB/s",
        response.DiskUsagePercent, response.DiskReadMBps, response.DiskWriteMBps);
    Console::WriteLine("[ASSERT] DiskUsagePercent in [0, 100] && DiskReadMBps >= 0 && DiskWriteMBps >= 0");
    EXPECT_GE(response.DiskUsagePercent, 0.0);
    EXPECT_LE(response.DiskUsagePercent, 100.0);
    EXPECT_GE(response.DiskReadMBps, 0.0);
    EXPECT_GE(response.DiskWriteMBps, 0.0);
}

TEST(TelemetryControllerTests, GivenRunningServer_WhenGetNetworkMetricsCalled_ThenReturnsNetworkMetrics) {
    Console::WriteLine("[TEST] Invoking GET /api/metrics/network...");
    TestRestClient client;
    SystemMetricsResponseDto response = client.GetNetworkMetrics();
    Console::WriteLine("[NET_DTO] NetworkUsageMbps={0} Mbps", response.NetworkUsageMbps);
    Console::WriteLine("[ASSERT] NetworkUsageMbps >= 0.0");
    EXPECT_GE(response.NetworkUsageMbps, 0.0);
}

TEST(TelemetryControllerTests, GivenRunningServer_WhenGetProcessesCalled_ThenReturnsTopProcessList) {
    Console::WriteLine("[TEST] Invoking GET /api/metrics/processes...");
    TestRestClient client;
    SystemMetricsResponseDto response = client.GetProcesses();
    Console::WriteLine("[PROCESSES_DTO] Count={0}", response.TopProcesses.GetCount());
    Console::WriteLine("[ASSERT] TopProcesses.Count >= 0");
    EXPECT_GE(response.TopProcesses.GetCount(), 0);
    for (int i = 0; i < response.TopProcesses.GetCount(); ++i) {
        LogAndAssertProcessResource(response.TopProcesses[i]);
    }
}

TEST(TelemetryControllerTests, GivenRunningServer_WhenGetSessionsCalled_ThenReturnsSessionDetails) {
    Console::WriteLine("[TEST] Invoking GET /api/metrics/sessions...");
    TestRestClient client;
    SystemMetricsResponseDto response = client.GetSessions();
    Console::WriteLine("[SESSIONS_DTO] ActiveUserSessions={0} | RdpSessions={1} | SystemUsers={2}",
        response.ActiveUserSessions.GetCount(), response.RdpSessions.GetCount(), response.SystemUsers.GetCount());
    EXPECT_GE(response.ActiveUserSessions.GetCount(), 0);
    EXPECT_GE(response.RdpSessions.GetCount(), 0);
    for (int i = 0; i < response.SystemUsers.GetCount(); ++i) {
        LogAndAssertUserPrincipal(response.SystemUsers[i]);
    }
}

TEST(TelemetryControllerTests, GivenRunningServer_WhenGetServicesCalled_ThenReturnsWindowsServices) {
    Console::WriteLine("[TEST] Invoking GET /api/metrics/services...");
    TestRestClient client;
    ServicesResponseDto response = client.GetServices();
    Console::WriteLine("[SERVICES_DTO] ServicesCount={0}", response.Services.GetCount());
    Console::WriteLine("[ASSERT] Services.Count >= 0");
    EXPECT_GE(response.Services.GetCount(), 0);
    for (int i = 0; i < response.Services.GetCount() && i < 5; ++i) {
        LogAndAssertServiceInfo(response.Services[i]);
    }
}
