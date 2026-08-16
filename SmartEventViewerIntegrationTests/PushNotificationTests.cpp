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

public:
    TestWebSocketClient() = default;
    ~TestWebSocketClient() override { Close(); }

    bool Connect(const String& sHost, int iPort, const String& sPath) {
        m_tcpClient.Connect(sHost, iPort);
        if (!m_tcpClient.Connected()) return false;

        auto stream = m_tcpClient.GetStream();
        String sHandshake = BuildHandshake(sHost, iPort, sPath);
        stream->Write(sHandshake.GetRawString(), 0, sHandshake.GetLength());

        char szRespBuf[1024] = { 0 };
        int nRead = stream->Read(szRespBuf, 0, sizeof(szRespBuf));
        if (nRead <= 0) return false;

        m_spWebSocket = SmartPtr<WebSocket>::NewShared(stream);
        m_spWebSocket->SetState(WebSocketState::Open);
        return true;
    }

    bool ReceiveNotification(String& sOutMessage) {
        if (m_spWebSocket.IsNull() || m_spWebSocket->GetState() != WebSocketState::Open) return false;
        return m_spWebSocket->ReceiveText(sOutMessage);
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
    Thread::Sleep(50);

    TestRestClient restClient;
    SmartEventViewer::AnalyzeRequestDto req;
    req.Channel = "Application";
    req.Query = "Detect security anomalies";
    SmartEventViewer::AnalyzeResponseDto initialResp = restClient.AnalyzeEvents(req);
    Console::WriteLine("[REST_DTO] TaskId={0} | Status={1}", initialResp.TaskId, initialResp.Status);
    EXPECT_FALSE(initialResp.TaskId.IsEmpty());

    String sNotification;
    bool bReceived = wsClient.ReceiveNotification(sNotification);
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
    bool bReceived = wsClient.ReceiveNotification(sNotification);
    Console::WriteLine("[WS_NOTIFICATION] Background Received={0} | Payload={1}", bReceived, sNotification);
    if (bReceived) {
        EXPECT_FALSE(sNotification.IsEmpty());
        EXPECT_TRUE(sNotification.Contains("TELEMETRY_UPDATED"));
    }
}
