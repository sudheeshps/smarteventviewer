#pragma once

#include "WebAppCore/WebSockets/WebSocketContext.h"
#include "Core/ITelemetryPushNotifier.h"
#include "System/Collections/Generic/List.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "System/SmartPointer.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;

    class TelemetryWebSocketHandler : public DotNetDupe::WebAppCore::WebSockets::IWebSocketHandler, public ITelemetryPushNotifier {
    private:
        DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::WebSockets::WebSocketContext>> m_clients{};
        mutable CriticalSection m_csLock{};

    public:
        TelemetryWebSocketHandler() = default;
        ~TelemetryWebSocketHandler() override = default;

        void OnConnected(DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::WebSockets::WebSocketContext> pContext) override;
        void OnMessage(DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::WebSockets::WebSocketContext> pContext, const DotNetDupe::System::String& message) override;
        void OnDisconnected(DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::WebSockets::WebSocketContext> pContext) override;

        void BroadcastCategoryUpdate(const String& sCategory) override;
        int GetConnectedClientCount() const override;
    };
}
