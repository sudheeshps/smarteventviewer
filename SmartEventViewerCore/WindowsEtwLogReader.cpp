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

    bool WindowsEtwLogReader::GetChannelLevelCounts(const String& sChannelName, EventLevelCounts& outCounts) {
        try {
            auto rawCounts = DotNetDupe::System::Diagnostics::EtwLogReader::GetChannelEventLevelCounts(sChannelName);
            outCounts.CriticalCount = rawCounts.uCriticalCount;
            outCounts.ErrorCount = rawCounts.uErrorCount;
            outCounts.WarningCount = rawCounts.uWarningCount;
            outCounts.InfoCount = rawCounts.uInfoCount;
            outCounts.VerboseCount = rawCounts.uVerboseCount;
            return true;
        } catch (const DotNetDupe::System::Exception&) {
            return false;
        }
    }

    static DotNetDupe::System::Diagnostics::EtwEventLevel ToEtwLevel(EventLevel lvl) {
        if (lvl == EventLevel::Critical) return DotNetDupe::System::Diagnostics::EtwEventLevel::Critical;
        if (lvl == EventLevel::Error) return DotNetDupe::System::Diagnostics::EtwEventLevel::Error;
        if (lvl == EventLevel::Warning) return DotNetDupe::System::Diagnostics::EtwEventLevel::Warning;
        if (lvl == EventLevel::Informational) return DotNetDupe::System::Diagnostics::EtwEventLevel::Info;
        if (lvl == EventLevel::Verbose) return DotNetDupe::System::Diagnostics::EtwEventLevel::Verbose;
        return DotNetDupe::System::Diagnostics::EtwEventLevel::All;
    }

    DotNetDupe::System::Collections::Generic::List<EventRecord> WindowsEtwLogReader::ReadEvents(
        const String& sChannelName,
        size_t nMaxCount,
        size_t nStartIndex,
        bool bReverseOrder,
        EventLevel eLevel) {
        DotNetDupe::System::Collections::Generic::List<EventRecord> results;
        auto raw = DotNetDupe::System::Diagnostics::EtwLogReader::ReadEvents(
            sChannelName, static_cast<int>(nMaxCount), static_cast<int>(nStartIndex), bReverseOrder, ToEtwLevel(eLevel));
        for (int i = 0; i < raw.GetCount(); ++i) {
            results.Add(EventRecord::FromEtwEvent(raw[i], sChannelName));
        }
        return results;
    }
}
