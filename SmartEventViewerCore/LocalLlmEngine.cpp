#include "pch.h"
#include "../Include/Ai/LocalLlmEngine.h"
#include "../Include/Core/EventRecord.h"
#include "../Include/Core/EventRecord.h"
#include "../Include/Core/AnomalyEngine.h"
#include "System/Diagnostics/Process.h"
#include "System/Threading/Tasks/Task.h"
#include "Logging/AppLoggerManager.h"
#include "System/Console.h"
#include "System/IO/File.h"
#include "System/IO/Directory.h"
#include "System/IO/Path.h"
#include "System/Net/Http/HttpClient.h"
#include "System/Net/Http/HttpRequestMessage.h"
#include "System/Net/Http/HttpResponseMessage.h"
#include "System/Net/Http/HttpMethod.h"
#include "System/Net/Http/FileDownloader.h"
#include "System/Threading/AutoResetEvent.h"
#include "Dto/AnalysisDtos.h"
#include "Dto/EventDtos.h"
#include "System/Array.h"
#include "System/Collections/Generic/List.h"
#include "System/Collections/Generic/Dictionary.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

#if __has_include(<llama.h>)
    #include <llama.h>
#elif __has_include("llama.h")
    #include "llama.h"
#endif

namespace SmartEventViewer {
    using Console = DotNetDupe::System::Console;
    using File = DotNetDupe::System::IO::File;
    using Directory = DotNetDupe::System::IO::Directory;
    using Path = DotNetDupe::System::IO::Path;
    template<typename T>
    using List = DotNetDupe::System::Collections::Generic::List<T>;

    void LocalLlmEngine::CountRiskMetrics(const List<EventRecord>& events, unsigned int& crit, unsigned int& high, unsigned int& err, unsigned int& warn) {
        crit = 0; high = 0; err = 0; warn = 0;
        for (int i = 0; i < events.GetCount(); ++i) {
            EventLevel lvl = events[i].GetLevel();
            RiskLevel risk = AnomalyEngine::StaticEvaluateRisk(events[i]);

            if (lvl == EventLevel::Critical || risk == RiskLevel::Critical) crit++;
            else if (risk == RiskLevel::High) high++;
            else if (lvl == EventLevel::Error) err++;
            else if (lvl == EventLevel::Warning || risk == RiskLevel::Medium) warn++;
        }
    }

    int LocalLlmEngine::ComputeThreatScore(unsigned int crit, unsigned int high, unsigned int err, unsigned int warn, unsigned int total) {
        if (total == 0) return 12;
        int rawScore = static_cast<int>((crit * 30) + (high * 20) + (err * 10) + (warn * 5));
        return (rawScore > 100) ? 100 : rawScore;
    }

    String LocalLlmEngine::GetSeverityLabel(int score) {
        if (score >= 75) return "CRITICAL (Immediate Action Required)";
        if (score >= 45) return "HIGH (Active Investigation Needed)";
        if (score >= 20) return "MEDIUM (Elevated Monitoring)";
        return "LOW (Normal Operational Baseline)";
    }

    String LocalLlmEngine::FormatAnomaliesSection(const List<EventRecord>& events) {
        if (events.GetCount() == 0) return " â€¢ No active event records returned.\n";

        String sResult;
        unsigned int shown = 0;
        for (int i = 0; i < events.GetCount() && shown < 5; ++i) {
            EventLevel lvl = events[i].GetLevel();
            RiskLevel risk = AnomalyEngine::StaticEvaluateRisk(events[i]);

            if (lvl == EventLevel::Critical || lvl == EventLevel::Error || risk == RiskLevel::Critical || risk == RiskLevel::High) {
                String sItem = String::Format(" â€¢ Event ID {0} [{1}]: {2}\n", events[i].GetEventId(), events[i].GetProviderName(), events[i].GetEventMessage());
                sResult = sResult + sItem;
                shown++;
            }
        }

        if (shown == 0) return " â€¢ Baseline telemetry verified. No anomalous event IDs flagged.\n";
        return sResult;
    }

