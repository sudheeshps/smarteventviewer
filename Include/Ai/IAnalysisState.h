#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include "Dto/AnalysisDtos.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;
    template <typename T>
    using SmartPointer = DotNetDupe::System::SmartPointer<T>;

    class AnalysisService;

    class SMARTEVENTVIEWER_API IAnalysisState : public virtual DotNetDupe::System::Object {
    public:
        ~IAnalysisState() override = default;

        virtual String GetStateName() const = 0;
        virtual String GetPublicStatus() const = 0;
        virtual bool IsTerminal() const { return false; }
        virtual void Execute(AnalysisService& context, const SmartPointer<AnalysisTaskItem>& pItem) = 0;
    };
}
