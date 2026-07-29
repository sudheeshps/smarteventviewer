#pragma once

#include "Common.h"
#include "Core/EventRecord.h"
#include "DotNetDupe/String.h"
#include "DotNetDupe/List.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using EventList = DotNetDupe::System::Collections::Generic::List<EventRecord>;

    class SMARTEVENTVIEWER_API RagVectorStore
    {
    private:
        EventList m_listIndexedEvents;

    public:
        RagVectorStore();
        ~RagVectorStore();

        void IndexEvent(const EventRecord& eventRec);
        EventList QuerySimilarEvents(const String& sQuery, size_t nTopK);
        size_t GetIndexedCount() const;
    };
}
