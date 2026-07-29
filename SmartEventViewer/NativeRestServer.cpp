#include "pch.h"
#include "../Include/Core/NativeRestServer.h"
#include <sstream>

namespace SmartEventViewer
{
    // ---------------------------------------------------------
    // DotNetDupe EventLogController JSON Serialization Handler
    // ---------------------------------------------------------
    String EventLogController::GetChannels()
    {
        StringList channels = m_logReader.EnumerateEventSources();
        std::stringstream ss;
        ss << "{\"channels\":[";
        for (size_t i = 0; i < channels.GetCount(); ++i)
        {
            ss << "\"" << channels[i].CStr() << "\"";
            if (i + 1 < channels.GetCount()) ss << ",";
        }
        ss << "]}";
        return String(ss.str().c_str());
    }

    String EventLogController::GetEvents(const String& sChannelName)
    {
        String sTargetChannel = sChannelName.IsEmpty() ? String("Application") : sChannelName;
        unsigned long long uTotalCount = m_logReader.GetChannelEventCount(sTargetChannel);

        EventLogResponseDto responseDto;
        responseDto.Channel = sTargetChannel;
        responseDto.TotalCount = uTotalCount;

        if (m_logReader.OpenLog(sTargetChannel))
        {
            EventRecord evt;
            size_t count = 0;
            while (m_logReader.ReadNextEvent(evt))
            {
                EventDto dto;
                dto.Index = count + 1;
                dto.Id = evt.GetEventId();
                dto.Level = (evt.GetLevel() == EventLevel::Critical ? "Critical" : (evt.GetLevel() == EventLevel::Error ? "Error" : (evt.GetLevel() == EventLevel::Warning ? "Warning" : "Information")));
                dto.Risk = (evt.GetRiskLevel() == RiskLevel::Critical ? "Critical" : (evt.GetRiskLevel() == RiskLevel::High ? "High" : (evt.GetRiskLevel() == RiskLevel::Medium ? "Medium" : "Low")));
                dto.Provider = evt.GetProviderName();
                dto.Time = evt.GetTimeCreated();
                dto.Message = evt.GetMessage();

                responseDto.Events.Add(dto);
                count++;
            }
            m_logReader.Close();
        }

        std::stringstream ss;
        ss << "{\"channel\":\"" << responseDto.Channel.CStr() << "\",\"totalCount\":" << responseDto.TotalCount << ",\"events\":[";

        for (size_t i = 0; i < responseDto.Events.GetCount(); ++i)
        {
            const EventDto& item = responseDto.Events[i];
            if (i > 0) ss << ",";
            ss << "{\"idx\":" << item.Index
               << ",\"id\":" << item.Id
               << ",\"level\":\"" << item.Level.CStr() << "\""
               << ",\"risk\":\"" << item.Risk.CStr() << "\""
               << ",\"provider\":\"" << item.Provider.CStr() << "\""
               << ",\"time\":\"" << item.Time.CStr() << "\""
               << ",\"desc\":\"" << item.Message.CStr() << "\"}";
        }
        ss << "]}";

        return String(ss.str().c_str());
    }

    // ---------------------------------------------------------
    // DotNetDupe WebApplicationBuilder & WebApplication Engine
    // ---------------------------------------------------------
    WebApplicationBuilder& WebApplicationBuilder::UseUrls(const String& sUrl)
    {
        (void)sUrl;
        return *this;
    }

    WebApplicationBuilder& WebApplicationBuilder::UseStaticFiles(const String& sWebRoot)
    {
        (void)sWebRoot;
        return *this;
    }

    WebApplication WebApplicationBuilder::Build()
    {
        WebApplication app;
        return app;
    }

    WebApplication::~WebApplication()
    {
        Stop();
    }

    WebApplicationBuilder WebApplication::CreateBuilder(int argc, char* argv[])
    {
        (void)argc;
        (void)argv;
        WebApplicationBuilder builder;
        return builder;
    }

    bool WebApplication::Run(unsigned short uPort)
    {
        m_uPort = uPort;
        m_bIsRunning = true;
        return true;
    }

    void WebApplication::Stop()
    {
        m_bIsRunning = false;
    }
}
