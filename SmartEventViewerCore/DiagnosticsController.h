#pragma once

#include "ViewerCommon.h"
#include "WebAppCore/Controllers/ControllerBase.h"
#include "System/Text/Json/JsonSerializer.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using StringList = DotNetDupe::System::Collections::Generic::List<String>;

    struct ServerLogsResponseDto
    {
        StringList Logs{};
    };

    class SMARTEVENTVIEWER_API DiagnosticsController : public DotNetDupe::WebAppCore::Controllers::ControllerBase
    {
    public:
        DiagnosticsController() = default;
        ~DiagnosticsController() override = default;

        // Server Diagnostics Logs Endpoint
        ServerLogsResponseDto GetServerLogs();
    };
}

namespace DotNetDupe {
    namespace System {
        namespace Text {
            namespace Json {
                template <typename Enable>
                struct JsonConverter<SmartEventViewer::ServerLogsResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::ServerLogsResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty(String("logs"), JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Write(value.Logs));
                        return obj;
                    }
                    static SmartEventViewer::ServerLogsResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::ServerLogsResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty(String("logs"), prop) || element.TryGetProperty(String("Logs"), prop)) dto.Logs = JsonConverter<DotNetDupe::System::Collections::Generic::List<DotNetDupe::System::String>>::Read(prop);
                        return dto;
                    }
                };
            }
        }
    }
}
