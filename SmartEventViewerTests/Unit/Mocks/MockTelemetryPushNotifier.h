#pragma once

#include "Core/ITelemetryPushNotifier.h"
#include "System/Collections/Generic/List.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"

namespace SmartEventViewer {
    namespace Tests {
        using StringList = DotNetDupe::System::Collections::Generic::List<String>;
        using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
        using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

        class MockTelemetryPushNotifier : public ITelemetryPushNotifier {
        private:
            StringList m_broadcastHistory{};
            int m_nClientCount{ 1 };
            mutable CriticalSection m_csLock{};

        public:
            MockTelemetryPushNotifier() = default;
            ~MockTelemetryPushNotifier() override = default;

            void BroadcastCategoryUpdate(const String& sCategory) override {
                LockCS lock(m_csLock);
                m_broadcastHistory.Add(sCategory);
            }

            int GetConnectedClientCount() const override {
                LockCS lock(m_csLock);
                return m_nClientCount;
            }

            void SetConnectedClientCount(int nCount) {
                LockCS lock(m_csLock);
                m_nClientCount = nCount;
            }

            StringList GetBroadcastHistory() const {
                LockCS lock(m_csLock);
                return m_broadcastHistory;
            }

            String GetLastBroadcast() const {
                LockCS lock(m_csLock);
                if (m_broadcastHistory.GetCount() == 0) return String("");
                return m_broadcastHistory[m_broadcastHistory.GetCount() - 1];
            }

            void ClearHistory() {
                LockCS lock(m_csLock);
                m_broadcastHistory.Clear();
            }
        };
    }
}
