#pragma once

#include "ViewerCommon.h"
#include "System/SmartPointer.h"
#include "System/String.h"
#include "System/Collections/Generic/List.h"
#include "System/Threading/CriticalSection.h"
#include "System/Threading/Lock.h"
#include "WebAppCore/WebSockets/WebSocketContext.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using CriticalSection = DotNetDupe::System::Threading::CriticalSection;
    using LockCS = DotNetDupe::System::Threading::Lock<CriticalSection>;
    using WebSocketContext = DotNetDupe::WebAppCore::WebSockets::WebSocketContext;
    using IWebSocketHandler = DotNetDupe::WebAppCore::WebSockets::IWebSocketHandler;

    class SMARTEVENTVIEWER_API TelemetryWebSocketHandler : public IWebSocketHandler
    {
    private:
        DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::SmartPointer<WebSocketContext>> m_clients{};
        CriticalSection m_csLock{};

    public:
        TelemetryWebSocketHandler() = default;
        ~TelemetryWebSocketHandler() override = default;

        static DotNetDupe::System::SmartPointer<TelemetryWebSocketHandler> GetInstance();

        void OnConnected(DotNetDupe::System::SmartPointer<WebSocketContext> pContext) override;
        void OnMessage(DotNetDupe::System::SmartPointer<WebSocketContext> pContext, const String& message) override;
        void OnDisconnected(DotNetDupe::System::SmartPointer<WebSocketContext> pContext) override;

        void BroadcastCategoryUpdate(const String& sCategory);
    };
}