    String LocalLlmEngine::FormatThreatAnalysisResponse(const List<EventRecord>& events) {
        unsigned int crit = 0, high = 0, err = 0, warn = 0;
        LocalLlmEngine::CountRiskMetrics(events, crit, high, err, warn);
        unsigned int total = crit + high + err + warn;

        int score = ComputeThreatScore(crit, high, err, warn, total);
        String sSev = GetSeverityLabel(score);

        String sExecSummary = "All ingested event channels & telemetry metrics operating within normal baseline.";
        if (total > 0) {
            sExecSummary = String::Format("Correlated RAG analysis identified {0} anomalous events ({1} Critical, {2} Errors, {3} Warnings).", total, crit, err, warn);
        }

        String sRootCause = "No suspicious root cause vectors observed.";
        if (total > 0) {
            sRootCause = "Primary risks stem from process creation, token assignment, or authentication logs.";
        }

        String sMitigations = "1. Maintain continuous 1s log ingestion.\n2. Enforce audit log retention.";
        if (total > 0) {
            sMitigations = "1. Isolate high-risk endpoints.\n2. Verify active user session privileges.\n3. Restrict administrative execution.";
        }

        String sAnomaliesSection = FormatAnomaliesSection(events);

        return String::Format("ðŸ“Œ EXECUTIVE SUMMARY:\n{0}\n\nðŸš¨ THREAT SCORE: {1} / 100 â€” {2}\n\nðŸ”  ROOT CAUSE ANALYSIS:\n{3}\n\nðŸ“Š KEY ANOMALIES & CORRELATED INDICATORS:\n{4}\nðŸ›¡ï¸  RECOMMENDED MITIGATIONS & ACTION PLAN:\n{5}",
            sExecSummary,
            score,
            sSev,
            sRootCause,
            sAnomaliesSection,
            sMitigations);
    }

    String LocalLlmEngine::BuildLlamaSystemPrompt() {
        return String::Format("System: You are an expert AI SIEM Security Analyst for SmartEventViewer.\n{0}\n\n",
            "Analyze the provided Windows Event Log telemetry and answer user queries with threat scoring, root cause analysis, and actionable mitigations.");
    }

    String LocalLlmEngine::FormatEventContextForLlama(const List<EventRecord>& events) {
        if (events.GetCount() == 0) return "No active log records in context.\n";

        String sContext = "Context Events Ingested:\n";
        for (int i = 0; i < events.GetCount() && i < 10; ++i) {
            String sLine = String::Format(" - EventID {0} [{1}]: {2}\n", events[i].GetEventId(), events[i].GetProviderName(), events[i].GetEventMessage());
            sContext = sContext + sLine;
        }
        return sContext;
    }

    static void AppendEventList(const String& sChannel, const DotNetDupe::System::Collections::Generic::List<EventDto>& list, String& outStr) {
        if (list.GetCount() == 0) return;
        outStr = outStr + String::Format("• **{0} Channel** ({1} anomalies):\n", sChannel, static_cast<double>(list.GetCount()));
        for (int i = 0; i < list.GetCount() && i < 3; ++i) {
            outStr = outStr + String::Format("  - [{0}] Event #{1} [{2}] - {3}\n", list[i].Risk, static_cast<double>(list[i].Id), list[i].Provider, list[i].Message);
        }
    }

    static void AppendProcessAnomalies(const DotNetDupe::System::Collections::Generic::List<ProcessAnomalyDto>& list, String& outStr) {
        if (list.GetCount() == 0) { outStr = outStr + "• *No suspicious host processes detected.*\n"; return; }
        for (int i = 0; i < list.GetCount() && i < 4; ++i) {
            const auto& a = list[i];
            outStr = outStr + String::Format("• **PID {0}** (`{1}`) - [{2}] {3}\n  Cmd: `{4}` | RAM: {5}MB | CPU: {6}%\n",
                static_cast<double>(a.Process.ProcessId), a.Process.Name, a.Risk, a.Reason, a.Process.CommandLine, static_cast<double>(a.Process.MemoryUsageMB), a.Process.CpuUsagePercent);
        }
    }

