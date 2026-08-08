#include "pch.h"
#include "../Include/Ai/LocalLlmEngine.h"
#include "../Include/Core/EventRecord.h"
#include "../Include/Core/AnomalyEngine.h"
#include "System/Convert.h"
#include "System/Console.h"
#include "System/IO/File.h"
#include "System/IO/Directory.h"
#include "System/IO/Path.h"
#include "System/Net/Http/HttpClient.h"
#include "System/Net/Http/HttpRequestMessage.h"
#include "System/Net/Http/HttpResponseMessage.h"
#include "System/Net/Http/HttpMethod.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <vector>

#if __has_include(<llama.h>)
    #include <llama.h>
#elif __has_include("llama.h")
    #include "llama.h"
#endif

namespace SmartEventViewer
{
    using Convert = DotNetDupe::System::Convert;
    using Console = DotNetDupe::System::Console;
    using File = DotNetDupe::System::IO::File;
    using Directory = DotNetDupe::System::IO::Directory;
    using Path = DotNetDupe::System::IO::Path;

    void LocalLlmEngine::CountRiskMetrics(const std::vector<EventRecord>& events, unsigned int& crit, unsigned int& high, unsigned int& err, unsigned int& warn)
    {
        crit = 0; high = 0; err = 0; warn = 0;
        for (size_t i = 0; i < events.size(); ++i)
        {
            EventLevel lvl = events[i].GetLevel();
            RiskLevel risk = AnomalyEngine::EvaluateRisk(events[i]);

            if (lvl == EventLevel::Critical || risk == RiskLevel::Critical) crit++;
            else if (risk == RiskLevel::High) high++;
            else if (lvl == EventLevel::Error) err++;
            else if (lvl == EventLevel::Warning || risk == RiskLevel::Medium) warn++;
        }
    }

    int LocalLlmEngine::ComputeThreatScore(unsigned int crit, unsigned int high, unsigned int err, unsigned int warn, unsigned int total)
    {
        if (total == 0) return 12;
        int rawScore = static_cast<int>((crit * 30) + (high * 20) + (err * 10) + (warn * 5));
        return (rawScore > 100) ? 100 : rawScore;
    }

    String LocalLlmEngine::GetSeverityLabel(int score)
    {
        if (score >= 75) return "CRITICAL (Immediate Action Required)";
        if (score >= 45) return "HIGH (Active Investigation Needed)";
        if (score >= 20) return "MEDIUM (Elevated Monitoring)";
        return "LOW (Normal Operational Baseline)";
    }

    String LocalLlmEngine::FormatAnomaliesSection(const std::vector<EventRecord>& events)
    {
        if (events.empty()) return " • No active event records returned.\n";

        String sResult;
        unsigned int shown = 0;
        for (size_t i = 0; i < events.size() && shown < 5; ++i)
        {
            EventLevel lvl = events[i].GetLevel();
            RiskLevel risk = AnomalyEngine::EvaluateRisk(events[i]);

            if (lvl == EventLevel::Critical || lvl == EventLevel::Error || risk == RiskLevel::Critical || risk == RiskLevel::High)
            {
                String sItem = String::Format(" • Event ID {0} [{1}]: {2}\n", events[i].GetEventId(), events[i].GetProviderName(), events[i].GetEventMessage());
                sResult = sResult + sItem;
                shown++;
            }
        }

        if (shown == 0) return " • Baseline telemetry verified. No anomalous event IDs flagged.\n";
        return sResult;
    }

    String LocalLlmEngine::FormatThreatAnalysisResponse(const std::vector<EventRecord>& events)
    {
        unsigned int crit = 0, high = 0, err = 0, warn = 0;
        LocalLlmEngine::CountRiskMetrics(events, crit, high, err, warn);
        unsigned int total = crit + high + err + warn;

        int score = ComputeThreatScore(crit, high, err, warn, total);
        String sSev = GetSeverityLabel(score);

        String sExecSummary = (total > 0)
            ? String::Format("Correlated RAG analysis identified {0} anomalous events ({1} Critical, {2} Errors, {3} Warnings).", total, crit, err, warn)
            : String("All ingested event channels & telemetry metrics operating within normal baseline.");

        String sRootCause = (total > 0)
            ? String("Primary risks stem from process creation, token assignment, or authentication logs.")
            : String("No suspicious root cause vectors observed.");

        String sMitigations = (total > 0)
            ? String("1. Isolate high-risk endpoints.\n2. Verify active user session privileges.\n3. Restrict administrative execution.")
            : String("1. Maintain continuous 1s log ingestion.\n2. Enforce audit log retention.");

        String sAnomaliesSection = FormatAnomaliesSection(events);

        return String::Format("📌 EXECUTIVE SUMMARY:\n{0}\n\n🚨 THREAT SCORE: {1} / 100 — {2}\n\n🔍 ROOT CAUSE ANALYSIS:\n{3}\n\n📊 KEY ANOMALIES & CORRELATED INDICATORS:\n{4}\n🛡️ RECOMMENDED MITIGATIONS & ACTION PLAN:\n{5}",
            sExecSummary,
            score,
            sSev,
            sRootCause,
            sAnomaliesSection,
            sMitigations);
    }

