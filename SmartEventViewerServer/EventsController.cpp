#include "EventsController.h"
#include "Core/EventRecord.h"
#include "System/Console.h"
#include "System/Convert.h"
#include "System/String.h"

using Console = DotNetDupe::System::Console;

namespace SmartEventViewer
{
    ChannelsResponseDto EventsController::GetChannels()
    {
        Console::WriteLine("[SERVER] Executing EventsController::GetChannels() -> Enumerating event sources...");
        ChannelsResponseDto dto;
        m_logReader.GetEventSources(dto.Channels);
        Console::WriteLine("[SERVER] Successfully enumerated channels count: {0}", static_cast<unsigned long long>(dto.Channels.GetCount()));
        return dto;
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName, size_t page, size_t pageSize)
    {
        String sTargetChannel = channelName.IsEmpty() ? String("Application") : channelName;
        if (page < 1) page = 1;
        if (pageSize < 1) pageSize = 20;

        Console::WriteLine("[SERVER] Executing EventsController::GetEvents() for channel: {0} (Page: {1}, PageSize: {2})", sTargetChannel, static_cast<unsigned long long>(page), static_cast<unsigned long long>(pageSize));

        unsigned long long uTotalCount = m_logReader.GetChannelEventCount(sTargetChannel);
        Console::WriteLine("[SERVER] Total log records reported by Win32 EvtGetLogInfo: {0}", uTotalCount);

        size_t totalPages = (uTotalCount == 0) ? 0 : static_cast<size_t>((uTotalCount + pageSize - 1) / pageSize);

        EventLogResponseDto responseDto;
        responseDto.Channel = sTargetChannel;
        responseDto.TotalCount = uTotalCount;
        responseDto.Page = page;
        responseDto.PageSize = pageSize;
        responseDto.TotalPages = totalPages;

        if (m_logReader.OpenLog(sTargetChannel))
        {
            EventRecord evt;
            size_t totalRead = 0;
            size_t startIndex = (page - 1) * pageSize;
            size_t endIndex = startIndex + pageSize;

            while (m_logReader.ReadNextEvent(evt))
            {
                if (totalRead >= startIndex && totalRead < endIndex)
                {
                    EventDto dto;
                    dto.Index = totalRead + 1;
                    dto.Id = evt.GetEventId();
                    dto.Level = (evt.GetLevel() == EventLevel::Critical ? "Critical" : (evt.GetLevel() == EventLevel::Error ? "Error" : (evt.GetLevel() == EventLevel::Warning ? "Warning" : "Information")));
                    dto.Risk = (evt.GetRiskLevel() == RiskLevel::Critical ? "Critical" : (evt.GetRiskLevel() == RiskLevel::High ? "High" : (evt.GetRiskLevel() == RiskLevel::Medium ? "Medium" : "Low")));
                    dto.Provider = evt.GetProviderName();
                    dto.Time = evt.GetTimeCreated();
                    dto.Message = evt.GetEventMessage();

                    responseDto.Events.Add(dto);
                }
                totalRead++;
                if (totalRead >= endIndex)
                {
                    break; // Stop querying full events once the page buffer is satisfied
                }
            }
            m_logReader.Close();
            Console::WriteLine("[SERVER] Successfully streamed paged event records to DTO response: {0} (Total Processed: {1})", static_cast<unsigned long long>(responseDto.Events.GetCount()), static_cast<unsigned long long>(totalRead));
        }
        else
        {
            Console::WriteLine("[SERVER] [WARNING] Failed to open channel log: {0}", sTargetChannel);
        }

        return responseDto;
    }

    EventLogResponseDto EventsController::GetEvents(const String& channelName)
    {
        return GetEvents(channelName, 1, 20);
    }
}
