#pragma once

#include "Core/IEventLogReader.h"
#include "System/Collections/Generic/Dictionary.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"

namespace SmartEventViewer {
    namespace Tests {
        using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
        using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

        class MockEventLogReader : public IEventLogReader {
        private:
            StringList m_channels{};
            DotNetDupe::System::Collections::Generic::Dictionary<String, DotNetDupe::System::Collections::Generic::List<EventRecord>> m_eventsByChannel{};
            mutable CriticalSection m_csLock{};

            static void SliceEvents(const DotNetDupe::System::Collections::Generic::List<EventRecord>& source,
                                    size_t nMaxCount, size_t nStartIndex, bool bReverseOrder,
                                    DotNetDupe::System::Collections::Generic::List<EventRecord>& results) {
                int iTotal = source.GetCount();
                for (size_t i = 0; i < nMaxCount; ++i) {
                    size_t idx = bReverseOrder ? (iTotal > 0 && nStartIndex + i < (size_t)iTotal ? (size_t)(iTotal - 1 - (nStartIndex + i)) : (size_t)-1)
                                               : (nStartIndex + i < (size_t)iTotal ? nStartIndex + i : (size_t)-1);
                    if (idx == (size_t)-1 || idx >= (size_t)iTotal) break;
                    results.Add(source[static_cast<int>(idx)]);
                }
            }

        public:
            MockEventLogReader() {
                AddChannel("Application");
                AddChannel("System");
                AddChannel("Security");
            }
            ~MockEventLogReader() override = default;

            StringList GetEventChannels() override {
                LockCS lock(m_csLock);
                return m_channels;
            }

            unsigned long long GetChannelEventCount(const String& sChannelName) override {
                LockCS lock(m_csLock);
                DotNetDupe::System::Collections::Generic::List<EventRecord> evts;
                if (m_eventsByChannel.TryGetValue(sChannelName, evts)) {
                    return static_cast<unsigned long long>(evts.GetCount());
                }
                return 0ULL;
            }

            bool GetChannelLevelCounts(const String& sChannelName, EventLevelCounts& outCounts) override {
                LockCS lock(m_csLock);
                DotNetDupe::System::Collections::Generic::List<EventRecord> evts;
                if (!m_eventsByChannel.TryGetValue(sChannelName, evts)) return false;
                outCounts = EventLevelCounts{};
                for (int i = 0; i < evts.GetCount(); ++i) {
                    auto lvl = evts[i].GetLevel();
                    if (lvl == EventLevel::Critical) outCounts.CriticalCount++;
                    else if (lvl == EventLevel::Error) outCounts.ErrorCount++;
                    else if (lvl == EventLevel::Warning) outCounts.WarningCount++;
                    else if (lvl == EventLevel::Informational) outCounts.InfoCount++;
                    else if (lvl == EventLevel::Verbose) outCounts.VerboseCount++;
                }
                return true;
            }

            DotNetDupe::System::Collections::Generic::List<EventRecord> ReadEvents(
                const String& sChannelName, size_t nMaxCount, size_t nStartIndex = 0, bool bReverseOrder = true, EventLevel eLevel = EventLevel::LogAlways) override {
                LockCS lock(m_csLock);
                DotNetDupe::System::Collections::Generic::List<EventRecord> results;
                DotNetDupe::System::Collections::Generic::List<EventRecord> evts;
                if (m_eventsByChannel.TryGetValue(sChannelName, evts)) {
                    if (eLevel != EventLevel::LogAlways) {
                        DotNetDupe::System::Collections::Generic::List<EventRecord> filtered;
                        for (int i = 0; i < evts.GetCount(); ++i) {
                            if (evts[i].GetLevel() == eLevel) filtered.Add(evts[i]);
                        }
                        SliceEvents(filtered, nMaxCount, nStartIndex, bReverseOrder, results);
                    } else {
                        SliceEvents(evts, nMaxCount, nStartIndex, bReverseOrder, results);
                    }
                }
                return results;
            }

            void AddChannel(const String& sChannelName) {
                LockCS lock(m_csLock);
                if (!m_channels.Contains(sChannelName)) {
                    m_channels.Add(sChannelName);
                    m_eventsByChannel.Add(sChannelName, DotNetDupe::System::Collections::Generic::List<EventRecord>{});
                }
            }

            void AddEvent(const String& sChannelName, const EventRecord& record) {
                LockCS lock(m_csLock);
                AddChannel(sChannelName);
                m_eventsByChannel[sChannelName].Add(record);
            }

            void Clear() {
                LockCS lock(m_csLock);
                m_channels.Clear();
                m_eventsByChannel.Clear();
            }
        };
    }
}
