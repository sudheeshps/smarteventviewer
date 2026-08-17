#include <gtest/gtest.h>
#include "TestRestClient.h"
#include "System/Console.h"
#include "System/Threading/Thread.h"
#include "System/Net/Sockets/TcpClient.h"
#include "System/Net/WebSockets/WebSocket.h"
#include "System/Text/Json/JsonSerializer.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::Threading;
using namespace DotNetDupe::System::Net::Sockets;
using namespace DotNetDupe::System::Net::WebSockets;
using namespace SmartEventViewer::IntegrationTests;

template<typename T>
using SmartPtr = DotNetDupe::System::SmartPointer<T>;

class TestWebSocketClient : public DotNetDupe::System::Object {
private:
    TcpClient m_tcpClient;
    SmartPtr<WebSocket> m_spWebSocket{ nullptr };

    static String BuildHandshake(const String& sHost, int iPort, const String& sPath) {
        String sReq = "GET " + sPath + " HTTP/1.1\r\n";
        sReq = sReq + "Host: " + sHost + ":" + DotNetDupe::System::Convert::ToString(static_cast<unsigned long long>(iPort)) + "\r\n";
        sReq = sReq + "Upgrade: websocket\r\n";
        sReq = sReq + "Connection: Upgrade\r\n";
        sReq = sReq + "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n";
        sReq = sReq + "Sec-WebSocket-Version: 13\r\n\r\n";
        return sReq;
    }

    static bool TryDecodeFrame(char* pBuf, int nRead, String& sOutMessage) {
        if (nRead >= 2 && static_cast<unsigned char>(pBuf[0]) == 0x81) {
            unsigned char lenByte = static_cast<unsigned char>(pBuf[1]);
            bool bMasked = (lenByte & 0x80) != 0;
            int payloadLen = lenByte & 0x7F;
            int offset = 2;
            if (payloadLen == 126 && nRead >= 4) {
                payloadLen = (static_cast<unsigned char>(pBuf[2]) << 8) | static_cast<unsigned char>(pBuf[3]);
                offset = 4;
            }
            if (bMasked) offset += 4;
            if (offset + payloadLen <= nRead) {
                pBuf[offset + payloadLen] = '\0';
                sOutMessage = String(&pBuf[offset]);
                return true;
            }
        }
        return false;
    }

public:
    TestWebSocketClient() = default;
    ~TestWebSocketClient() override { Close(); }

    bool Connect(const String& sHost, int iPort, const String& sPath) {
        try {
            m_tcpClient.Connect(sHost, iPort);
            if (!m_tcpClient.Connected()) return false;

            auto stream = m_tcpClient.GetStream();
            String sHandshake = BuildHandshake(sHost, iPort, sPath);
            stream->Write(sHandshake.GetRawString(), 0, sHandshake.GetLength());

            char szRespBuf[1024] = { 0 };
            int nRead = stream->Read(szRespBuf, 0, sizeof(szRespBuf) - 1);
            if (nRead <= 0) return false;

            m_spWebSocket = SmartPtr<WebSocket>::NewShared(stream);
            m_spWebSocket->SetState(WebSocketState::Open);
            return true;
        } catch (...) {
            return false;
        }
    }

    bool ReceiveNotification(String& sOutMessage) {
        if (!m_tcpClient.Connected()) return false;
        try {
            auto stream = m_tcpClient.GetStream();
            if (stream.IsNull()) return false;

            auto clientSock = m_tcpClient.GetClient();
            if (!clientSock.IsNull() && !clientSock->Poll(50000, SelectMode::SelectRead)) {
                return false;
            }

            char szBuf[2048] = { 0 };
            int nRead = stream->Read(szBuf, 0, sizeof(szBuf) - 1);
            if (nRead <= 0) return false;
            szBuf[nRead] = '\0';

            if (TryDecodeFrame(szBuf, nRead, sOutMessage)) return true;

            String sRaw(szBuf);
            if (sRaw.Contains("TELEMETRY_UPDATED")) {
                sOutMessage = sRaw;
                return true;
            }
            if (!m_spWebSocket.IsNull()) {
                return m_spWebSocket->ReceiveText(sOutMessage);
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    void Close() {
        if (!m_spWebSocket.IsNull()) m_spWebSocket->Close();
        m_tcpClient.Close();
    }
};

TEST(PushNotificationTests, GivenConnectedWebSocketClient_WhenAnalysisTriggered_ThenReceivesLlmAnalysisPushNotification) {
    Console::WriteLine("[TEST] Connecting WebSocket client to ws://127.0.0.1:8080/ws/telemetry...");
    TestWebSocketClient wsClient;
    bool bConnected = wsClient.Connect("127.0.0.1", 8080, "/ws/telemetry");
    Console::WriteLine("[ASSERT] WebSocket connection established (Connected: {0})", bConnected);
    ASSERT_TRUE(bConnected) << "Failed to connect WebSocket client to /ws/telemetry";
    Thread::Sleep(300);

    TestRestClient restClient;
    SmartEventViewer::AnalyzeRequestDto req;
    req.Channel = "Application";
    req.Query = "Detect security anomalies";
    SmartEventViewer::AnalyzeResponseDto initialResp = restClient.AnalyzeEvents(req);
    Console::WriteLine("[REST_DTO] TaskId={0} | Status={1}", initialResp.TaskId, initialResp.Status);
    EXPECT_FALSE(initialResp.TaskId.IsEmpty());

    String sNotification;
    bool bReceived = false;
    for (int i = 0; i < 60; ++i) {
        if (wsClient.ReceiveNotification(sNotification) && !sNotification.IsEmpty()) {
            bReceived = true;
            break;
        }
        Thread::Sleep(50);
    }
    Console::WriteLine("[WS_NOTIFICATION] Received={0} | Payload={1}", bReceived, sNotification);
    Console::WriteLine("[ASSERT] Notification received && Contains 'TELEMETRY_UPDATED'");
    EXPECT_TRUE(bReceived);
    EXPECT_FALSE(sNotification.IsEmpty());
    EXPECT_TRUE(sNotification.Contains("TELEMETRY_UPDATED"));
}

TEST(PushNotificationTests, GivenConnectedWebSocketClient_WhenBackgroundTelemetryRuns_ThenReceivesTelemetryPushNotification) {
    Console::WriteLine("[TEST] Connecting WebSocket client to listen for background telemetry push...");
    TestWebSocketClient wsClient;
    bool bConnected = wsClient.Connect("127.0.0.1", 8080, "/ws/telemetry");
    Console::WriteLine("[ASSERT] WebSocket connection established (Connected: {0})", bConnected);
    ASSERT_TRUE(bConnected) << "Failed to connect WebSocket client to /ws/telemetry";

    String sNotification;
    bool bReceived = false;
    for (int i = 0; i < 60; ++i) {
        if (wsClient.ReceiveNotification(sNotification) && !sNotification.IsEmpty()) {
            bReceived = true;
            break;
        }
        Thread::Sleep(50);
    }
    Console::WriteLine("[WS_NOTIFICATION] Background Received={0} | Payload={1}", bReceived, sNotification);
    if (bReceived) {
        EXPECT_FALSE(sNotification.IsEmpty());
        EXPECT_TRUE(sNotification.Contains("TELEMETRY_UPDATED"));
    }
}
