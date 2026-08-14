#pragma once

#include "ViewerCommon.h"
#include "Core/EventRecord.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using EventList = DotNetDupe::System::Collections::Generic::List<EventRecord>;

    class RagVectorStore {
    private:
        EventList m_listIndexedEvents;

    public:
        SMARTEVENTVIEWER_API RagVectorStore();
        SMARTEVENTVIEWER_API ~RagVectorStore();

        SMARTEVENTVIEWER_API void IndexEvent(const EventRecord& eventRec);
        SMARTEVENTVIEWER_API bool QuerySimilarEvents(const String& sQuery, size_t nTopK, EventList& outResults);
        SMARTEVENTVIEWER_API size_t GetIndexedCount() const;
    };
}
