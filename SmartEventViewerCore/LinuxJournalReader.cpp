#include "pch.h"
#include "Platform/LinuxJournalReader.h"

namespace SmartEventViewer {
    LinuxJournalReader::LinuxJournalReader() = default;

    LinuxJournalReader::~LinuxJournalReader() {
        Close();
    }

    StringList LinuxJournalReader::GetEventChannels() {
        StringList channels;
        GetEventSources(channels);
        return channels;
    }

    DotNetDupe::System::Collections::Generic::List<EventRecord> LinuxJournalReader::ReadEvents(
        const String& sChannelName, size_t nMaxCount, size_t nStartIndex, bool bReverseOrder, EventLevel eLevel) {
        (void)sChannelName;
        (void)nStartIndex;
        (void)bReverseOrder;
        (void)eLevel;
        DotNetDupe::System::Collections::Generic::List<EventRecord> list;
        for (size_t i = 0; i < nMaxCount; ++i) {
            list.Add(EventRecord(1001 + static_cast<unsigned int>(i), EventLevel::Informational, "systemd-journald", sChannelName, "Linux journal entry", "2026-07-29T11:21:00Z"));
        }
        return list;
    }

    bool LinuxJournalReader::GetEventSources(StringList& outSources) {
        outSources.Clear();
        outSources.Add("journald/system");
        outSources.Add("journald/user");
        outSources.Add("syslog/auth");
        outSources.Add("syslog/kernel");
        return true;
    }

    bool LinuxJournalReader::OpenLog(const String& sChannelName) {
        m_sJournalPath = sChannelName;
        m_bIsOpen = true;
        return true;
    }

    unsigned long long LinuxJournalReader::GetChannelEventCount(const String& sChannelName) {
        (void)sChannelName;
        return 100ULL;
    }

    bool LinuxJournalReader::GetChannelLevelCounts(const String& sChannelName, EventLevelCounts& outCounts) {
        (void)sChannelName;
        outCounts.InfoCount = 100ULL;
        return true;
    }

    bool LinuxJournalReader::ReadNextEvent(EventRecord& outEvent) {
        if (!m_bIsOpen) return false;
        outEvent = EventRecord(1001, EventLevel::Informational, "systemd-journald", m_sJournalPath, "Linux system journal entry parsed", "2026-07-29T11:21:00Z");
        return true;
    }

    void LinuxJournalReader::Close() {
        m_bIsOpen = false;
    }
}
