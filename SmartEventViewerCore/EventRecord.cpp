#include "pch.h"
#include "../Include/Core/EventRecord.h"
#include "System/Convert.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using Convert = DotNetDupe::System::Convert;

    static EventLevel LevelFromInt(int iLevel) {
        if (iLevel == 1) return EventLevel::Critical;
        if (iLevel == 2) return EventLevel::Error;
        if (iLevel == 3) return EventLevel::Warning;
        if (iLevel == 5) return EventLevel::Verbose;
        return EventLevel::Informational;
    }

    static int ParseXmlLevelValue(const String& sRawXml) {
        int iLvlPos = sRawXml.IndexOf("<Level>");
        if (iLvlPos < 0) return 0;

        int iEndLvl = sRawXml.IndexOf("</Level>", iLvlPos);
        if (iEndLvl < 0) return 0;

        String sLvlStr = sRawXml.Substring(iLvlPos + 7, iEndLvl - (iLvlPos + 7));
        try {
            return Convert::ToInt32(sLvlStr);
        } catch (const DotNetDupe::System::Exception&) {
            return 0;
        }
    }

    static EventLevel ParseEtwLevel(int iLevel, const String& sRawXml) {
        int actualLevel = iLevel;
        if (actualLevel == 0 && !sRawXml.IsEmpty()) {
            actualLevel = ParseXmlLevelValue(sRawXml);
        }
        return LevelFromInt(actualLevel);
    }

    static String GetEventIdDescription(int iEventId) {
        if (iEventId == 4688) return " (New Process Creation)";
        if (iEventId == 4689) return " (Process Termination)";
        if (iEventId == 4624) return " (Successful User Account Logon)";
        if (iEventId == 4625) return " (Failed User Account Logon Attempt)";
        if (iEventId == 1102) return " (Security Audit Log Cleared)";
        if (iEventId == 7045) return " (New Windows Service Installed)";
        return "";
    }

    static String FormatFallbackMessage(int iEventId, const String& sProvider, const String& sMsg) {
        if (!sMsg.IsEmpty() && !sMsg.StartsWith("<Event") && !sMsg.StartsWith("<?xml")) {
            return sMsg;
        }
        String sDesc = GetEventIdDescription(iEventId);
        return String::Format("Event ID #{0} logged by provider '{1}'{2}", iEventId, sProvider, sDesc);
    }

    EventRecord EventRecord::FromEtwEvent(const DotNetDupe::System::Diagnostics::EtwEvent& etwEvt, const String& sDefaultChannel) {
        EventLevel level = ParseEtwLevel(etwEvt.iLevel, etwEvt.sRawXml);
        String sMsg = FormatFallbackMessage(etwEvt.iEventId, etwEvt.sProviderName, etwEvt.sMessage);
        String sChannel = etwEvt.sChannelName.IsEmpty() ? sDefaultChannel : etwEvt.sChannelName;
        String sTimeStr = etwEvt.dtTimeCreated.ToString();

        return EventRecord(static_cast<unsigned int>(etwEvt.iEventId), level, etwEvt.sProviderName, sChannel, sMsg, sTimeStr, etwEvt.sRawXml);
    }

    EventRecord::EventRecord(unsigned int uEventId, EventLevel eLevel, const String& sProvider, const String& sChannel, const String& sMessage, const String& sTimeCreated, const String& sRawXml)
        : m_uEventId(uEventId), m_eLevel(eLevel), m_sProviderName(sProvider), m_sChannel(sChannel), m_sMessage(sMessage), m_sTimeCreated(sTimeCreated), m_sRawXml(sRawXml) {
    }

    unsigned int EventRecord::GetEventId() const {
        return m_uEventId;
    }

    EventLevel EventRecord::GetLevel() const {
        return m_eLevel;
    }

    RiskLevel EventRecord::GetRiskLevel() const {
        return m_eRiskLevel;
    }

    void EventRecord::SetRiskLevel(RiskLevel eRiskLevel) {
        m_eRiskLevel = eRiskLevel;
    }

    String EventRecord::GetProviderName() const {
        return m_sProviderName;
    }

    String EventRecord::GetChannel() const {
        return m_sChannel;
    }

    String EventRecord::GetEventMessage() const {
        return m_sMessage;
    }

    String EventRecord::GetTimeCreated() const {
        return m_sTimeCreated;
    }

    String EventRecord::GetRawXml() const {
        return m_sRawXml;
    }
}
