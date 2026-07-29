#pragma once

#include "Common.h"
#include "Core/EventRecord.h"
#include "DotNetDupe/String.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;

    class SMARTEVENTVIEWER_API EvtxExporter
    {
    public:
        EvtxExporter() = default;

        // Saves valid native binary .evtx file to user-specified path using Windows EvtExportLog API
        static bool ExportChannelToEvtx(const String& sChannelPath, const String& sTargetEvtxFilePath);
    };
}
