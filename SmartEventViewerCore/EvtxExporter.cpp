#include "pch.h"
#include "../Include/Core/EvtxExporter.h"

#if defined(_WIN32)
#include <windows.h>
#include <winevt.h>
#pragma comment(lib, "wevtapi.lib")
#endif

namespace SmartEventViewer
{
    bool EvtxExporter::ExportChannelToEvtx(const String& sChannelPath, const String& sTargetEvtxFilePath)
    {
#if defined(_WIN32)
        String sChannel = sChannelPath.IsEmpty() ? 
            String("Microsoft-Windows-TerminalServices-LocalSessionManager/Operational") : sChannelPath;
        String sTargetFile = sTargetEvtxFilePath.IsEmpty() ? 
            String("C:\\Users\\Public\\RdpEvents_Valid.evtx") : sTargetEvtxFilePath;

        // Convert DotNetDupe String to WCHAR wide string for Win32 API
        std::wstring wsChannel(sChannel.ToWString());
        std::wstring wsTargetFile(sTargetFile.ToWString());

        LPCWSTR pwszQuery = L"*[System[(EventID=21 or EventID=22 or EventID=24 or EventID=25 or EventID=4624 or EventID=4634)]]";

        // Native Windows EvtExportLog API call using dynamic user-chosen target path
        BOOL bSuccess = EvtExportLog(
            NULL,
            wsChannel.c_str(),
            pwszQuery,
            wsTargetFile.c_str(),
            EvtExportLogChannelPath
        );

        return (bSuccess == TRUE);
#else
        (void)sChannelPath;
        (void)sTargetEvtxFilePath;
        return false;
#endif
    }
}
