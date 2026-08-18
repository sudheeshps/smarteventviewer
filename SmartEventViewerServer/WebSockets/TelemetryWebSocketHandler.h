#pragma once

#include "WebAppCore/WebSockets/WebSocketContext.h"
#include "Core/ITelemetryPushNotifier.h"
#include "System/Collections/Concurrent/ConcurrentDictionary.h"
#include "System/SmartPointer.h"
#include <cstdint>

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    using WebSocketContext = DotNetDupe::WebAppCore::WebSockets::WebSocketContext;

    class TelemetryWebSocketHandler : public DotNetDupe::WebAppCore::WebSockets::IWebSocketHandler, public ITelemetryPushNotifier {
    private:
        using ClientMap = DotNetDupe::System::Collections::Concurrent::ConcurrentDictionary<unsigned long long, DotNetDupe::System::SmartPointer<WebSocketContext>>;
        ClientMap m_clients{};

        static unsigned long long GetKey(const DotNetDupe::System::SmartPointer<WebSocketContext>& pContext);
        static bool SendToClient(const DotNetDupe::System::SmartPointer<WebSocketContext>& pContext, const String& sPayload);

    public:
        TelemetryWebSocketHandler() = default;
        ~TelemetryWebSocketHandler() override = default;

        void OnConnected(DotNetDupe::System::SmartPointer<WebSocketContext> pContext) override;
        void OnMessage(DotNetDupe::System::SmartPointer<WebSocketContext> pContext, const DotNetDupe::System::String& message) override;
        void OnDisconnected(DotNetDupe::System::SmartPointer<WebSocketContext> pContext) override;

        void BroadcastCategoryUpdate(const String& sCategory) override;
        int GetConnectedClientCount() const override;
    };
}
