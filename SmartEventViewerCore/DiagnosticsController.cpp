#include "pch.h"
#include "DiagnosticsController.h"
#include "EventsController.h"

namespace SmartEventViewer
{
    ServerLogsResponseDto DiagnosticsController::GetServerLogs()
    {
        ServerLogsResponseDto dto;
        dto.Logs = EventsController::GetServerLogList();
        return dto;
    }
}
