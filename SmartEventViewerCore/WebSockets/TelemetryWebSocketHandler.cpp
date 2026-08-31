#include "pch.h"
#include "WebSockets/TelemetryWebSocketHandler.h"
#include "Logging/AppLoggerManager.h"
#include "System/Net/WebSockets/WebSocketException.h"
#include "System/Exception.h"
#include <cstdio>

namespace SmartEventViewer {
    using WebSocketException = DotNetDupe::System::Net::WebSockets::WebSocketException;
    using Exception = DotNetDupe::System::Exception;

    static String FormatClientId(unsigned long long key) {
        char buf[32];
        snprintf(buf, sizeof(buf), "0x%llX", key);
        return String(buf);
    }

    unsigned long long TelemetryWebSocketHandler::GetKey(const DotNetDupe::System::SmartPointer<WebSocketContext>& pContext) {
        if (pContext.IsNull() || pContext->GetWebSocket().IsNull()) return 0;
        return reinterpret_cast<unsigned long long>(pContext->GetWebSocket().Get());
    }

    void TelemetryWebSocketHandler::OnConnected(DotNetDupe::System::SmartPointer<WebSocketContext> pContext) {
        unsigned long long key = GetKey(pContext);
        if (key != 0) m_clients.TryAdd(key, pContext);
        String sId = FormatClientId(key);
        AppLoggerManager::Info("SERVER", String::Format("[WS] Client connected. ClientId: {0}, Total clients: {1}", sId, static_cast<double>(m_clients.GetCount())));
        SendToClient(pContext, "{\"type\":\"TELEMETRY_UPDATED\",\"category\":\"summary\"}");
    }

    void TelemetryWebSocketHandler::OnMessage(DotNetDupe::System::SmartPointer<WebSocketContext> pContext, const DotNetDupe::System::String& message) {
        if (message.Contains("PING")) {
            SendToClient(pContext, "{\"type\":\"PONG\"}");
        }
    }

    void TelemetryWebSocketHandler::OnDisconnected(DotNetDupe::System::SmartPointer<WebSocketContext> pContext) {
        unsigned long long key = GetKey(pContext);
        DotNetDupe::System::SmartPointer<WebSocketContext> removed;
        if (key != 0) m_clients.TryRemove(key, removed);
        String sId = FormatClientId(key);
        AppLoggerManager::Info("SERVER", String::Format("[WS] Client disconnected. ClientId: {0}, Remaining: {1}", sId, static_cast<double>(m_clients.GetCount())));
    }

    bool TelemetryWebSocketHandler::SendToClient(const DotNetDupe::System::SmartPointer<WebSocketContext>& pContext, const String& sPayload) {
        if (pContext.IsNull() || pContext->GetWebSocket().IsNull()) return false;
        auto spWs = pContext->GetWebSocket();
        unsigned long long key = GetKey(pContext);
        if (spWs->GetState() != DotNetDupe::System::Net::WebSockets::WebSocketState::Open) return false;
        try {
            return spWs->SendAsync(sPayload);
        } catch (const WebSocketException& ex) {
            AppLoggerManager::Warning("SERVER", String::Format("[WS] WebSocketException on ClientId {0}: {1}", FormatClientId(key), ex.What()));
            return false;
        } catch (const Exception& ex) {
            AppLoggerManager::Warning("SERVER", String::Format("[WS] Framework exception on ClientId {0}: {1}", FormatClientId(key), ex.What()));
            return false;
        } catch (...) {
            AppLoggerManager::Warning("SERVER", String::Format("[WS] Unknown send error on ClientId {0}", FormatClientId(key)));
            return false;
        }
    }

    void TelemetryWebSocketHandler::BroadcastCategoryUpdate(const String& sCategory) {
        if (m_clients.IsEmpty()) return;
        String payload = String("{\"type\":\"TELEMETRY_UPDATED\",\"category\":\"") + sCategory + "\"}";
        auto clients = m_clients.GetValues();
        for (int i = 0; i < clients.GetLength(); ++i) {
            if (!SendToClient(clients[i], payload)) {
                unsigned long long key = GetKey(clients[i]);
                DotNetDupe::System::SmartPointer<WebSocketContext> dead;
                bool bRemoved = m_clients.TryRemove(key, dead);
                AppLoggerManager::Warning("SERVER", String::Format("[WS] Pruned dead client ClientId {0} on '{1}'. Removed: {2}, Remaining: {3}", FormatClientId(key), sCategory, bRemoved ? "true" : "false", static_cast<double>(m_clients.GetCount())));
            }
        }
    }

    int TelemetryWebSocketHandler::GetConnectedClientCount() const {
        return m_clients.GetCount();
    }
}
