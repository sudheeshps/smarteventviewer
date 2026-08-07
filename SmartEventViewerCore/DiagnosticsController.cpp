#include "pch.h"
#include "Core/DiagnosticsController.h"
#include "Core/EventsController.h"

namespace SmartEventViewer
{
    ServerLogsResponseDto DiagnosticsController::GetServerLogs()
    {
        ServerLogsResponseDto dto;
        dto.Logs = EventsController::GetServerLogList();
        return dto;
    }
}
