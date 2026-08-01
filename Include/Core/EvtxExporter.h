#pragma once

#include "../Common.h"
#include "Core/EventRecord.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"

namespace SmartEventViewer
{
    using String = System::String;

    class SMARTEVENTVIEWER_API EvtxExporter
    {
    public:
        EvtxExporter() = default;

        // Saves valid native binary .evtx file to user-specified path using Windows EvtExportLog API
        static bool ExportChannelToEvtx(const String& sChannelPath, const String& sTargetEvtxFilePath);
    };
}
