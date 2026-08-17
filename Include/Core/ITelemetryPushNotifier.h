#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "System/String.h"
#include "System/SmartPointer.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;

    class ITelemetryPushNotifier : public virtual DotNetDupe::System::Object {
    public:
        virtual ~ITelemetryPushNotifier() = default;

        virtual void BroadcastCategoryUpdate(const String& sCategory) = 0;
        virtual int GetConnectedClientCount() const = 0;
    };
}
