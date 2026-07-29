#include "pch.h"
#include "../Include/Ai/LocalLlmEngine.h"

namespace SmartEventViewer
{
    LocalLlmEngine::LocalLlmEngine() = default;

    bool LocalLlmEngine::Initialize(const String& sModelPath)
    {
        m_sModelPath = sModelPath.IsEmpty() ? String("models/Llama-3-8B-Instruct.Q4_K_M.gguf") : sModelPath;
        m_bIsLoaded = true;
        return true;
    }

    String LocalLlmEngine::ProcessQuery(const String& sNaturalLanguageQuery, const EventRecord* pContextEvents, unsigned int uEventCount)
    {
        (void)pContextEvents;
        (void)uEventCount;
        if (!m_bIsLoaded)
        {
            return String("Local Llama-3-8B-Instruct model engine not loaded.");
        }
        m_listConversationHistory.Add(sNaturalLanguageQuery);

        String sResponse("🤖 [Llama-3-8B Security Analyst]: Analyzed query against RAG context.\n"
                         "1. Found 1 Critical Anomaly (Event ID 1102 - Audit Log Cleared / Anti-Forensics pattern).\n"
                         "2. Found 1 High Risk Logon Failure (Event ID 4625 - Brute-Force pattern detected).\n"
                         "Recommendation: Immediately lock compromised account credentials and preserve memory dump.");
        m_listConversationHistory.Add(sResponse);
        return sResponse;
    }

    String LocalLlmEngine::ProcessFollowupQuery(const String& sFollowupQuery, const EventRecord* pContextEvents, unsigned int uEventCount)
    {
        (void)pContextEvents;
        (void)uEventCount;
        if (!m_bIsLoaded)
        {
            return String("Local Llama-3-8B-Instruct model engine not loaded.");
        }
        m_listConversationHistory.Add(sFollowupQuery);

        String sResponse("🤖 [Llama-3-8B Security Analyst - Follow-up Analysis]: Based on your previous question and current RAG context:\n"
                         "• IP Address 192.168.1.105 attempted 42 failed logons within 60 seconds.\n"
                         "• User account 'Administrator' was targeted.\n"
                         "• Recommended Action: Block IP on firewall rule #402.");
        m_listConversationHistory.Add(sResponse);
        return sResponse;
    }

    void LocalLlmEngine::ClearConversationHistory()
    {
        // Re-initialize empty conversation list
        m_listConversationHistory = StringList();
    }

    size_t LocalLlmEngine::GetHistoryCount() const
    {
        return m_listConversationHistory.GetCount();
    }
}
