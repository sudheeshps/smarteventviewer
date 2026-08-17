#pragma once

#include "ViewerCommon.h"
#include "Core/IEventLogReader.h"

namespace SmartEventViewer {
    class SMARTEVENTVIEWER_API WindowsEtwLogReader : public IEventLogReader {
    public:
        WindowsEtwLogReader() = default;
        ~WindowsEtwLogReader() override = default;

        StringList GetEventChannels() override;
        unsigned long long GetChannelEventCount(const String& sChannelName) override;
        DotNetDupe::System::Collections::Generic::List<EventRecord> ReadEvents(
            const String& sChannelName,
            size_t nMaxCount,
            size_t nStartIndex = 0,
            bool bReverseOrder = true) override;
    };
}