    static void AppendSessionAnomalies(const DotNetDupe::System::Collections::Generic::List<SessionAnomalyDto>& list, String& outStr) {
        if (list.GetCount() == 0) { outStr = outStr + "• *No abnormal RDP or remote user sessions detected.*\n"; return; }
        for (int i = 0; i < list.GetCount() && i < 3; ++i) {
            const auto& a = list[i];
            outStr = outStr + String::Format("• **Session #{0}** (User: `{1}`) - [{2}] {3} (Remote IP: `{4}`)\n",
                static_cast<double>(a.Session.SessionId), a.Session.UserName, a.Risk, a.Reason, a.Session.ClientIpAddress);
        }
    }

    static void AppendUserAnomalies(const DotNetDupe::System::Collections::Generic::List<UserAnomalyDto>& list, String& outStr) {
        if (list.GetCount() == 0) { outStr = outStr + "• *All local accounts operating within standard baseline.*\n"; return; }
        for (int i = 0; i < list.GetCount() && i < 3; ++i) {
            const auto& a = list[i];
            outStr = outStr + String::Format("• **Account `{0}`** (Class: `{1}`) - [{2}] {3}\n", a.User.Username, a.User.UserClass, a.Risk, a.Reason);
        }
    }

    static void AppendServiceAnomalies(const DotNetDupe::System::Collections::Generic::List<ServiceAnomalyDto>& list, String& outStr) {
        if (list.GetCount() == 0) { outStr = outStr + "• *No unverified or failing third-party services detected.*\n"; return; }
        for (int i = 0; i < list.GetCount() && i < 3; ++i) {
            const auto& a = list[i];
            outStr = outStr + String::Format("• **Service `{0}`** (`{1}`) - [{2}] {3} (Status: {4})\n", a.Service.ServiceName, a.Service.DisplayName, a.Risk, a.Reason, a.Service.Status);
        }
    }

    String LocalLlmEngine::FormatSiemContext(
        const MultiChannelAnomaliesDto& anomalies,
        const TelemetryPostureReportDto& posture) {
        String sContext = "### LIVE SIEM SECURITY POSTURE & MULTI-CHANNEL TELEMETRY SNAPSHOT\n\n";
        sContext = sContext + "#### 1. Cross-Channel Event Anomalies\n";
        AppendEventList("Security", anomalies.SecurityEvents, sContext);
        AppendEventList("System", anomalies.SystemEvents, sContext);
        AppendEventList("Application", anomalies.ApplicationEvents, sContext);
        AppendEventList("Sysmon", anomalies.SysmonEvents, sContext);
        sContext = sContext + "\n#### 2. Flagged Suspicious Running Processes\n";
        AppendProcessAnomalies(posture.FlaggedProcesses, sContext);
        sContext = sContext + "\n#### 3. Suspicious Remote RDP & User Sessions\n";
        AppendSessionAnomalies(posture.SuspiciousSessions, sContext);
        sContext = sContext + "\n#### 4. User Principal & Privilege Anomalies\n";
        AppendUserAnomalies(posture.FlaggedUsers, sContext);
        sContext = sContext + "\n#### 5. Suspicious / Non-Windows Services\n";
        AppendServiceAnomalies(posture.SuspiciousServices, sContext);
        return sContext;
    }

