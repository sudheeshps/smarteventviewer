#include "pch.h"
#include "../Include/Platform/WinEventLogReader.h"
#include "System/Console.h"
#include "System/Diagnostics/EtwLogReader.h"
#include "System/Threading/Mutex.h"
#include "System/Threading/Lock.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace SmartEventViewer
{
    using Console = DotNetDupe::System::Console;
    using EtwLogReader = DotNetDupe::System::Diagnostics::EtwLogReader;
    using EtwEvent = DotNetDupe::System::Diagnostics::EtwEvent;
    using Mutex = DotNetDupe::System::Threading::Mutex;
    using MutexLock = DotNetDupe::System::Threading::MutexLock;

    static String FormatGlobalMutexName(const String& sChannelName)
    {
        String sCleanName = sChannelName;
        sCleanName.Replace("/", "_");
        sCleanName.Replace("\\", "_");
        return String("Global\\SmartEventViewer_Channel_") + sCleanName;
    }

    WinEventLogReader::WinEventLogReader() = default;

    WinEventLogReader::~WinEventLogReader()
    {
        Close();
    }

    bool WinEventLogReader::GetEventSources(StringList& outSources)
    {
        outSources.Clear();
        auto channels = EtwLogReader::GetEventChannels();
        for (int i = 0; i < channels.GetCount(); ++i)
        {
            outSources.Add(channels[i]);
        }
        return (outSources.GetCount() > 0);
    }

    bool WinEventLogReader::OpenLog(const String& sChannelName)
    {
        return OpenLogPaged(sChannelName, -1, 0);
    }

    bool WinEventLogReader::OpenLogPaged(const String& sChannelName, int iMaxEvents, int iStartIndex)
    {
        m_sChannel = sChannelName;
        m_cachedEvents.clear();
        m_readIndex = 0;

        Mutex channelMutex(FormatGlobalMutexName(sChannelName));
        MutexLock lock(channelMutex);

        auto eventsList = EtwLogReader::ReadEvents(sChannelName, iMaxEvents, iStartIndex, true);
        for (int i = 0; i < eventsList.GetCount(); ++i)
        {
            m_cachedEvents.push_back(eventsList[i]);
        }
        return true;
    }

    unsigned long long WinEventLogReader::GetChannelEventCount(const String& sChannelName)
    {
        return EtwLogReader::GetChannelEventCount(sChannelName);
    }

    bool WinEventLogReader::ReadNextEvent(EventRecord& outEvent)
    {
        if (m_readIndex >= m_cachedEvents.size())
        {
            return false;
        }

        const auto& etwEvt = m_cachedEvents[m_readIndex++];

        EventLevel level = EventLevel::Informational;
        if (etwEvt.iLevel == 1) level = EventLevel::Critical;
        else if (etwEvt.iLevel == 2) level = EventLevel::Error;
        else if (etwEvt.iLevel == 3) level = EventLevel::Warning;
        else if (etwEvt.iLevel == 4) level = EventLevel::Informational;
        else if (etwEvt.iLevel == 5) level = EventLevel::Verbose;

        String sMsg = etwEvt.sMessage;
        std::string rawMsg = sMsg.GetRawString();
        if (sMsg.IsEmpty() || rawMsg.rfind("<Event", 0) == 0 || rawMsg.rfind("<?xml", 0) == 0)
        {
            std::string sSummary = "Event ID #" + std::to_string(etwEvt.iEventId) + " logged by provider '" + etwEvt.sProviderName.GetRawString() + "'";
            if (etwEvt.iEventId == 4688) sSummary += " (New Process Creation)";
            else if (etwEvt.iEventId == 4689) sSummary += " (Process Termination)";
            else if (etwEvt.iEventId == 4624) sSummary += " (Successful User Account Logon)";
            else if (etwEvt.iEventId == 4625) sSummary += " (Failed User Account Logon Attempt)";
            else if (etwEvt.iEventId == 1102) sSummary += " (Security Audit Log Cleared)";
            else if (etwEvt.iEventId == 7045) sSummary += " (New Windows Service Installed)";
            sMsg = String(sSummary.c_str());
        }

        String sTimeStr = etwEvt.dtTimeCreated.ToString();

        outEvent = EventRecord(
            static_cast<unsigned int>(etwEvt.iEventId),
            level,
            etwEvt.sProviderName,
            m_sChannel,
            sMsg,
            sTimeStr,
            etwEvt.sRawXml
        );

        return true;
    }

    void WinEventLogReader::Close()
    {
        m_cachedEvents.clear();
        m_readIndex = 0;
    }
}
