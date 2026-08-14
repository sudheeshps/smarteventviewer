#include "pch.h"
#include "../Include/Core/EventRecord.h"
namespace SmartEventViewer {
    using String = DotNetDupe::System::String;

    static EventLevel ParseEtwLevel(int iLevel, const String& sRawXml) {
        int actualLevel = iLevel;
        std::string rawXml = sRawXml.GetRawString();
        if (actualLevel == 0 && !rawXml.empty()) {
            size_t lvlPos = rawXml.find("<Level>");
            if (lvlPos != std::string::npos) {
                size_t endLvl = rawXml.find("</Level>", lvlPos);
                if (endLvl != std::string::npos) {
                    std::string lvlStr = rawXml.substr(lvlPos + 7, endLvl - (lvlPos + 7));
                    try { actualLevel = std::stoi(lvlStr); } catch(...) {}
                }
            }
        }
        if (actualLevel == 1) return EventLevel::Critical;
        if (actualLevel == 2) return EventLevel::Error;
        if (actualLevel == 3) return EventLevel::Warning;
        if (actualLevel == 5) return EventLevel::Verbose;
        return EventLevel::Informational;
    }

    static String FormatFallbackMessage(int iEventId, const String& sProvider, const String& sMsg) {
        std::string rawMsg = sMsg.GetRawString();
        if (!sMsg.IsEmpty() && rawMsg.rfind("<Event", 0) != 0 && rawMsg.rfind("<?xml", 0) != 0) {
            return sMsg;
        }
        std::string sSummary = "Event ID #" + std::to_string(iEventId) + " logged by provider '" + sProvider.GetRawString() + "'";
        if (iEventId == 4688) sSummary += " (New Process Creation)";
        else if (iEventId == 4689) sSummary += " (Process Termination)";
        else if (iEventId == 4624) sSummary += " (Successful User Account Logon)";
        else if (iEventId == 4625) sSummary += " (Failed User Account Logon Attempt)";
        else if (iEventId == 1102) sSummary += " (Security Audit Log Cleared)";
        else if (iEventId == 7045) sSummary += " (New Windows Service Installed)";
        return String(sSummary.c_str());
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
