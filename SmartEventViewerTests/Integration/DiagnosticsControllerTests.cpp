#include <gtest/gtest.h>
#include "Controllers/DiagnosticsController.h"
#include "System/Console.h"

using namespace DotNetDupe::System;
using namespace SmartEventViewer;

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

TEST(DiagnosticsControllerTests, GivenController_WhenGetLogFormatCalled_ThenReturnsLogSchemaColumns) {
    DiagnosticsController controller;
    LogFormatResponseDto response = controller.GetLogFormat();
    EXPECT_GE(response.Columns.GetCount(), 6);
    EXPECT_EQ(response.Columns[0].Key, "timestamp");
    EXPECT_EQ(response.Columns[0].HeaderName, "Timestamp");
    for (int i = 0; i < response.Columns.GetCount(); ++i) {
        LogAndAssertLogColumn(response.Columns[i]);
    }
}

TEST(DiagnosticsControllerTests, GivenController_WhenGetServerLogsCalled_ThenReturnsServerLogEntries) {
    DiagnosticsController controller;
    ServerLogsResponseDto response = controller.GetServerLogs();
    EXPECT_GE(response.Records.GetCount(), 0);
    for (int i = 0; i < response.Records.GetCount() && i < 10; ++i) {
        LogAndAssertLogRecord(response.Records[i]);
    }
}
