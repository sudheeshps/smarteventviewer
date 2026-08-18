#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/DateTime.h"
#include "System/Diagnostics/EtwLogReader.h"

namespace SmartEventViewer {
    class EventRecord {
    private:
        unsigned int m_uEventId{ 0 };
        EventLevel m_eLevel{ EventLevel::Informational };
        RiskLevel m_eRiskLevel{ RiskLevel::Low };
        DotNetDupe::System::String m_sProviderName{};
        DotNetDupe::System::String m_sChannel{};
        DotNetDupe::System::String m_sMessage{};
        DotNetDupe::System::DateTime m_dtTimeCreated{};
        DotNetDupe::System::String m_sTimeCreated{};
        DotNetDupe::System::String m_sRawXml{};

    public:
        SMARTEVENTVIEWER_API EventRecord() = default;
        SMARTEVENTVIEWER_API EventRecord(const EventRecord& evt) = default;
        SMARTEVENTVIEWER_API EventRecord(unsigned int uEventId, EventLevel eLevel, const DotNetDupe::System::String& sProvider, const DotNetDupe::System::String& sChannel, const DotNetDupe::System::String& sMessage, DotNetDupe::System::DateTime dtTimeCreated, const DotNetDupe::System::String& sRawXml = DotNetDupe::System::String());
        SMARTEVENTVIEWER_API EventRecord(unsigned int uEventId, EventLevel eLevel, const DotNetDupe::System::String& sProvider, const DotNetDupe::System::String& sChannel, const DotNetDupe::System::String& sMessage, const DotNetDupe::System::String& sTimeCreated, const DotNetDupe::System::String& sRawXml = DotNetDupe::System::String());

        SMARTEVENTVIEWER_API static EventRecord FromEtwEvent(const DotNetDupe::System::Diagnostics::EtwEvent& etwEvt, const DotNetDupe::System::String& sDefaultChannel = DotNetDupe::System::String());

        SMARTEVENTVIEWER_API unsigned int GetEventId() const;
        SMARTEVENTVIEWER_API EventLevel GetLevel() const;
        SMARTEVENTVIEWER_API RiskLevel GetRiskLevel() const;
        SMARTEVENTVIEWER_API void SetRiskLevel(RiskLevel eRiskLevel);
        SMARTEVENTVIEWER_API DotNetDupe::System::String GetProviderName() const;
        SMARTEVENTVIEWER_API DotNetDupe::System::String GetChannel() const;
        SMARTEVENTVIEWER_API DotNetDupe::System::String GetEventMessage() const;
        SMARTEVENTVIEWER_API DotNetDupe::System::DateTime GetDateTime() const;
        SMARTEVENTVIEWER_API DotNetDupe::System::String GetTimeCreated() const;
        SMARTEVENTVIEWER_API DotNetDupe::System::String GetRawXml() const;
    };
}
