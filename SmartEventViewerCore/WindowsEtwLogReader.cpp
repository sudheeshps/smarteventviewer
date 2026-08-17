#include "pch.h"
#include "Platform/WindowsEtwLogReader.h"
#include "System/Diagnostics/EtwLogReader.h"

namespace SmartEventViewer {
    StringList WindowsEtwLogReader::GetEventChannels() {
        return DotNetDupe::System::Diagnostics::EtwLogReader::GetEventChannels();
    }

    unsigned long long WindowsEtwLogReader::GetChannelEventCount(const String& sChannelName) {
        return DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventCount(sChannelName);
    }

    DotNetDupe::System::Collections::Generic::List<EventRecord> WindowsEtwLogReader::ReadEvents(
        const String& sChannelName,
        size_t nMaxCount,
        size_t nStartIndex,
        bool bReverseOrder) {
        DotNetDupe::System::Collections::Generic::List<EventRecord> results;
        auto raw = DotNetDupe::System::Diagnostics::EtwLogReader::ReadEvents(
            sChannelName, static_cast<int>(nMaxCount), static_cast<int>(nStartIndex), bReverseOrder);
        for (int i = 0; i < raw.GetCount(); ++i) {
            results.Add(EventRecord::FromEtwEvent(raw[i], sChannelName));
        }
        return results;
    }
}
