#include <gtest/gtest.h>
#include "TestRestClient.h"
#include "System/Console.h"

using namespace DotNetDupe::System;
using namespace SmartEventViewer;
using namespace SmartEventViewer::IntegrationTests;

static void LogAndAssertLogColumn(const LogColumnFormatDto& col) {
    Console::WriteLine("  [COLUMN_DTO] Key={0} | Header={1} | Type={2} | Width={3}px",
        col.Key, col.HeaderName, col.Type, col.WidthPx);
    EXPECT_FALSE(col.Key.IsEmpty());
    EXPECT_FALSE(col.HeaderName.IsEmpty());
    EXPECT_FALSE(col.Type.IsEmpty());
    EXPECT_GT(col.WidthPx, 0);
}

static void LogAndAssertLogRecord(const LogRecordDto& rec) {
    Console::WriteLine("  [LOG_REC_DTO] Category={0} | PID={1} | TID={2} | Msg={3}",
        rec.Category, rec.ProcessId, rec.ThreadId, rec.Message);
    EXPECT_FALSE(rec.Category.IsEmpty());
    EXPECT_FALSE(rec.Message.IsEmpty());
    EXPECT_GE(rec.ProcessId, 0);
    EXPECT_GE(rec.ThreadId, 0);
}

TEST(DiagnosticsControllerTests, GivenRunningServer_WhenGetLogFormatCalled_ThenReturnsLogSchemaColumns) {
    Console::WriteLine("[TEST] Invoking GET /api/logs/format...");
    TestRestClient client;
    LogFormatResponseDto response = client.GetLogFormat();
    Console::WriteLine("[LOG_FORMAT_DTO] TotalColumns={0}", response.Columns.GetCount());
    Console::WriteLine("[ASSERT] Columns.Count >= 6 && Columns[0].Key == 'timestamp'");
    EXPECT_GE(response.Columns.GetCount(), 6);
    EXPECT_EQ(response.Columns[0].Key, "timestamp");
    EXPECT_EQ(response.Columns[0].HeaderName, "Timestamp");
    for (int i = 0; i < response.Columns.GetCount(); ++i) {
        LogAndAssertLogColumn(response.Columns[i]);
    }
}

TEST(DiagnosticsControllerTests, GivenRunningServer_WhenGetServerLogsCalled_ThenReturnsServerLogEntries) {
    Console::WriteLine("[TEST] Invoking GET /api/logs...");
    TestRestClient client;
    ServerLogsResponseDto response = client.GetServerLogs();
    Console::WriteLine("[SERVER_LOGS_DTO] RecordsCount={0}", response.Records.GetCount());
    Console::WriteLine("[ASSERT] Records.Count >= 0");
    EXPECT_GE(response.Records.GetCount(), 0);
    for (int i = 0; i < response.Records.GetCount() && i < 10; ++i) {
        LogAndAssertLogRecord(response.Records[i]);
    }
}
