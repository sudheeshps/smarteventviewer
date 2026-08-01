#include "pch.h"
#include "../Include/Platform/LinuxJournalReader.h"

namespace SmartEventViewer
{
    LinuxJournalReader::LinuxJournalReader() = default;

    LinuxJournalReader::~LinuxJournalReader()
    {
        Close();
    }

    bool LinuxJournalReader::GetEventSources(StringList& outSources)
    {
        outSources.Clear();
        outSources.Add("journald/system");
        outSources.Add("journald/user");
        outSources.Add("syslog/auth");
        outSources.Add("syslog/kernel");
        return true;
    }

    bool LinuxJournalReader::OpenLog(const String& sChannelName)
    {
        m_sJournalPath = sChannelName;
        m_bIsOpen = true;
        return true;
    }

    unsigned long long LinuxJournalReader::GetChannelEventCount(const String& sChannelName)
    {
        (void)sChannelName;
        return 100ULL;
    }

    bool LinuxJournalReader::ReadNextEvent(EventRecord& outEvent)
    {
        if (!m_bIsOpen) return false;
        outEvent = EventRecord(1001, EventLevel::Informational, String("systemd-journald"), m_sJournalPath, String("Linux system journal entry parsed"), String("2026-07-29T11:21:00Z"));
        return true;
    }

    void LinuxJournalReader::Close()
    {
        m_bIsOpen = false;
    }
}
