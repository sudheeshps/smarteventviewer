#include "TelemetryWebSocketHandler.h"
#include "Logging/AppLoggerManager.h"

namespace SmartEventViewer {
    DotNetDupe::System::SmartPointer<TelemetryWebSocketHandler> TelemetryWebSocketHandler::GetInstance() {
        static auto s_instance = DotNetDupe::System::SmartPointer<TelemetryWebSocketHandler>::NewShared();
        return s_instance;
    }

    void TelemetryWebSocketHandler::OnConnected(DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::WebSockets::WebSocketContext> pContext) {
        LockCS lock(m_csLock);
        m_clients.Add(pContext);
        AppLoggerManager::Info("SERVER", String::Format("[WS] Client connected. Total clients: {0}", static_cast<double>(m_clients.GetCount())));
    }

    void TelemetryWebSocketHandler::OnMessage(DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::WebSockets::WebSocketContext> pContext, const DotNetDupe::System::String& message) {
        (void)pContext;
        (void)message;
    }

    void TelemetryWebSocketHandler::OnDisconnected(DotNetDupe::System::SmartPointer<DotNetDupe::WebAppCore::WebSockets::WebSocketContext> pContext) {
        LockCS lock(m_csLock);
        for (int i = 0; i < m_clients.GetCount(); ++i) {
            if (m_clients[i] == pContext) {
                m_clients.RemoveAt(i);
                break;
            }
        }
        AppLoggerManager::Info("SERVER", String::Format("[WS] Client disconnected. Remaining: {0}", static_cast<double>(m_clients.GetCount())));
    }

    void TelemetryWebSocketHandler::BroadcastCategoryUpdate(const String& sCategory) {
        LockCS lock(m_csLock);
        if (m_clients.GetCount() == 0) return;
        String payload = String("{\"type\":\"TELEMETRY_UPDATED\",\"category\":\"") + sCategory + "\"}";
        for (int i = 0; i < m_clients.GetCount(); ++i) {
            try {
                if (!m_clients[i].IsNull() && !m_clients[i]->GetWebSocket().IsNull()) {
                    m_clients[i]->GetWebSocket()->SendAsync(payload);
                }
            } catch (...) {
            }
        }
    }

    int TelemetryWebSocketHandler::GetConnectedClientCount() const {
        LockCS lock(m_csLock);
        return m_clients.GetCount();
    }
}
