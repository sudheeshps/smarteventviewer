#include <gtest/gtest.h>
#include "Controllers/LlmAnalysisController.h"
#include "Core/AnalysisService.h"
#include "Core/EventService.h"
#include "Mocks/MockLlamaModelProvider.h"
#include "System/Console.h"
#include "System/Threading/Thread.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::Threading;
using namespace SmartEventViewer;

template<typename T>
using SmartPtr = DotNetDupe::System::SmartPointer<T>;

static AnalyzeResponseDto WaitForAnalysisCompletion(LlmAnalysisController& controller, const String& sTaskId, int nTimeoutSec = 5) {
    AnalyzeResponseDto status;
    int nAttempts = nTimeoutSec * 20;
    for (int i = 0; i < nAttempts; ++i) {
        status = controller.GetAnalyzeStatus(sTaskId);
        if (status.Status == "COMPLETED" || status.Status == "FAILED") break;
        Thread::Sleep(50);
    }
    return status;
}

static void LogAndAssertInitialResponse(const AnalyzeResponseDto& resp, const String& sChannel, const String& sQuery) {
    EXPECT_FALSE(resp.TaskId.IsEmpty());
    EXPECT_TRUE(resp.Status == "DOWNLOADING" || resp.Status == "PENDING" || resp.Status == "RUNNING" || resp.Status == "COMPLETED");
    EXPECT_EQ(resp.Channel, sChannel);
    EXPECT_EQ(resp.Query, sQuery);
    EXPECT_FALSE(resp.ProgressMessage.IsEmpty());
}

static void LogAndAssertFinalResult(const AnalyzeResponseDto& resp, const String& sChannel, const String& sQuery) {
    EXPECT_FALSE(resp.TaskId.IsEmpty());
    EXPECT_EQ(resp.Status, "COMPLETED");
    EXPECT_EQ(resp.Channel, sChannel);
    EXPECT_EQ(resp.Query, sQuery);
    EXPECT_FALSE(resp.ProgressMessage.IsEmpty());
    EXPECT_FALSE(resp.Analysis.IsEmpty());
}

TEST(LlmAnalysisControllerTests, GivenMockProvider_WhenAnalyzeEventsCalled_ThenCompletesAnalysisAndReturnsThreatDossier) {
    auto spMockProvider = SmartPtr<ILlamaModelProvider>(SmartPtr<MockLlamaModelProvider>::NewShared());
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared(spMockProvider);
    auto spEvents = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared());
    auto spAnalysis = SmartPtr<IAnalysisService>(SmartPtr<AnalysisService>::NewShared(spLlm, spEvents));
    LlmAnalysisController controller(spAnalysis);

    AnalyzeRequestDto request;
    request.Channel = "Application";
    request.Query = "Detect anomalous process executions";
    AnalyzeResponseDto initial = controller.AnalyzeEvents(request);
    LogAndAssertInitialResponse(initial, "Application", "Detect anomalous process executions");
    AnalyzeResponseDto finalResult = WaitForAnalysisCompletion(controller, initial.TaskId, 5);
    LogAndAssertFinalResult(finalResult, "Application", "Detect anomalous process executions");
}

TEST(LlmAnalysisControllerTests, GivenSystemChannelQuery_WhenAnalyzeEventsCalled_ThenProcessesQueryAndCompletesAnalysis) {
    auto spMockProvider = SmartPtr<ILlamaModelProvider>(SmartPtr<MockLlamaModelProvider>::NewShared());
    auto spLlm = SmartPtr<LocalLlmEngine>::NewShared(spMockProvider);
    auto spEvents = SmartPtr<IEventService>(SmartPtr<EventService>::NewShared());
    auto spAnalysis = SmartPtr<IAnalysisService>(SmartPtr<AnalysisService>::NewShared(spLlm, spEvents));
    LlmAnalysisController controller(spAnalysis);

    AnalyzeRequestDto request;
    request.Channel = "System";
    request.Query = "Evaluate critical driver failure trends";
    AnalyzeResponseDto initial = controller.AnalyzeEvents(request);
    LogAndAssertInitialResponse(initial, "System", "Evaluate critical driver failure trends");
    AnalyzeResponseDto finalResult = WaitForAnalysisCompletion(controller, initial.TaskId, 5);
    LogAndAssertFinalResult(finalResult, "System", "Evaluate critical driver failure trends");
}

TEST(LlmAnalysisControllerTests, GivenNonExistentTaskId_WhenGetAnalyzeStatusCalled_ThenReturnsNotFoundStatus) {
    LlmAnalysisController controller;
    AnalyzeResponseDto response = controller.GetAnalyzeStatus("TASK_NON_EXISTENT_99999");
    EXPECT_EQ(response.TaskId, "TASK_NON_EXISTENT_99999");
    EXPECT_EQ(response.Status, "NOT_FOUND");
    EXPECT_EQ(response.ProgressMessage, "Specified Task ID was not found in active queue.");
}
