#pragma once

#include "Core/IEventLogReader.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;

    class LinuxJournalReader : public IEventLogReader {
    private:
        String m_sJournalPath{};
        bool m_bIsOpen{ false };

    public:
        SMARTEVENTVIEWER_API LinuxJournalReader();
        SMARTEVENTVIEWER_API ~LinuxJournalReader() override;

        SMARTEVENTVIEWER_API StringList GetEventChannels() override;
        SMARTEVENTVIEWER_API unsigned long long GetChannelEventCount(const String& sChannelName) override;
        SMARTEVENTVIEWER_API DotNetDupe::System::Collections::Generic::List<EventRecord> ReadEvents(
            const String& sChannelName,
            size_t nMaxCount,
            size_t nStartIndex = 0,
            bool bReverseOrder = true) override;

        SMARTEVENTVIEWER_API bool GetEventSources(StringList& outSources) override;
        SMARTEVENTVIEWER_API bool OpenLog(const String& sChannelName) override;
        SMARTEVENTVIEWER_API bool ReadNextEvent(EventRecord& outEvent) override;
        SMARTEVENTVIEWER_API void Close() override;
    };
}
