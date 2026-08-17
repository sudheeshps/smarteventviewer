#pragma once

#include "ViewerCommon.h"
#include "System/String.h"
#include "System/Text/Json/JsonSerializer.h"

namespace SmartEventViewer {
    using String = DotNetDupe::System::String;

    struct AnalyzeRequestDto {
        String Channel{};
        String Query{};

        AnalyzeRequestDto() = default;
        AnalyzeRequestDto(const AnalyzeRequestDto&) = default;
        AnalyzeRequestDto& operator=(const AnalyzeRequestDto&) = default;
    };

    struct AnalyzeResponseDto {
        String TaskId{};
        String Status{}; // "PENDING", "PROCESSING", "DOWNLOADING", "COMPLETED", "FAILED"
        String ProgressMessage{};
        double DownloadProgress{ 0.0 }; // 0.0 to 100.0
        double DownloadRateBytesPerSec{ 0.0 };
        unsigned long long DownloadedBytes{ 0 };
        unsigned long long TotalBytes{ 0 };
        String Channel{};
        String Query{};
        String Analysis{};
        unsigned long long EventsAnalyzed{ 0 };

        AnalyzeResponseDto() = default;
        AnalyzeResponseDto(const AnalyzeResponseDto&) = default;
        AnalyzeResponseDto& operator=(const AnalyzeResponseDto&) = default;
    };

    struct AnalysisTaskItem {
        DotNetDupe::System::String TaskId{};
        AnalyzeRequestDto Request{};
    };
}

namespace DotNetDupe {
    namespace System {
        namespace Text {
            namespace Json {
                template <typename Enable>
                struct JsonConverter<SmartEventViewer::AnalyzeRequestDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::AnalyzeRequestDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("channel", JsonElement(value.Channel));
                        obj.SetProperty("query", JsonElement(value.Query));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeRequestDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeRequestDto dto;
                        JsonElement prop;
                        if ((element.TryGetProperty("channel", prop) || element.TryGetProperty("Channel", prop)) && prop.GetValueKind() == JsonValueKind::String) dto.Channel = prop.GetString();
                        if ((element.TryGetProperty("query", prop) || element.TryGetProperty("Query", prop)) && prop.GetValueKind() == JsonValueKind::String) dto.Query = prop.GetString();
                        return dto;
                    }
                };

                template <typename Enable>
                struct JsonConverter<SmartEventViewer::AnalyzeResponseDto, Enable> {
                    static JsonElement Write(const SmartEventViewer::AnalyzeResponseDto& value) {
                        JsonElement obj(JsonValueKind::Object);
                        obj.SetProperty("taskId", JsonElement(value.TaskId));
                        obj.SetProperty("status", JsonElement(value.Status));
                        obj.SetProperty("progressMessage", JsonElement(value.ProgressMessage));
                        obj.SetProperty("downloadProgress", JsonElement(value.DownloadProgress));
                        obj.SetProperty("downloadRateBytesPerSec", JsonElement(value.DownloadRateBytesPerSec));
                        obj.SetProperty("downloadedBytes", JsonElement(static_cast<double>(value.DownloadedBytes)));
                        obj.SetProperty("totalBytes", JsonElement(static_cast<double>(value.TotalBytes)));
                        obj.SetProperty("channel", JsonElement(value.Channel));
                        obj.SetProperty("query", JsonElement(value.Query));
                        obj.SetProperty("analysis", JsonElement(value.Analysis));
                        obj.SetProperty("eventsAnalyzed", JsonElement(static_cast<double>(value.EventsAnalyzed)));
                        return obj;
                    }
                    static SmartEventViewer::AnalyzeResponseDto Read(const JsonElement& element) {
                        SmartEventViewer::AnalyzeResponseDto dto;
                        JsonElement prop;
                        if (element.TryGetProperty("taskId", prop) && prop.GetValueKind() == JsonValueKind::String) dto.TaskId = prop.GetString();
                        if (element.TryGetProperty("status", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Status = prop.GetString();
                        if (element.TryGetProperty("progressMessage", prop) && prop.GetValueKind() == JsonValueKind::String) dto.ProgressMessage = prop.GetString();
                        if (element.TryGetProperty("downloadProgress", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.DownloadProgress = prop.GetDouble();
                        if (element.TryGetProperty("downloadRateBytesPerSec", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.DownloadRateBytesPerSec = prop.GetDouble();
                        if (element.TryGetProperty("downloadedBytes", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.DownloadedBytes = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("totalBytes", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.TotalBytes = static_cast<unsigned long long>(prop.GetDouble());
                        if (element.TryGetProperty("channel", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Channel = prop.GetString();
                        if (element.TryGetProperty("query", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Query = prop.GetString();
                        if (element.TryGetProperty("analysis", prop) && prop.GetValueKind() == JsonValueKind::String) dto.Analysis = prop.GetString();
                        if (element.TryGetProperty("eventsAnalyzed", prop) && prop.GetValueKind() == JsonValueKind::Number) dto.EventsAnalyzed = static_cast<unsigned long long>(prop.GetDouble());
                        return dto;
                    }
                };
            }
        }
    }
}
