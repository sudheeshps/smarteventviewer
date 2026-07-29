#pragma once

#include "Common.h"
#include "DotNetDupe/String.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;

    class SMARTEVENTVIEWER_API EventRecord
    {
    private:
        unsigned int m_uEventId{ 0 };
        EventLevel m_eLevel{ EventLevel::Informational };
        RiskLevel m_eRiskLevel{ RiskLevel::Low };
        String m_sProviderName{};
        String m_sChannel{};
        String m_sMessage{};
        String m_sTimeCreated{};

    public:
        EventRecord() = default;
        EventRecord(unsigned int uEventId, EventLevel eLevel, const String& sProvider, const String& sChannel, const String& sMessage, const String& sTimeCreated);

        unsigned int GetEventId() const;
        EventLevel GetLevel() const;
        RiskLevel GetRiskLevel() const;
        void SetRiskLevel(RiskLevel eRiskLevel);
        String GetProviderName() const;
        String GetChannel() const;
        String GetMessage() const;
        String GetTimeCreated() const;
    };
}
