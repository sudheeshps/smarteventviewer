#include "pch.h"
#include "../Include/Core/EventRecord.h"

namespace SmartEventViewer
{
    EventRecord::EventRecord(unsigned int uEventId, EventLevel eLevel, const String& sProvider, const String& sChannel, const String& sMessage, const String& sTimeCreated)
        : m_uEventId(uEventId), m_eLevel(eLevel), m_sProviderName(sProvider), m_sChannel(sChannel), m_sMessage(sMessage), m_sTimeCreated(sTimeCreated)
    {
    }

    unsigned int EventRecord::GetEventId() const
    {
        return m_uEventId;
    }

    EventLevel EventRecord::GetLevel() const
    {
        return m_eLevel;
    }

    RiskLevel EventRecord::GetRiskLevel() const
    {
        return m_eRiskLevel;
    }

    void EventRecord::SetRiskLevel(RiskLevel eRiskLevel)
    {
        m_eRiskLevel = eRiskLevel;
    }

    String EventRecord::GetProviderName() const
    {
        return m_sProviderName;
    }

    String EventRecord::GetChannel() const
    {
        return m_sChannel;
    }

    String EventRecord::GetMessage() const
    {
        return m_sMessage;
    }

    String EventRecord::GetTimeCreated() const
    {
        return m_sTimeCreated;
    }
}