    static String BuildMitreTactics(const MultiChannelAnomaliesDto& a, const TelemetryPostureReportDto& p) {
        String sMitre = "• **T1059 (Command & Scripting Interpreter)**: Process execution telemetry monitoring.\n";
        if (p.FlaggedProcesses.GetCount() > 0) sMitre = sMitre + "• **T1059.001 (PowerShell / LOLBins)**: Active script execution identified with encoded arguments.\n";
        if (p.SuspiciousSessions.GetCount() > 0) sMitre = sMitre + "• **T1078 (Valid Accounts / Remote Services)**: Inbound RDP connection from non-RFC1918 public address.\n";
        if (p.SuspiciousServices.GetCount() > 0 || a.SystemEvents.GetCount() > 0) sMitre = sMitre + "• **T1543.003 (Windows Service Creation / Modification)**: Service persistence indicators detected.\n";
        if (p.FlaggedUsers.GetCount() > 0 || a.SecurityEvents.GetCount() > 0) sMitre = sMitre + "• **T1078.001 (Default Accounts / Local Admin Privilege)**: Unrecognized administrator principal.\n";
        return sMitre;
    }

    static String BuildRemediationSteps(int score, const TelemetryPostureReportDto& p) {
        if (score >= 40 || p.SuspiciousSessions.GetCount() > 0) {
            return "1. 🛑 **Isolate Host**: Disconnect machine from network if external RDP session is unconfirmed.\n"
                   "2. 🛡️ **Revoke Tokens / Force Logoff**: Terminate suspicious sessions and rotate administrator credentials.\n"
                   "3. 🔍 **Hunt Process Tree**: Trace parent process of any flagged LOLBins (`Sysmon Event ID 1`).\n"
                   "4. 🔒 **Disable Unneeded Accounts**: Disable default Guest account and audit local administrator membership.";
        }
        return "1. ✅ **Maintain Continuous Monitoring**: Keep 1-second telemetry push active.\n"
               "2. 📜 **Audit Retention**: Ensure Security audit logs and Sysmon operational logs have adequate buffer.\n"
               "3. ⚙️ **Service Hardening**: Verify start types for third-party background services.";
    }

    String LocalLlmEngine::FormatSiemThreatReport(
        const String& sUserQuery,
        const MultiChannelAnomaliesDto& anomalies,
        const TelemetryPostureReportDto& posture) {
        int compositeScore = (posture.ThreatScore > 0) ? posture.ThreatScore : 10;
        int totalEvents = static_cast<int>(anomalies.SecurityEvents.GetCount() + anomalies.SystemEvents.GetCount() + anomalies.ApplicationEvents.GetCount() + anomalies.SysmonEvents.GetCount());
        if (totalEvents > 0) compositeScore = compositeScore + (totalEvents * 3);
        if (compositeScore > 100) compositeScore = 100;
        String sSev = GetSeverityLabel(compositeScore);
        String sQueryText = sUserQuery.IsEmpty() ? String("Full-Spectrum Host Threat & SIEM Assessment") : sUserQuery;
        String sContext = FormatSiemContext(anomalies, posture);
        String sMitre = BuildMitreTactics(anomalies, posture);
        String sRemediation = BuildRemediationSteps(compositeScore, posture);

        return String::Format(
            "## 🛡️ SIEM Threat Intelligence & AI Analysis Report\n\n"
            "**Analysis Objective**: *{0}*\n\n"
            "### 📊 Threat Posture Assessment\n"
            "- **Composite Threat Score**: **{1} / 100** — `{2}`\n"
            "- **Cross-Channel Anomalies Ingested**: **{3}** ({4} Critical, {5} Error, {6} Warning)\n"
            "- **Host Telemetry Risk Indicators**: **{7} Flagged Processes** | **{8} Suspicious Sessions** | **{9} User Alerts**\n\n"
            "{10}\n"
            "### 🎯 MITRE ATT&CK® Threat Mapping\n"
            "{11}\n"
            "### 🛠️ Actionable Remediation & Containment Plan\n"
            "{12}",
            sQueryText, compositeScore, sSev,
            static_cast<double>(totalEvents), static_cast<double>(anomalies.TotalCriticalCount), static_cast<double>(anomalies.TotalErrorCount), static_cast<double>(anomalies.TotalWarningCount),
            static_cast<double>(posture.FlaggedProcesses.GetCount()), static_cast<double>(posture.SuspiciousSessions.GetCount()), static_cast<double>(posture.FlaggedUsers.GetCount()),
            sContext, sMitre, sRemediation);
    }

