#include "pch.h"
#include "Core/TelemetryWebSocketHandler.h"
#include "System/Console.h"
#include "System/Exception.h"
#include "System/SystemException.h"

using Console = DotNetDupe::System::Console;
using WebSocketState = DotNetDupe::System::Net::WebSockets::WebSocketState;
using BasicCharException = DotNetDupe::System::Exception;
using BasicCharSystemException = DotNetDupe::System::SystemException;

namespace SmartEventViewer {
    DotNetDupe::System::SmartPointer<TelemetryWebSocketHandler> TelemetryWebSocketHandler::GetInstance() {
        static DotNetDupe::System::SmartPointer<TelemetryWebSocketHandler> instance = DotNetDupe::System::SmartPointer<TelemetryWebSocketHandler>::NewShared();
        return instance;
    }

    void TelemetryWebSocketHandler::OnConnected(DotNetDupe::System::SmartPointer<WebSocketContext> pContext) {
        try {
            LockCS lock(m_csLock);
            m_clients.Add(pContext);
            Console::WriteLine("[TELEMETRY_WS] Client connected to /ws/telemetry");
        } catch (const BasicCharSystemException& sysEx) {
            Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] OnConnected DotNetDupe SystemException: {0}", sysEx.What()));
        } catch (const BasicCharException& ex) {
            Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] OnConnected DotNetDupe Exception: {0}", ex.What()));
        } catch (...) {
            Console::WriteLine("[TELEMETRY_WS_ERROR] OnConnected failed with unknown exception.");
        }
    }

    void TelemetryWebSocketHandler::OnMessage(DotNetDupe::System::SmartPointer<WebSocketContext> pContext, const String& message) {
        // Optional client ping/pong response
    }

    void TelemetryWebSocketHandler::OnDisconnected(DotNetDupe::System::SmartPointer<WebSocketContext> pContext) {
        try {
            LockCS lock(m_csLock);
            for (int i = 0; i < m_clients.GetCount(); ++i) {
                if (m_clients[i].Get() == pContext.Get()) {
                    m_clients.RemoveAt(i);
                    break;
                }
            }
            Console::WriteLine("[TELEMETRY_WS] Client disconnected from /ws/telemetry");
        } catch (const BasicCharSystemException& sysEx) {
            Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] OnDisconnected DotNetDupe SystemException: {0}", sysEx.What()));
        } catch (const BasicCharException& ex) {
            Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] OnDisconnected DotNetDupe Exception: {0}", ex.What()));
        } catch (...) {
            Console::WriteLine("[TELEMETRY_WS_ERROR] OnDisconnected failed with unknown exception.");
        }
    }

    void TelemetryWebSocketHandler::BroadcastCategoryUpdate(const String& sCategory) {
        String sPayload = String::Format("{{\"type\":\"TELEMETRY_UPDATED\",\"category\":\"{0}\"}}", sCategory);

        LockCS lock(m_csLock);
        int iClientCount = m_clients.GetCount();
        if (iClientCount == 0) {
            return;
        }

        Console::WriteLine(String::Format("[TELEMETRY_WS] Broadcasting update for '{0}' to {1} connected clients...", sCategory, iClientCount));
        DotNetDupe::System::Collections::Generic::List<int> toRemove{};

        for (int i = 0; i < m_clients.GetCount(); ++i) {
            try {
                auto clientCtx = m_clients[i];
                if (clientCtx.IsNull() || clientCtx->GetWebSocket().IsNull()) {
                    Console::WriteLine(String::Format("[TELEMETRY_WS_WARN] Client context at index {0} is null", i));
                    toRemove.Add(i);
                    continue;
                }

                auto ws = clientCtx->GetWebSocket();
                if (ws->GetState() != WebSocketState::Open) {
                    Console::WriteLine(String::Format("[TELEMETRY_WS] Client websocket at index {0} is not Open (State: {1})", i, static_cast<int>(ws->GetState())));
                    toRemove.Add(i);
                    continue;
                }

                if (!ws->SendAsync(sPayload)) {
                    Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] SendAsync failed for client index {0}", i));
                    toRemove.Add(i);
                }
            } catch (const BasicCharSystemException& sysEx) {
                Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] SendAsync DotNetDupe SystemException for client index {0}: {1}", i, sysEx.What()));
                toRemove.Add(i);
            } catch (const BasicCharException& ex) {
                Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] SendAsync DotNetDupe Exception for client index {0}: {1}", i, ex.What()));
                toRemove.Add(i);
            } catch (...) {
                Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] SendAsync unknown exception for client index {0}", i));
                toRemove.Add(i);
            }
        }

        for (int i = toRemove.GetCount() - 1; i >= 0; --i) {
            try {
                m_clients.RemoveAt(toRemove[i]);
            } catch (const BasicCharException& ex) {
                Console::WriteLine(String::Format("[TELEMETRY_WS_ERROR] Failed to prune client index {0}: {1}", toRemove[i], ex.What()));
            } catch (...) {}
        }
    }
}