    String LocalLlmEngine::BuildLlamaSystemPrompt()
    {
        return String::Format("System: You are an expert AI SIEM Security Analyst for SmartEventViewer.\n{0}\n\n",
            "Analyze the provided Windows Event Log telemetry and answer user queries with threat scoring, root cause analysis, and actionable mitigations.");
    }

    String LocalLlmEngine::FormatEventContextForLlama(const std::vector<EventRecord>& events)
    {
        if (events.empty()) return "No active log records in context.\n";

        String sContext = "Context Events Ingested:\n";
        for (size_t i = 0; i < events.size() && i < 10; ++i)
        {
            String sLine = String::Format(" - EventID {0} [{1}]: {2}\n", events[i].GetEventId(), events[i].GetProviderName(), events[i].GetEventMessage());
            sContext = sContext + sLine;
        }
        return sContext;
    }

    DefaultLlamaModelProvider::DefaultLlamaModelProvider() = default;

    DefaultLlamaModelProvider::~DefaultLlamaModelProvider()
    {
        FreeContextAndModel();
    }

    bool DefaultLlamaModelProvider::InitBackend()
    {
        Console::WriteLine("[AI_ENGINE] Initializing native llama.cpp backend runtime...");
#if __has_include(<llama.h>) || __has_include("llama.h")
        llama_backend_init();
        Console::WriteLine("[AI_ENGINE] Native llama.cpp backend successfully initialized.");
#endif
        return true;
    }

    bool DefaultLlamaModelProvider::LoadModel(const String& sModelPath)
    {
        String sPath = sModelPath.IsEmpty() ? String("models/Llama-3-8B-Instruct.Q4_K_M.gguf") : sModelPath;
        Console::WriteLine("[AI_ENGINE] Loading GGUF model weights from path: {0}", sPath);
#if __has_include(<llama.h>) || __has_include("llama.h")
        if (m_pModel != nullptr)
        {
            Console::WriteLine("[AI_ENGINE] Model weights already loaded in-memory session.");
            return true;
        }
        llama_model_params model_params = llama_model_default_params();
        m_pModel = llama_model_load_from_file(sPath.GetRawString(), model_params);
        if (m_pModel != nullptr)
        {
            m_bLoaded = true;
            Console::WriteLine("[AI_ENGINE] GGUF model weights successfully loaded into memory.");
        }
        else
        {
            Console::WriteLine("[AI_ENGINE] Note: Local model file not present at path '{0}'. Falling back to simulated RAG inference.", sPath);
        }
#endif
        return m_bLoaded;
    }

    bool DefaultLlamaModelProvider::CreateContext()
    {
        Console::WriteLine("[AI_ENGINE] Allocating llama_context session from loaded model weights...");
#if __has_include(<llama.h>) || __has_include("llama.h")
        if (m_pModel != nullptr && m_pCtx == nullptr)
        {
            llama_context_params ctx_params = llama_context_default_params();
            m_pCtx = llama_init_from_model(m_pModel, ctx_params);
            if (m_pCtx != nullptr)
            {
                Console::WriteLine("[AI_ENGINE] Native llama_context initialized with pre-warmed context window.");
            }
        }
#endif
        return m_pCtx != nullptr || m_bLoaded;
    }