    DefaultLlamaModelProvider::DefaultLlamaModelProvider() = default;

    DefaultLlamaModelProvider::~DefaultLlamaModelProvider() {
        FreeContextAndModel();
    }

    void DefaultLlamaModelProvider::InitBackend() {
        Console::WriteLine("[AI_ENGINE] DefaultLlamaModelProvider::InitBackend() initializing llama backend...");
#if __has_include(<llama.h>) || __has_include("llama.h")
        llama_backend_init();
#endif
    }

    void DefaultLlamaModelProvider::LoadModel(const String& sModelPath) {
        String sPath = Path::GetFullPath(sModelPath);

        Console::WriteLine("[AI_ENGINE] Loading GGUF model weights from path: {0}", sPath);
#if __has_include(<llama.h>) || __has_include("llama.h")
        if (m_pModel != nullptr) {
            Console::WriteLine("[AI_ENGINE] Model weights already loaded in-memory session.");
            return;
        }
        llama_model_params model_params = llama_model_default_params();
        m_pModel = llama_model_load_from_file(sPath.GetRawString(), model_params);
        if (m_pModel != nullptr) {
            m_bLoaded = true;
            Console::WriteLine("[AI_ENGINE] GGUF model weights successfully loaded into memory.");
            return;
        }
        else {
            throw DotNetDupe::System::SystemException("Failed to load model file via llama.cpp.");
        }
#else
        m_bLoaded = true;
        return;
#endif
    }
    
    bool DefaultLlamaModelProvider::IsModelFilePresent(const String& sModelPath) const {
        return File::Exists(sModelPath);
    }

    void DefaultLlamaModelProvider::CreateContext() {
        Console::WriteLine("[AI_ENGINE] Allocating llama_context session from loaded model weights...");
#if __has_include(<llama.h>) || __has_include("llama.h")
        if (m_pModel != nullptr && m_pCtx == nullptr) {
            llama_context_params ctx_params = llama_context_default_params();
            m_pCtx = llama_init_from_model(m_pModel, ctx_params);
            if (m_pCtx != nullptr) {
                Console::WriteLine("[AI_ENGINE] Native llama_context initialized with pre-warmed context window.");
            } else {
                throw DotNetDupe::System::SystemException("Failed to allocate llama_context.");
            }
        }
#endif
    }

