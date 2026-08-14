#include "pch.h"
#include "../Include/Ai/RagVectorStore.h"

namespace SmartEventViewer {
    RagVectorStore::RagVectorStore() = default;
    RagVectorStore::~RagVectorStore() = default;

    void RagVectorStore::IndexEvent(const EventRecord& eventRec) {
        m_listIndexedEvents.Add(eventRec);
    }

    bool RagVectorStore::QuerySimilarEvents(const String& sQuery, size_t nTopK, EventList& outResults) {
        (void)sQuery;
        outResults.Clear();
        size_t nLimit = (nTopK < m_listIndexedEvents.GetCount()) ? nTopK : m_listIndexedEvents.GetCount();
        for (size_t i = 0; i < nLimit; ++i) {
            outResults.Add(m_listIndexedEvents[static_cast<int>(i)]);
        }
        return (outResults.GetCount() > 0);
    }

    size_t RagVectorStore::GetIndexedCount() const {
        return m_listIndexedEvents.GetCount();
    }
}