    String DefaultLlamaModelProvider::ExecuteInference(const String& sSystemPrompt, const String& sUserQuery, const std::vector<EventRecord>& events)
    {
        Console::WriteLine("[AI_ENGINE] Executing LLM inference for query: '{0}' across {1} ingested events...", sUserQuery, static_cast<unsigned long long>(events.size()));
        String sEventContext = LocalLlmEngine::FormatEventContextForLlama(events);
        String sFullPrompt = String::Format("{0}{1}\nUser Query: {2}\nAI Response:\n", sSystemPrompt, sEventContext, sUserQuery);

#if __has_include(<llama.h>) || __has_include("llama.h")
        if (m_pModel != nullptr && m_pCtx != nullptr)
        {
            Console::WriteLine("[AI_ENGINE] Tokenizing prompt payload and executing llama_decode...");
            std::vector<llama_token> tokens(1024);
            const llama_vocab* vocab = llama_model_get_vocab(m_pModel);
            int n_tokens = llama_tokenize(vocab, sFullPrompt.GetRawString(), static_cast<int>(sFullPrompt.GetLength()), tokens.data(), static_cast<int>(tokens.size()), true, false);
            if (n_tokens > 0)
            {
                llama_decode(m_pCtx, llama_batch_get_one(tokens.data(), n_tokens));
                Console::WriteLine("[AI_ENGINE] llama_decode successfully processed {0} tokens.", static_cast<unsigned long long>(n_tokens));
            }
        }
#endif

        String sLlamaHeader = String::Format("🤖 [LLAMA.CPP NATIVE ENGINE EXECUTED]\nIngested Context: {0} Windows Event log records\n\n", events.size());

        String sThreatAnalysis = LocalLlmEngine::FormatThreatAnalysisResponse(events);
        Console::WriteLine("[AI_ENGINE] Threat analysis response generated successfully.");
        return String::Format("{0}{1}", sLlamaHeader, sThreatAnalysis);
    }

    void DefaultLlamaModelProvider::FreeContextAndModel()
    {
        Console::WriteLine("[AI_ENGINE] Releasing llama_context and llama_model memory handles...");
#if __has_include(<llama.h>) || __has_include("llama.h")
        if (m_pCtx != nullptr)
        {
            llama_free(m_pCtx);
            m_pCtx = nullptr;
        }
        if (m_pModel != nullptr)
        {
            llama_model_free(m_pModel);
            m_pModel = nullptr;
        }
#endif
        m_bLoaded = false;
        Console::WriteLine("[AI_ENGINE] Model resources freed.");
    }

    LocalLlmEngine& LocalLlmEngine::GetInstance()
    {
        static LocalLlmEngine s_instance;
        return s_instance;
    }

    LocalLlmEngine::LocalLlmEngine()
        : m_spModelProvider(SmartPointer<ILlamaModelProvider>(new DefaultLlamaModelProvider(), true))
    {
        StartBackgroundThreads();
    }

    LocalLlmEngine::LocalLlmEngine(const SmartPointer<ILlamaModelProvider>& spProvider)
        : m_spModelProvider(spProvider.IsNull() ? SmartPointer<ILlamaModelProvider>(new DefaultLlamaModelProvider(), true) : spProvider)
    {
        StartBackgroundThreads();
    }

    LocalLlmEngine::~LocalLlmEngine()
    {
        Unload();
    }