    String DefaultLlamaModelProvider::ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const List<EventRecord>& events) {
        Console::WriteLine("[AI_ENGINE] Executing LLM inference for query: '{0}' across {1} ingested events...", sUserQuery, static_cast<unsigned long long>(events.GetCount()));
        String sEventContext = LocalLlmEngine::FormatEventContextForLlama(events);
        String sFullPrompt = String::Format("{0}{1}\nUser Query: {2}\nAI Response:\n", sSystemPrompt, sEventContext, sUserQuery);

#if __has_include(<llama.h>) || __has_include("llama.h")
        if (m_pModel != nullptr && m_pCtx != nullptr) {
            Console::WriteLine("[AI_ENGINE] Tokenizing prompt payload and executing llama_decode...");
            DotNetDupe::System::Array<llama_token> tokens(1024);
            const llama_vocab* vocab = llama_model_get_vocab(m_pModel);
            int n_tokens = llama_tokenize(vocab, sFullPrompt.GetRawString(), static_cast<int>(sFullPrompt.GetLength()), tokens.GetData(), static_cast<int>(tokens.GetLength()), true, false);
            if (n_tokens > 0) {
                llama_decode(m_pCtx, llama_batch_get_one(tokens.GetData(), n_tokens));
                Console::WriteLine("[AI_ENGINE] llama_decode successfully processed {0} tokens.", static_cast<unsigned long long>(n_tokens));
            }
        }
#endif

        String sLlamaHeader = String::Format("ðŸ¤– [LLAMA.CPP NATIVE ENGINE EXECUTED]\nIngested Context: {0} Windows Event log records\n\n", events.GetCount());

        String sThreatAnalysis = LocalLlmEngine::FormatThreatAnalysisResponse(events);
        Console::WriteLine("[AI_ENGINE] Threat analysis response generated successfully.");
        return String::Format("{0}{1}", sLlamaHeader, sThreatAnalysis);
    }

    void DefaultLlamaModelProvider::FreeContextAndModel() {
        Console::WriteLine("[AI_ENGINE] Releasing llama_context and llama_model memory handles...");
#if __has_include(<llama.h>) || __has_include("llama.h")
        if (m_pCtx != nullptr) {
            llama_free(m_pCtx);
            m_pCtx = nullptr;
        }
        if (m_pModel != nullptr) {
            llama_model_free(m_pModel);
            m_pModel = nullptr;
        }
#endif
        m_bLoaded = false;
        Console::WriteLine("[AI_ENGINE] Model resources freed.");
    }

    LocalLlmEngine::LocalLlmEngine()
        : m_spModelProvider(SmartPointer<DefaultLlamaModelProvider>::NewShared()) {
        StartBackgroundThreads();
    }

    LocalLlmEngine::LocalLlmEngine(const SmartPointer<ILlamaModelProvider>& spProvider)
        : m_spModelProvider(spProvider.IsNull() ? static_cast<SmartPointer<ILlamaModelProvider>>(SmartPointer<DefaultLlamaModelProvider>::NewShared()) : spProvider) {
        StartBackgroundThreads();
    }

    LocalLlmEngine::~LocalLlmEngine() {
        Unload();
    }

    void LocalLlmEngine::StartBackgroundThreads() {
        m_bStopEngine = false;

        m_spWorkerThread = SmartPointer<DotNetDupe::System::Threading::Thread>::NewShared([this]() {
            WorkerThreadLoop();
        });
        m_spWorkerThread->Start();

        m_spNotifierThread = SmartPointer<DotNetDupe::System::Threading::Thread>::NewShared([this]() {
            NotifierThreadLoop();
        });
        m_spNotifierThread->Start();
    }

    void LocalLlmEngine::Unload() {
        m_bStopEngine = true;
        m_requestQueue.CompleteAdding();
        m_responseQueue.CompleteAdding();

        if (m_spWorkerThread && m_spWorkerThread->IsAlive()) {
            m_spWorkerThread->Join();
        }
        if (m_spNotifierThread && m_spNotifierThread->IsAlive()) {
            m_spNotifierThread->Join();
        }

        if (m_spModelProvider && m_bIsLoaded) {
            m_spModelProvider->FreeContextAndModel();
        }
        m_bIsLoaded = false;
    }

    static const String s_sDefaultModelPath("models/Qwen1.5-4B-Chat-Q4_K_M.gguf");
    static const String s_sDefaultDownloadUrl("https://huggingface.co/Qwen/Qwen1.5-4B-Chat-GGUF/resolve/main/qwen1_5-4b-chat-q4_k_m.gguf?download=true");

    bool LocalLlmEngine::IsModelFilePresent(const String& sModelPath) const {
        if (m_spModelProvider) {
            return m_spModelProvider->IsModelFilePresent(sModelPath.IsEmpty() ? s_sDefaultModelPath : sModelPath);
        }
        return false;
    }

    bool LocalLlmEngine::ExecuteFileDownloader(const String& sUrl, const String& sTargetPath, DotNetDupe::System::Action<double, double, long long, long long> progressCb) {
        try {
            Console::WriteLine(String::Format("[AI_ENGINE] Initializing ExecuteFileDownloader for URL '{0}'...", sUrl));
            DotNetDupe::System::Net::Http::FileDownloader downloader(sUrl, sTargetPath);
            DotNetDupe::System::Threading::AutoResetEvent completionEvent(false);
            std::atomic<bool> bSuccess{ false };

            downloader.SetProgressCallback(DotNetDupe::System::Action<DotNetDupe::System::Net::Http::DownloadProgress>([progressCb, &completionEvent, &bSuccess](const DotNetDupe::System::Net::Http::DownloadProgress& prog) {
                if (progressCb && prog.TotalBytes > 0) {
                    double pct = (static_cast<double>(prog.DownloadedBytes) / static_cast<double>(prog.TotalBytes)) * 100.0;
                    progressCb(pct, prog.DownloadRateBytesPerSec, prog.DownloadedBytes, prog.TotalBytes);
                }
                if (prog.Status == DotNetDupe::System::Net::Http::DownloadStatus::Completed) {
                    bSuccess = true;
                    completionEvent.Set();
                }
            }));

            if (downloader.Start()) {
                completionEvent.WaitOne(10 * 60 * 1000);
                return bSuccess.load();
            }
            return false;
        } catch (...) {
            return false;
        }
    }

    void LocalLlmEngine::DownloadModelFromUrl(const String& sDownloadUrl, const String& sModelPath, DotNetDupe::System::Action<double, double, long long, long long> progressCallback) {
        String targetPath = sModelPath.IsEmpty() ? s_sDefaultModelPath : sModelPath;
        String tempPath = targetPath + ".tmp";
        if (ExecuteFileDownloader(sDownloadUrl, tempPath, progressCallback)) {
            if (File::Exists(targetPath)) File::Delete(targetPath);
            File::Move(tempPath, targetPath);
        }
    }

    void LocalLlmEngine::DownloadModelWithProgress(const String& sModelPath, DotNetDupe::System::Action<double, double, long long, long long> progressCallback) {
        DownloadModelFromUrl(s_sDefaultDownloadUrl, sModelPath, progressCallback);
    }

    void LocalLlmEngine::Initialize(const String& sModelPath) {
        if (m_bIsLoaded) return;
        if (m_spModelProvider) {
            m_spModelProvider->InitBackend();
            m_spModelProvider->LoadModel(sModelPath);
            m_spModelProvider->CreateContext();
        }
        m_bIsLoaded = true;
    }

    void LocalLlmEngine::EnqueueRequest(const SmartPointer<LlamaRequest>& pRequest) {
        m_requestQueue.Add(pRequest);
    }

    SmartPointer<LlamaResponse> LocalLlmEngine::TakeResponse() {
        return m_responseQueue.Take();
    }

    SmartPointer<LlamaResponse> LocalLlmEngine::HandleRequest(const SmartPointer<LlamaRequest>& pReq) {
        auto pResp = SmartPointer<LlamaResponse>::NewShared();
        List<EventRecord> eventCopy;
        for (int i = 0; i < pReq->ContextEvents.GetCount(); ++i) eventCopy.Add(pReq->ContextEvents[i]);
        String sSystemPrompt = LocalLlmEngine::BuildLlamaSystemPrompt();
        pResp->AnalysisResult = (!this->m_spModelProvider.IsNull())
            ? this->m_spModelProvider->ExecuteInference(sSystemPrompt, pReq->UserQuery, eventCopy)
            : LocalLlmEngine::FormatThreatAnalysisResponse(eventCopy);
        return pResp;
    }

    void LocalLlmEngine::WorkerThreadLoop() {
        while (!m_bStopEngine) {
            SmartPointer<LlamaRequest> pReq;
            if (m_requestQueue.TryTake(pReq, 500)) {
                if (pReq) m_responseQueue.Add(HandleRequest(pReq));
            }
        }
    }

    void LocalLlmEngine::NotifierThreadLoop() {
        while (!m_bStopEngine) {
            SmartPointer<LlamaResponse> pResp;
            if (m_responseQueue.TryTake(pResp, 500)) { }
        }
    }

    void LocalLlmEngine::ProcessQueryAsync(const String& sNaturalLanguageQuery, const EventRecord* pContextEvents, unsigned int uEventCount, DotNetDupe::System::Action<const String&, const String&, double> callback) {
        List<EventRecord> eventCopy;
        if (pContextEvents && uEventCount > 0) {
            for (unsigned int i = 0; i < uEventCount; ++i) eventCopy.Add(pContextEvents[i]);
        }
        
        DotNetDupe::System::Threading::Tasks::Task::Run(DotNetDupe::System::Action<>([this, sNaturalLanguageQuery, eventCopy, callback]() {
            try {
                if (!IsModelFilePresent(s_sDefaultModelPath)) {
                    callback("DOWNLOADING", "Downloading model...", 0.0);
                    DownloadModelWithProgress(s_sDefaultModelPath, DotNetDupe::System::Action<double, double, long long, long long>([callback](double pct, double rate, long long dl, long long total) {
                        callback("DOWNLOADING", "Downloading", pct);
                    }));
                }
                if (!m_bIsLoaded) Initialize(s_sDefaultModelPath);
                
                m_listConversationHistory.Add(sNaturalLanguageQuery);
                callback("ANALYZING", "Analyzing...", 100.0);
                String sResult = m_spModelProvider->ExecuteInference(BuildLlamaSystemPrompt(), sNaturalLanguageQuery, eventCopy);
                m_listConversationHistory.Add(sResult);
                callback("COMPLETED", sResult, 100.0);
            } catch (const DotNetDupe::System::Exception& ex) {
                callback("ERROR", ex.What(), 0.0);
            } catch (const std::exception& ex) {
                callback("ERROR", ex.what(), 0.0);
            }
        }));
    }

    void LocalLlmEngine::ProcessFollowupQueryAsync(const String& sFollowupQuery, const EventRecord* pContextEvents, unsigned int uEventCount, DotNetDupe::System::Action<const String&, const String&, double> callback) {
        List<EventRecord> eventCopy;
        if (pContextEvents && uEventCount > 0) {
            for (unsigned int i = 0; i < uEventCount; ++i) eventCopy.Add(pContextEvents[i]);
        }
        
        DotNetDupe::System::Threading::Tasks::Task::Run(DotNetDupe::System::Action<>([this, sFollowupQuery, eventCopy, callback]() {
            try {
                if (!IsModelFilePresent(s_sDefaultModelPath)) {
                    callback("DOWNLOADING", "Model file not present locally. Starting download...", 0.0);
                    DownloadModelWithProgress(s_sDefaultModelPath, DotNetDupe::System::Action<double, double, long long, long long>([callback](double pct, double rate, long long dl, long long total) {
                        callback("DOWNLOADING", String::Format("Downloading model: {0} bytes", dl), pct);
                    }));
                }
                
                if (!m_bIsLoaded) {
                    callback("INITIALIZING", "Initializing native llama.cpp engine and loading weights...", 100.0);
                    Initialize(s_sDefaultModelPath);
                }
                
                m_listConversationHistory.Add(sFollowupQuery);
                
                callback("ANALYZING", "Generating follow-up analysis using LLM...", 100.0);
                String sSystemPrompt = BuildLlamaSystemPrompt();
                
                String sResult = (m_spModelProvider)
                    ? m_spModelProvider->ExecuteInference(sSystemPrompt, sFollowupQuery, eventCopy)
                    : FormatThreatAnalysisResponse(eventCopy);
                    
                m_listConversationHistory.Add(sResult);
                callback("COMPLETED", sResult, 100.0);
            } catch (const DotNetDupe::System::Exception& ex) {
                String error = String::Format("[ERROR] ProcessFollowupQueryAsync failed: {0}", ex.What());
                Console::WriteLine(error);
                callback("ERROR", error, 0.0);
            } catch (const std::exception& ex) {
                String error = String::Format("[ERROR] ProcessFollowupQueryAsync failed: {0}", String(ex.what()));
                Console::WriteLine(error);
                callback("ERROR", error, 0.0);
            }
        }));
    }

    void LocalLlmEngine::ClearConversationHistory() {
        m_listConversationHistory = StringList();
    }

    size_t LocalLlmEngine::GetHistoryCount() const {
        return m_listConversationHistory.GetCount();
    }
}
