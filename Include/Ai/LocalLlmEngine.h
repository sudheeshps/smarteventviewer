#pragma once

#include "../Common.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include "System/Collections/Generic/List.h"

// Forward declaration of internal llama context structures
struct llama_model;
struct llama_context;

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    class EventRecord;

    class LocalLlmEngine
    {
    private:
        String m_sModelPath{};
        bool m_bIsLoaded{ false };
        StringList m_listConversationHistory{};

        // In-process embedded llama.cpp runtime contexts
        llama_model* m_pLlamaModel{ nullptr };
        llama_context* m_pLlamaCtx{ nullptr };

    public:
        SMARTEVENTVIEWER_API LocalLlmEngine();
        SMARTEVENTVIEWER_API ~LocalLlmEngine();

        // Non-copyable for safe memory management of in-process native llama handles
        LocalLlmEngine(const LocalLlmEngine&) = delete;
        LocalLlmEngine& operator=(const LocalLlmEngine&) = delete;

        SMARTEVENTVIEWER_API bool Initialize(const String& sModelPath);
        SMARTEVENTVIEWER_API void Unload();

        SMARTEVENTVIEWER_API String ProcessQuery(const String& sNaturalLanguageQuery, const EventRecord* pContextEvents, unsigned int uEventCount);
        SMARTEVENTVIEWER_API String ProcessFollowupQuery(const String& sFollowupQuery, const EventRecord* pContextEvents, unsigned int uEventCount);
        SMARTEVENTVIEWER_API void ClearConversationHistory();
        SMARTEVENTVIEWER_API size_t GetHistoryCount() const;
        SMARTEVENTVIEWER_API bool IsModelLoaded() const { return m_bIsLoaded; }
    };
}
