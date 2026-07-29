#pragma once

#include "Common.h"
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "DotNetDupe/String.h"
#include "DotNetDupe/List.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using EventRecordList = DotNetDupe::System::Collections::Generic::List<EventRecord>;

    class SMARTEVENTVIEWER_API ForensicExporter
    {
    public:
        ForensicExporter() = default;

        static String ExportToForensicPackageJson(const EventRecord* pEvents, unsigned int uCount, const String& sInvestigatorNotes);
        static String GenerateDigitalSignatureSha256(const String& sData);
    };
}
