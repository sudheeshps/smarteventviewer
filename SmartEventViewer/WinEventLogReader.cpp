#include "pch.h"
#include "../Include/Platform/WinEventLogReader.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <winevt.h>
#pragma comment(lib, "wevtapi.lib")

inline std::string WCharToUtf8(const wchar_t* wstr)
{
    if (!wstr || wstr[0] == 0) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return "";
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
    if (strTo.back() == '\0') strTo.pop_back();
    return strTo;
}
#endif

namespace SmartEventViewer
{
    WinEventLogReader::WinEventLogReader() = default;

    WinEventLogReader::~WinEventLogReader()
    {
        Close();
    }

    bool WinEventLogReader::GetEventSources(StringList& outSources)
    {
        outSources.Clear();
#if defined(_WIN32)
        EVT_HANDLE hEnum = EvtOpenChannelEnum(NULL, 0);
        if (hEnum != NULL)
        {
            WCHAR szChannelName[512] = { 0 };
            DWORD dwReturned = 0;
            while (EvtNextChannelPath(hEnum, 512, szChannelName, &dwReturned))
            {
                std::string sUtf8 = WCharToUtf8(szChannelName);
                if (!sUtf8.empty())
                {
                    outSources.Add(String(sUtf8.c_str()));
                }
            }
            EvtClose(hEnum);
            return (outSources.GetCount() > 0);
        }
        
        return true;
#else
        outSources.Add("Application");
        outSources.Add("System");
        outSources.Add("Security");
        outSources.Add("Setup");
        return true;
#endif
    }

    bool WinEventLogReader::OpenLog(const String& sChannelName)
    {
        m_sChannel = sChannelName;
#if defined(_WIN32)
        WCHAR szWChannel[512] = { 0 };
        const char* szUtf8 = sChannelName.GetRawString();
        MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, szWChannel, 512);

        m_hSubscription = (void*)EvtQuery(NULL, szWChannel, NULL, EvtQueryChannelPath | EvtQueryReverseDirection);
        return (m_hSubscription != nullptr);
#else
        return false;
#endif
    }

    unsigned long long WinEventLogReader::GetChannelEventCount(const String& sChannelName)
    {
#if defined(_WIN32)
        WCHAR szWChannel[512] = { 0 };
        const char* szUtf8 = sChannelName.GetRawString();
        MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, szWChannel, 512);

        EVT_HANDLE hLog = EvtOpenLog(NULL, szWChannel, EvtOpenChannelPath);
        if (hLog != NULL)
        {
            EVT_VARIANT varRecordCount;
            DWORD dwBufferUsed = 0;
            if (EvtGetLogInfo(hLog, EvtLogNumberOfLogRecords, sizeof(varRecordCount), &varRecordCount, &dwBufferUsed))
            {
                unsigned long long uCount = varRecordCount.UInt64Val;
                EvtClose(hLog);
                return uCount;
            }
            EvtClose(hLog);
        }
#endif
        return 0ULL;
    }

    bool WinEventLogReader::ReadNextEvent(EventRecord& outEvent)
    {
        if (!m_hSubscription) return false;
#if defined(_WIN32)
        EVT_HANDLE hEvent = NULL;
        DWORD dwReturned = 0;

        if (!EvtNext((EVT_HANDLE)m_hSubscription, 1, &hEvent, INFINITE, 0, &dwReturned) || dwReturned == 0)
        {
            return false;
        }

        // Render Event XML to extract real fields
        DWORD dwBufferUsed = 0;
        DWORD dwPropertyCount = 0;
        EvtRender(NULL, hEvent, EvtRenderEventXml, 0, NULL, &dwBufferUsed, &dwPropertyCount);

        std::vector<WCHAR> xmlBuffer(dwBufferUsed / sizeof(WCHAR) + 2, 0);
        if (EvtRender(NULL, hEvent, EvtRenderEventXml, dwBufferUsed, xmlBuffer.data(), &dwBufferUsed, &dwPropertyCount))
        {
            std::string utf8Xml = WCharToUtf8(xmlBuffer.data());
            const char* szUtf8 = utf8Xml.c_str();
            String sXml(szUtf8);

            unsigned int uEventId = 0;
            String sProvider("System");
            String sTimeCreated("2026-07-29T00:00:00Z");
            EventLevel level = EventLevel::Informational;

            // Extract EventID (handles both <EventID>100</EventID> and <EventID Qualifiers='16384'>100</EventID>)
            const char* pIdStart = strstr(szUtf8, "<EventID");
            if (pIdStart)
            {
                const char* pValueStart = strchr(pIdStart, '>');
                if (pValueStart)
                {
                    uEventId = (unsigned int)::atoi(pValueStart + 1);
                }
            }

            // Extract Provider Name
            const char* pProvStart = strstr(szUtf8, "Name='");
            if (!pProvStart) pProvStart = strstr(szUtf8, "Name=\"");
            if (pProvStart)
            {
                pProvStart += 6;
                const char* pProvEnd = strpbrk(pProvStart, "'\"");
                if (pProvEnd)
                {
                    std::string provStr(pProvStart, pProvEnd - pProvStart);
                    sProvider = String(provStr.c_str());
                }
            }

            // Extract TimeCreated
            const char* pTimeStart = strstr(szUtf8, "SystemTime='");
            if (!pTimeStart) pTimeStart = strstr(szUtf8, "SystemTime=\"");
            if (pTimeStart)
            {
                pTimeStart += 12;
                const char* pTimeEnd = strpbrk(pTimeStart, "'\"");
                if (pTimeEnd)
                {
                    std::string timeStr(pTimeStart, pTimeEnd - pTimeStart);
                    sTimeCreated = String(timeStr.c_str());
                }
            }

            // Extract Level
            const char* pLevelStart = strstr(szUtf8, "<Level>");
            if (pLevelStart)
            {
                int lvlVal = ::atoi(pLevelStart + 7);
                if (lvlVal == 1) level = EventLevel::Critical;
                else if (lvlVal == 2) level = EventLevel::Error;
                else if (lvlVal == 3) level = EventLevel::Warning;
                else if (lvlVal == 4) level = EventLevel::Informational;
                else if (lvlVal == 5) level = EventLevel::Verbose;
            }

            outEvent = EventRecord(uEventId, level, sProvider, m_sChannel, sXml, sTimeCreated);
            EvtClose(hEvent);
            return true;
        }

        EvtClose(hEvent);
        return false;
#else
        return false;
#endif
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
