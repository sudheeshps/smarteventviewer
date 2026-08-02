#pragma once

#include "Core/IEventLogReader.h"

#include <vector>
#include "System/Diagnostics/EtwLogReader.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;

    class WinEventLogReader : public IEventLogReader
    {
    private:
        String m_sChannel{};
        std::vector<DotNetDupe::System::Diagnostics::EtwEvent> m_cachedEvents{};
        size_t m_readIndex{ 0 };

    public:
        SMARTEVENTVIEWER_API WinEventLogReader();
        SMARTEVENTVIEWER_API ~WinEventLogReader() override;

        SMARTEVENTVIEWER_API bool GetEventSources(StringList& outSources) override;
        SMARTEVENTVIEWER_API bool OpenLog(const String& sChannelName) override;
        SMARTEVENTVIEWER_API bool OpenLogPaged(const String& sChannelName, int iMaxEvents, int iStartIndex);
        SMARTEVENTVIEWER_API unsigned long long GetChannelEventCount(const String& sChannelName) override;
        SMARTEVENTVIEWER_API bool ReadNextEvent(EventRecord& outEvent) override;
        SMARTEVENTVIEWER_API void Close() override;
    };
}
