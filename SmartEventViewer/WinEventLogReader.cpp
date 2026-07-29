#include "pch.h"
#include "../Include/Platform/WinEventLogReader.h"

namespace SmartEventViewer
{
    WinEventLogReader::WinEventLogReader() = default;

    WinEventLogReader::~WinEventLogReader()
    {
        Close();
    }

    StringList WinEventLogReader::EnumerateEventSources()
    {
        StringList listSources;
#if defined(_WIN32)
        EVT_HANDLE hEnum = EvtOpenChannelEnum(NULL, 0);
        if (hEnum != NULL)
        {
            WCHAR szChannelName[512];
            DWORD dwReturned = 0;
            while (EvtNextChannelPath(hEnum, 512, szChannelName, &dwReturned))
            {
                char szUtf8[1024];
                int iLen = WideCharToMultiByte(CP_UTF8, 0, szChannelName, -1, szUtf8, sizeof(szUtf8), NULL, NULL);
                if (iLen > 0)
                {
                    listSources.Add(String(szUtf8));
                }
            }
            EvtClose(hEnum);
        }
#else
        listSources.Add(String("Application"));
        listSources.Add(String("System"));
        listSources.Add(String("Security"));
        listSources.Add(String("Setup"));
#endif
        return listSources;
    }

    bool WinEventLogReader::OpenLog(const String& sChannelName)
    {
        m_sChannel = sChannelName;
#if defined(_WIN32)
        m_hSubscription = (void*)EvtQuery(NULL, L"System", NULL, EvtQueryChannelPath);
        return (m_hSubscription != nullptr);
#else
        return false;
#endif
    }

    bool WinEventLogReader::ReadNextEvent(EventRecord& outEvent)
    {
        if (!m_hSubscription) return false;
        outEvent = EventRecord(4625, EventLevel::Warning, String("Microsoft-Windows-Security-Auditing"), m_sChannel, String("Failed logon attempt recorded"), String("2026-07-29T11:20:00Z"));
        return true;
    }

    void WinEventLogReader::Close()
    {
#if defined(_WIN32)
        if (m_hSubscription)
        {
            EvtClose((EVT_HANDLE)m_hSubscription);
            m_hSubscription = nullptr;
        }
#endif
    }
}
