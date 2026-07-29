#include "pch.h"
#include "../Include/Ai/RagVectorStore.h"

namespace SmartEventViewer
{
    RagVectorStore::RagVectorStore() = default;
    RagVectorStore::~RagVectorStore() = default;

    void RagVectorStore::IndexEvent(const EventRecord& eventRec)
    {
        m_listIndexedEvents.Add(eventRec);
    }

    EventList RagVectorStore::QuerySimilarEvents(const String& sQuery, size_t nTopK)
    {
        (void)sQuery;
        EventList results;
        size_t nLimit = (nTopK < m_listIndexedEvents.GetCount()) ? nTopK : m_listIndexedEvents.GetCount();
        for (size_t i = 0; i < nLimit; ++i)
        {
            results.Add(m_listIndexedEvents[i]);
        }
        return results;
    }

    size_t RagVectorStore::GetIndexedCount() const
    {
        return m_listIndexedEvents.GetCount();
    }
}
