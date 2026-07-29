#pragma once

#include "Common.h"
#include "Core/EventRecord.h"
#include "DotNetDupe/String.h"
#include "DotNetDupe/List.h"

// Forward declaration of internal llama context structures
struct llama_model;
struct llama_context;

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    class SMARTEVENTVIEWER_API LocalLlmEngine
    {
    private:
        String m_sModelPath{};
        bool m_bIsLoaded{ false };
        StringList m_listConversationHistory{};

        // In-process embedded llama.cpp runtime contexts
        llama_model* m_pLlamaModel{ nullptr };
        llama_context* m_pLlamaCtx{ nullptr };

    public:
        LocalLlmEngine();
        ~LocalLlmEngine();

        // Non-copyable for safe memory management of in-process native llama handles
        LocalLlmEngine(const LocalLlmEngine&) = delete;
        LocalLlmEngine& operator=(const LocalLlmEngine&) = delete;

        bool Initialize(const String& sModelPath);
        void Unload();

        String ProcessQuery(const String& sNaturalLanguageQuery, const EventRecord* pContextEvents, unsigned int uEventCount);
        String ProcessFollowupQuery(const String& sFollowupQuery, const EventRecord* pContextEvents, unsigned int uEventCount);
        void ClearConversationHistory();
        size_t GetHistoryCount() const;
        bool IsModelLoaded() const { return m_bIsLoaded; }
    };
}