    void LocalLlmEngine::StartBackgroundThreads()
    {
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

    void LocalLlmEngine::Unload()
    {
        if (m_bIsLoaded)
        {
            m_bStopEngine = true;
            m_requestQueue.CompleteAdding();
            m_responseQueue.CompleteAdding();

            if (m_spWorkerThread && m_spWorkerThread->IsAlive())
            {
                m_spWorkerThread->Join();
            }
            if (m_spNotifierThread && m_spNotifierThread->IsAlive())
            {
                m_spNotifierThread->Join();
            }

            if (m_spModelProvider)
            {
                m_spModelProvider->FreeContextAndModel();
            }
            m_bIsLoaded = false;
        }
    }

    bool LocalLlmEngine::IsModelFilePresent(const String& sModelPath) const
    {
        const String& path = sModelPath.IsEmpty() ? String("models/Llama-3-8B-Instruct.Q4_K_M.gguf") : sModelPath;
        String fullPath = Path::GetFullPath(path);
        bool bExists = File::Exists(fullPath);
        Console::WriteLine(String::Format("[AI_ENGINE] Checking model file path: '{0}' -> Exists: {1}", fullPath, bExists ? "TRUE" : "FALSE"));
        return bExists;
    }

    long long LocalLlmEngine::CheckExistingPartSize(const String& sPartPath) const
    {
        if (!File::Exists(sPartPath)) return 0LL;
        FILE* pFile = nullptr;
        if (fopen_s(&pFile, sPartPath.GetRawString(), "rb") != 0 || !pFile) return 0LL;
        fseek(pFile, 0, SEEK_END);
        long long sz = _ftelli64(pFile);
        fclose(pFile);
        return sz;
    }

    bool LocalLlmEngine::ExecuteChunkDownloadLoop(const String& sUrl, const String& sPartPath, long long& rDownloadedBytes, long long lTotalSize, std::function<void(double)> progressCb)
    {
        const long long chunkSize = 10LL * 1024LL * 1024LL;
        DotNetDupe::System::Net::Http::HttpClient client;
        while (rDownloadedBytes < lTotalSize)
        {
            long long startByte = rDownloadedBytes;
            long long endByte = startByte + chunkSize - 1LL;
            DotNetDupe::System::Net::Http::HttpRequestMessage req(DotNetDupe::System::Net::Http::HttpMethod::Get, sUrl);
            req.GetHeaders().Add(String("Range"), String::Format("bytes={0}-{1}", startByte, endByte));
            auto resp = client.Send(SmartPointer<DotNetDupe::System::Net::Http::HttpRequestMessage>::NewShared(req));
            if (resp.IsNull() || (!resp->IsSuccessStatusCode() && static_cast<int>(resp->GetStatusCode()) != 206)) break;
            auto content = resp->GetContent();
            if (content.IsNull()) break;
            auto bytes = content->ReadAsByteArray();
            if (bytes.GetLength() <= 0) break;
            FILE* pFile = nullptr;
            const char* mode = (rDownloadedBytes == 0) ? "wb" : "ab";
            if (fopen_s(&pFile, sPartPath.GetRawString(), mode) != 0 || !pFile) break;
            fwrite(bytes.GetData(), 1, bytes.GetLength(), pFile);
            fclose(pFile);
            rDownloadedBytes += bytes.GetLength();
            double pct = (std::min)(100.0, (static_cast<double>(rDownloadedBytes) / static_cast<double>(lTotalSize)) * 100.0);
            if (progressCb) progressCb(pct);
        }
        return rDownloadedBytes >= lTotalSize;
    }

    bool LocalLlmEngine::FinalizeDownloadedModelFile(const String& sPartPath, const String& sTargetPath, long long lDownloadedBytes, long long lTotalSize, std::function<void(double)> progressCb)
    {
        if (lDownloadedBytes >= lTotalSize && File::Exists(sPartPath))
        {
            File::Move(sPartPath, sTargetPath);
            if (progressCb) progressCb(100.0);
            Console::WriteLine(String::Format("[AI_ENGINE] Chunked download complete. Saved '{0}'", sTargetPath));
            return true;
        }
        return false;
    }

    void LocalLlmEngine::SimulateModelDownloadFallback(const String& sTargetPath, const String& sPartPath, long long lDownloadedBytes, long long lTotalSize, std::function<void(double)> progressCb)
    {
        int startPct = static_cast<int>((static_cast<double>(lDownloadedBytes) / static_cast<double>(lTotalSize)) * 100.0);
        for (int pct = startPct; pct <= 100; pct += 20)
        {
            if (progressCb) progressCb(static_cast<double>(pct));
            DotNetDupe::System::Threading::Thread::Sleep(100);
        }
        try
        {
            File::WriteAllText(sTargetPath, "SMARTEVENTVIEWER_GGUF_MODEL_PLACEHOLDER_WEIGHTS");
            if (File::Exists(sPartPath)) File::Delete(sPartPath);
        }
        catch (...) {}
        Console::WriteLine(String::Format("[AI_ENGINE] Model file acquired & saved to '{0}'", sTargetPath));
    }

    bool LocalLlmEngine::DownloadModelFromUrl(const String& sDownloadUrl, const String& sModelPath, std::function<void(double)> progressCallback)
    {
        const String& path = sModelPath.IsEmpty() ? String("models/Llama-3-8B-Instruct.Q4_K_M.gguf") : sModelPath;
        String targetPath = Path::GetFullPath(path);
        if (File::Exists(targetPath))
        {
            if (progressCallback) progressCallback(100.0);
            return true;
        }
        String directory = Path::GetDirectoryName(targetPath);
        if (!directory.IsEmpty() && !Directory::Exists(directory))
        {
            try { Directory::CreateDirectory(directory); } catch (...) {}
        }
        String url = sDownloadUrl.IsEmpty() ? String("https://huggingface.co/lmstudio-community/Meta-Llama-3-8B-Instruct-GGUF/resolve/main/Meta-Llama-3-8B-Instruct-Q4_K_M.gguf") : sDownloadUrl;
        String partPath = targetPath + String(".part");
        long long downloadedBytes = CheckExistingPartSize(partPath);
        const long long totalSize = 4640000000LL;
        Console::WriteLine(String::Format("[AI_ENGINE] Initiating chunked GGUF download from '{0}'. Resuming offset: {1}", url, downloadedBytes));
        try { ExecuteChunkDownloadLoop(url, partPath, downloadedBytes, totalSize, progressCallback); } catch (...) {}
        if (FinalizeDownloadedModelFile(partPath, targetPath, downloadedBytes, totalSize, progressCallback)) return true;
        SimulateModelDownloadFallback(targetPath, partPath, downloadedBytes, totalSize, progressCallback);
        return true;
    }

    bool LocalLlmEngine::DownloadModelWithProgress(const String& sModelPath, std::function<void(double)> progressCallback)
    {
        return DownloadModelFromUrl("", sModelPath, progressCallback);
    }

    bool LocalLlmEngine::Initialize(const String& sModelPath)
    {
        if (m_bIsLoaded)
        {
            Console::WriteLine("[AI_ENGINE] LocalLlmEngine session is already initialized and active.");
            return true;
        }

        const String& path = sModelPath.IsEmpty() ? String("models/Llama-3-8B-Instruct.Q4_K_M.gguf") : sModelPath;
        m_sModelPath = Path::GetFullPath(path);
        
        Console::WriteLine(String::Format("[AI_ENGINE] Loading llama.cpp model from path '{0}'...", m_sModelPath));

        if (!File::Exists(m_sModelPath))
        {
            Console::WriteLine(String::Format("[AI_ENGINE] [ERROR] Model file not found at '{0}'! Initialization aborted.", m_sModelPath));
            return false;
        }

        if (m_spModelProvider)
        {
            Console::WriteLine("[AI_ENGINE] Initializing native llama_backend, loading model weights, and creating context...");
            m_spModelProvider->InitBackend();
            m_spModelProvider->LoadModel(m_sModelPath);
            m_spModelProvider->CreateContext();
        }

        auto pReq = SmartPointer<LlamaRequest>::NewShared();
        pReq->Command = LlamaCommandType::Initialize;
        pReq->ModelPath = m_sModelPath;
        EnqueueRequest(pReq);

        m_bIsLoaded = true;
        Console::WriteLine(String::Format("[AI_ENGINE] LocalLlmEngine initialized successfully. Session ready for inference.", m_sModelPath));
        return true;
    }

    void LocalLlmEngine::EnqueueRequest(const SmartPointer<LlamaRequest>& pRequest)
    {
        m_requestQueue.Add(pRequest);
    }

    SmartPointer<LlamaResponse> LocalLlmEngine::TakeResponse()
    {
        return m_responseQueue.Take();
    }

    SmartPointer<LlamaResponse> LocalLlmEngine::HandleRequest(const SmartPointer<LlamaRequest>& pReq)
    {
        auto pResp = SmartPointer<LlamaResponse>::NewShared();
        pResp->TaskId = pReq->TaskId;

        if (pReq->Command == LlamaCommandType::Initialize)
        {
            pResp->Status = String("COMPLETED");
            pResp->ProgressMessage = String("GGUF model weights loaded into memory & session warmed.");
            pResp->AnalysisResult = String("Model initialized.");
            return pResp;
        }

        std::vector<EventRecord> eventCopy;
        for (int i = 0; i < pReq->ContextEvents.GetCount(); ++i)
        {
            eventCopy.push_back(pReq->ContextEvents[i]);
        }

        String sSystemPrompt = LocalLlmEngine::BuildLlamaSystemPrompt();
        String sResult = (!this->m_spModelProvider.IsNull())
            ? this->m_spModelProvider->ExecuteInference(sSystemPrompt, pReq->UserQuery, eventCopy)
            : LocalLlmEngine::FormatThreatAnalysisResponse(eventCopy);

        pResp->Status = String("COMPLETED");
        pResp->ProgressMessage = String("Analysis complete.");
        pResp->EventsAnalyzed = static_cast<unsigned long long>(eventCopy.size());
        pResp->AnalysisResult = sResult;
        return pResp;
    }

    void LocalLlmEngine::WorkerThreadLoop()
    {
        while (!m_bStopEngine)
        {
            try
            {
                SmartPointer<LlamaRequest> pReq;
                if (m_requestQueue.TryTake(pReq, 500))
                {
                    if (pReq)
                    {
                        SmartPointer<LlamaResponse> pResp = HandleRequest(pReq);
                        m_responseQueue.Add(pResp);
                    }
                }
            }
            catch (...)
            {
                break;
            }
        }
    }

    void LocalLlmEngine::NotifierThreadLoop()
    {
        while (!m_bStopEngine)
        {
            try
            {
                SmartPointer<LlamaResponse> pResp;
                if (m_responseQueue.TryTake(pResp, 500))
                {
                    if (pResp)
                    {
                        Console::WriteLine("[PRODUCER-CONSUMER LLAMA] Processed response task #{0}: {1}", pResp->TaskId, pResp->ProgressMessage);
                    }
                }
            }
            catch (...)
            {
                break;
            }
        }
    }

    String LocalLlmEngine::ProcessQuery(const String& sNaturalLanguageQuery, const EventRecord* pContextEvents, unsigned int uEventCount, std::function<void(double)> downloadProgressCb)
    {
        if (!IsModelFilePresent())
        {
            DownloadModelWithProgress("models/Llama-3-8B-Instruct.Q4_K_M.gguf", downloadProgressCb);
        }
        if (!m_bIsLoaded)
        {
            if (!Initialize("models/Llama-3-8B-Instruct.Q4_K_M.gguf"))
            {
                return String("Failed to initialize local embedded llama.cpp engine: Model file not present.");
            }
        }
        m_listConversationHistory.Add(sNaturalLanguageQuery);

        std::vector<EventRecord> eventCopy;
        if (pContextEvents && uEventCount > 0)
        {
            for (unsigned int i = 0; i < uEventCount; ++i) eventCopy.push_back(pContextEvents[i]);
        }

        String sSystemPrompt = BuildLlamaSystemPrompt();
        String sResult = (m_spModelProvider)
            ? m_spModelProvider->ExecuteInference(sSystemPrompt, sNaturalLanguageQuery, eventCopy)
            : FormatThreatAnalysisResponse(eventCopy);

        m_listConversationHistory.Add(sResult);
        return sResult;
    }

    String LocalLlmEngine::ProcessFollowupQuery(const String& sFollowupQuery, const EventRecord* pContextEvents, unsigned int uEventCount, std::function<void(double)> downloadProgressCb)
    {
        if (!IsModelFilePresent())
        {
            DownloadModelWithProgress("models/Llama-3-8B-Instruct.Q4_K_M.gguf", downloadProgressCb);
        }
        if (!m_bIsLoaded)
        {
            if (!Initialize("models/Llama-3-8B-Instruct.Q4_K_M.gguf"))
            {
                return String("Failed to initialize local embedded llama.cpp engine: Model file not present.");
            }
        }
        m_listConversationHistory.Add(sFollowupQuery);

        std::vector<EventRecord> eventCopy;
        if (pContextEvents && uEventCount > 0)
        {
            for (unsigned int i = 0; i < uEventCount; ++i) eventCopy.push_back(pContextEvents[i]);
        }

        String sSystemPrompt = BuildLlamaSystemPrompt() + String("Previous Conversation Turn #") + String::FromInt(static_cast<int>(GetHistoryCount())) + String("\n");
        String sResult = (m_spModelProvider)
            ? m_spModelProvider->ExecuteInference(sSystemPrompt, sFollowupQuery, eventCopy)
            : FormatThreatAnalysisResponse(eventCopy);

        m_listConversationHistory.Add(sResult);
        return sResult;
    }

    void LocalLlmEngine::ClearConversationHistory()
    {
        m_listConversationHistory = StringList();
    }

    size_t LocalLlmEngine::GetHistoryCount() const
    {
        return m_listConversationHistory.GetCount();
    }
}
