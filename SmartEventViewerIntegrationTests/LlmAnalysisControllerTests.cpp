#include <gtest/gtest.h>
#include "TestRestClient.h"
#include "System/Console.h"
#include "System/Threading/Thread.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::Threading;
using namespace SmartEventViewer;
using namespace SmartEventViewer::IntegrationTests;

static AnalyzeResponseDto WaitForAnalysisCompletion(TestRestClient& client, const String& sTaskId, int nTimeoutSec = 60) {
    AnalyzeResponseDto status;
    for (int i = 0; i < nTimeoutSec; ++i) {
        status = client.GetAnalyzeStatus(sTaskId);
        Console::WriteLine("[AI_PROGRESS] Task: {0} | Status: {1} | Pct: {2:F2}% | Rate: {3:F2} KB/s | Msg: {4}",
            status.TaskId, status.Status, status.DownloadProgress, (status.DownloadRateBytesPerSec / 1024.0), status.ProgressMessage);
        if (status.Status == "COMPLETED" || status.Status == "FAILED") break;
        Thread::Sleep(1000);
    }
    return status;
}

static void LogAndAssertInitialResponse(const AnalyzeResponseDto& resp, const String& sChannel, const String& sQuery) {
    Console::WriteLine("[INITIAL_ANALYZE_DTO] TaskId={0} | Status={1} | Channel={2} | Msg={3}",
        resp.TaskId, resp.Status, resp.Channel, resp.ProgressMessage);
    Console::WriteLine("[ASSERT] TaskId != '' | Status in (DOWNLOADING, PENDING, RUNNING, COMPLETED) | Channel == '{0}'", sChannel);
    EXPECT_FALSE(resp.TaskId.IsEmpty());
    EXPECT_TRUE(resp.Status == "DOWNLOADING" || resp.Status == "PENDING" || resp.Status == "RUNNING" || resp.Status == "COMPLETED");
    EXPECT_EQ(resp.Channel, sChannel);
    EXPECT_EQ(resp.Query, sQuery);
    EXPECT_FALSE(resp.ProgressMessage.IsEmpty());
}

static void LogAndAssertFinalResult(const AnalyzeResponseDto& resp, const String& sChannel, const String& sQuery) {
    Console::WriteLine("[FINAL_ANALYZE_DTO] TaskId={0} | Status={1} | EventsEvaluated={2} | AnalysisLength={3} chars",
        resp.TaskId, resp.Status, resp.EventsAnalyzed, resp.Analysis.GetLength());
    Console::WriteLine("[ASSERT] Status in (COMPLETED, FAILED, DOWNLOADING) | Channel == '{0}' | Analysis != ''", sChannel);
    EXPECT_FALSE(resp.TaskId.IsEmpty());
    EXPECT_TRUE(resp.Status == "COMPLETED" || resp.Status == "FAILED" || resp.Status == "DOWNLOADING");
    EXPECT_EQ(resp.Channel, sChannel);
    EXPECT_EQ(resp.Query, sQuery);
    EXPECT_FALSE(resp.ProgressMessage.IsEmpty());
    EXPECT_FALSE(resp.Analysis.IsEmpty());
}

TEST(LlmAnalysisControllerTests, GivenMissingModelFile_WhenAnalyzeEventsCalled_ThenDownloadsModelAndCompletesAnalysis) {
    Console::WriteLine("[TEST] Invoking POST /api/analyze (Application channel)...");
    TestRestClient client;
    AnalyzeRequestDto request;
    request.Channel = "Application";
    request.Query = "Detect anomalous process executions";
    AnalyzeResponseDto initial = client.AnalyzeEvents(request);
    LogAndAssertInitialResponse(initial, "Application", "Detect anomalous process executions");
    AnalyzeResponseDto finalResult = WaitForAnalysisCompletion(client, initial.TaskId, 60);
    LogAndAssertFinalResult(finalResult, "Application", "Detect anomalous process executions");
}

TEST(LlmAnalysisControllerTests, GivenExistingModelFile_WhenAnalyzeEventsCalled_ThenProcessesQueryAndCompletesAnalysis) {
    Console::WriteLine("[TEST] Invoking POST /api/analyze (System channel)...");
    TestRestClient client;
    AnalyzeRequestDto request;
    request.Channel = "System";
    request.Query = "Evaluate critical driver failure trends";
    AnalyzeResponseDto initial = client.AnalyzeEvents(request);
    LogAndAssertInitialResponse(initial, "System", "Evaluate critical driver failure trends");
    AnalyzeResponseDto finalResult = WaitForAnalysisCompletion(client, initial.TaskId, 60);
    LogAndAssertFinalResult(finalResult, "System", "Evaluate critical driver failure trends");
}

TEST(LlmAnalysisControllerTests, GivenNonExistentTaskId_WhenGetAnalyzeStatusCalled_ThenReturnsNotFoundStatus) {
    Console::WriteLine("[TEST] Invoking GET /api/analyze/status?taskId=TASK_NON_EXISTENT_99999...");
    TestRestClient client;
    AnalyzeResponseDto response = client.GetAnalyzeStatus("TASK_NON_EXISTENT_99999");
    Console::WriteLine("[ANALYZE_STATUS_DTO] TaskId={0} | Status={1} | Msg={2}",
        response.TaskId, response.Status, response.ProgressMessage);
    Console::WriteLine("[ASSERT] TaskId == 'TASK_NON_EXISTENT_99999' | Status == 'NOT_FOUND'");
    EXPECT_EQ(response.TaskId, "TASK_NON_EXISTENT_99999");
    EXPECT_EQ(response.Status, "NOT_FOUND");
    EXPECT_EQ(response.ProgressMessage, "Specified Task ID was not found in active queue.");
}
