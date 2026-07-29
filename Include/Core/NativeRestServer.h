#pragma once

#include "Common.h"
#include "DotNetDupe/String.h"
#include "DotNetDupe/List.h"
#include "Platform/WinEventLogReader.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    struct EventDto
    {
        size_t Index{ 0 };
        unsigned int Id{ 0 };
        String Level{};
        String Risk{};
        String Provider{};
        String Time{};
        String Message{};

        EventDto() = default;
        EventDto(const EventDto&) = default;
        EventDto& operator=(const EventDto&) = default;
    };

    struct EventLogResponseDto
    {
        String Channel{};
        unsigned long long TotalCount{ 0 };
        DotNetDupe::System::Collections::Generic::List<EventDto> Events{};
    };

    class SMARTEVENTVIEWER_API EventLogController
    {
    private:
        WinEventLogReader m_logReader{};

    public:
        EventLogController() = default;

        String GetChannels();
        String GetEvents(const String& sChannelName);
    };

    class WebApplication;

    class SMARTEVENTVIEWER_API WebApplicationBuilder
    {
    public:
        WebApplicationBuilder() = default;
        WebApplicationBuilder& UseUrls(const String& sUrl);
        WebApplicationBuilder& UseStaticFiles(const String& sWebRoot = String("UI"));
        WebApplication Build();
    };

    class SMARTEVENTVIEWER_API WebApplication
    {
    private:
        unsigned short m_uPort{ 8080 };
        bool m_bIsRunning{ false };
        EventLogController m_controller{};

    public:
        WebApplication() = default;
        ~WebApplication();

        static WebApplicationBuilder CreateBuilder(int argc = 0, char* argv[] = nullptr);

        bool Run(unsigned short uPort = 8080);
        void Stop();
        bool IsRunning() const { return m_bIsRunning; }
        unsigned short GetPort() const { return m_uPort; }

        EventLogController& GetController() { return m_controller; }
    };
}
