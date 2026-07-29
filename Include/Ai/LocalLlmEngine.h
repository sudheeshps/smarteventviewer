#pragma once

#include "Common.h"
#include "Core/EventRecord.h"
#include "DotNetDupe/String.h"
#include "DotNetDupe/List.h"

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

    public:
        LocalLlmEngine();

        bool Initialize(const String& sModelPath);
        String ProcessQuery(const String& sNaturalLanguageQuery, const EventRecord* pContextEvents, unsigned int uEventCount);
        String ProcessFollowupQuery(const String& sFollowupQuery, const EventRecord* pContextEvents, unsigned int uEventCount);
        void ClearConversationHistory();
        size_t GetHistoryCount() const;
    };
}
