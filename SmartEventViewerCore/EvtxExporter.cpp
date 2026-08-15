#include "pch.h"
#include "../Include/Core/EvtxExporter.h"
#include "System/Array.h"

#if defined(_WIN32)
#include <windows.h>
#include <winevt.h>
#pragma comment(lib, "wevtapi.lib")
#endif

namespace SmartEventViewer {
    static DotNetDupe::System::Array<wchar_t> ToWideCharArray(const String& sInput) {
        if (sInput.IsEmpty()) {
            DotNetDupe::System::Array<wchar_t> emptyArr(1);
            emptyArr[0] = L'\0';
            return emptyArr;
        }
        int iLen = ::MultiByteToWideChar(CP_UTF8, 0, sInput.GetRawString(), -1, NULL, 0);
        if (iLen <= 0) {
            DotNetDupe::System::Array<wchar_t> emptyArr(1);
            emptyArr[0] = L'\0';
            return emptyArr;
        }
        DotNetDupe::System::Array<wchar_t> wideArr(iLen);
        ::MultiByteToWideChar(CP_UTF8, 0, sInput.GetRawString(), -1, wideArr.GetData(), iLen);
        return wideArr;
    }

    bool EvtxExporter::ExportChannelToEvtx(const String& sChannelPath, const String& sTargetEvtxFilePath) {
#if defined(_WIN32)
        String sChannel = sChannelPath.IsEmpty() ? 
            "Microsoft-Windows-TerminalServices-LocalSessionManager/Operational" : sChannelPath;
        String sTargetFile = sTargetEvtxFilePath.IsEmpty() ? 
            "C:\\Users\\Public\\RdpEvents_Valid.evtx" : sTargetEvtxFilePath;

        auto wsChannel = ToWideCharArray(sChannel);
        auto wsTargetFile = ToWideCharArray(sTargetFile);

        LPCWSTR pwszQuery = L"*[System[(EventID=21 or EventID=22 or EventID=24 or EventID=25 or EventID=4624 or EventID=4634)]]";

        BOOL bSuccess = EvtExportLog(
            NULL,
            wsChannel.GetData(),
            pwszQuery,
            wsTargetFile.GetData(),
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
