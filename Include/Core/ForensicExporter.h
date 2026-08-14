#pragma once

#include "../Common.h"
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"

namespace SmartEventViewer {
    using String = System::String;
    using EventRecordList = System::Collections::Generic::List<EventRecord>;

    class SMARTEVENTVIEWER_API ForensicExporter {
    public:
        ForensicExporter() = default;

        static String ExportToForensicPackageJson(const EventRecord* pEvents, unsigned int uCount, const String& sInvestigatorNotes);
        static String GenerateDigitalSignatureSha256(const String& sData);
    };
}
