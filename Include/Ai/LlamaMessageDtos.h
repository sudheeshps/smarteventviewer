#pragma once

#include "ViewerCommon.h"
#include "System/Object.h"
#include "System/String.h"
#include "System/SmartPointer.h"
#include "System/Collections/Generic/List.h"
#include "Core/EventRecord.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    template<typename T>
    using List = DotNetDupe::System::Collections::Generic::List<T>;

    enum class LlamaCommandType
    {
        Initialize = 0,
        IngestEvents = 1,
        ProcessQuery = 2,
        ProcessFollowup = 3
    };

    class LlamaRequest : public DotNetDupe::System::Object
    {
    public:
        String TaskId{};
        LlamaCommandType Command{ LlamaCommandType::ProcessQuery };
        String ChannelName{};
        String UserQuery{};
        String ModelPath{};
        List<EventRecord> ContextEvents{};

        LlamaRequest() = default;
        ~LlamaRequest() override = default;
    };

    class LlamaResponse : public DotNetDupe::System::Object
    {
    public:
        String TaskId{};
        String Status{};
        String ProgressMessage{};
        String AnalysisResult{};
        unsigned long long EventsAnalyzed{ 0 };

        LlamaResponse() = default;
        ~LlamaResponse() override = default;
    };
}
