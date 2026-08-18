# Comprehensive SIEM Multi-Channel & Host Telemetry Threat Analysis Architecture Proposal

## 1. Executive Summary & Problem Analysis

### Current Limitations
1. **Isolated Single-Channel Analysis**:
   - The analysis engine evaluates events on a **single channel** (e.g. `Security` or `Application` in isolation).
   - Real-world cyber attacks (e.g., initial access, execution, persistence, privilege escalation, credential dumping, lateral movement) span multiple Windows event logs simultaneously:
     - **`Security`**: Failed logins (Event 4625), privilege escalation (4672), account creation/modification (4720, 4738), audit log clearing (1102).
     - **`System`**: New service creation (7045), abnormal driver loads, crash dump manipulation.
     - **`Application`**: Application crashes (1000/1001), injected DLL errors, script host runtime warnings.
     - **`Sysmon` (`Microsoft-Windows-Sysmon/Operational`)**: Process creation with full command lines (Event 1), network connection by binary (Event 3), driver loads (Event 7), remote thread injection (Event 8), registry tampering (Events 12–14).

2. **Absence of Host Runtime Telemetry Correlation**:
   - Event logs represent historical point-in-time entries.
   - To accurately detect and confirm active compromises, security analysts and AI models must correlate event logs with **live system runtime state**:
     - **Top / Suspicious Processes**: Unusual command lines, LOLBins (`powershell.exe`, `certutil.exe`, `mshta.exe`, `wscript.exe`), binaries executing from `\AppData\Local\Temp\` or `\Users\Public\`, processes with abnormal CPU/RAM or active network listeners.
     - **Suspicious RDP & User Sessions**: Remote connections from external/unknown public IPs, multiple concurrent sessions for a single user, lingering shadow/disconnected sessions outside business hours.
     - **Unrecognized / High-Risk User Principals**: Local administrator accounts outside baseline, enabled `Guest` account, disabled accounts with active sessions, non-standard group assignments (e.g. `Remote Desktop Users`).
     - **Suspicious / Non-Windows Services**: Third-party services running as `LocalSystem`, services executing from non-`System32` or user paths, newly registered services correlated with System Event 7045.

---

## 2. Target SIEM Architecture & Data Flow

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                        MULTI-VECTOR DATA INGESTION ENGINE                              │
├──────────────────────────────────────────┬─────────────────────────────────────────────┤
│        High-Priority Event Logs          │             Live Host Telemetry             │
│  (Critical, Error, Warning, High-Risk)   │     (DotNetDupe Kernel Telemetry APIs)      │
│  • Security Channel                      │  • ProcessStreamer (All running processes) │
│  • System Channel                        │  • ActiveUserSession (Logins & state)       │
│  • Application Channel                   │  • TerminalSession (RDP sessions & IPs)     │
│  • Microsoft-Windows-Sysmon/Operational │  • UserPrincipal (Accounts & privileges)    │
│                                          │  • SystemMetrics::GetAllServices (Services) │
└──────────────────────────────────────────┴─────────────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                   CORRELATION & ANOMALY DETECTION ENGINE (`AnomalyEngine`)             │
│  • Flags LOLBin process executions & suspicious paths                                  │
│  • Identifies external RDP IP logins & session hijacking patterns                      │
│  • Flags non-standard admin accounts & privilege anomalies                             │
│  • Detects unsigned/third-party services running as LocalSystem                        │
│  • Correlates Event 7045/4625/Sysmon with live runtime state                           │
│  • Computes Composite Threat Score (0–100) & Risk Level (Critical/High/Medium/Low)     │
└────────────────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                   STRUCTURED RAG CONTEXT BUILDER & LOCAL LLM ENGINE                    │
│  Generates Rich Markdown Security Snapshot:                                           │
│    ├── [CROSS-CHANNEL EVENT ANOMALIES]                                                 │
│    ├── [SUSPICIOUS RUNTIME PROCESSES]                                                  │
│    ├── [RDP & USER SESSION THREATS]                                                    │
│    ├── [USER PRINCIPAL & PRIVILEGE VIOLATIONS]                                         │
│    └── [SUSPICIOUS / NON-WINDOWS SERVICES]                                             │
│                                                                                        │
│  Llama 3.2 GGUF Local Inference:                                                       │
│  • MITRE ATT&CK Mapping (T1059, T1078, T1543, etc.)                                    │
│  • Root-Cause Threat Narrative & Indicators of Compromise (IoCs)                       │
│  • Concrete Remediation & Hardening Actions                                            │
└────────────────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                        UNIFIED SIEM DASHBOARD UI (REACT/TS)                            │
│  • "Full-Spectrum SIEM Threat Analysis" Button                                        │
│  • Multi-Channel Event Log Drill-Down (Security, System, App, Sysmon)                  │
│  • Real-Time Threat Badges (Flagged Processes, Suspicious RDP, Account Alerts)         │
│  • Interactive AI Security Copilot (Natural language cross-correlation queries)        │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Technical Component Designs

### A. Multi-Channel Event Log Ingestion (`EventService`)

Extend `IEventService` and `EventService` to collect high-priority (`Critical`, `Error`, `Warning`, and high-risk security audit) events across all four core channels:

```cpp
namespace SmartEventViewer {

    struct MultiChannelAnomaliesDto {
        DotNetDupe::System::Collections::Generic::List<EventDto> SecurityEvents;
        DotNetDupe::System::Collections::Generic::List<EventDto> SystemEvents;
        DotNetDupe::System::Collections::Generic::List<EventDto> ApplicationEvents;
        DotNetDupe::System::Collections::Generic::List<EventDto> SysmonEvents;
        unsigned long long TotalCriticalCount{ 0 };
        unsigned long long TotalErrorCount{ 0 };
        unsigned long long TotalWarningCount{ 0 };
    };

    class IEventService : public virtual DotNetDupe::System::Object {
    public:
        virtual ~IEventService() = default;
        // ... existing methods ...
        virtual MultiChannelAnomaliesDto GetCrossChannelAnomalies(size_t maxPerChannel = 10) = 0;
    };
}
```

**Channel Ingestion Strategy**:
- Scans `Security`, `System`, `Application`, and `Microsoft-Windows-Sysmon/Operational`.
- Filters for `Critical`, `Error`, and `Warning` levels.
- Gracefully handles environments where Sysmon is not installed without throwing errors or breaking the pipeline.

---

### B. Host Telemetry Anomaly Evaluator (`AnomalyEngine`)

Enhance `AnomalyEngine` with dedicated rule evaluators for live telemetry:

```cpp
namespace SmartEventViewer {

    struct ProcessAnomalyInfo {
        ProcessResourceDto Process;
        String Reason;
        RiskLevel Severity;
    };

    struct SessionAnomalyInfo {
        RdpSessionDto Session;
        String Reason;
        RiskLevel Severity;
    };

    struct UserAnomalyInfo {
        UserPrincipalDto User;
        String Reason;
        RiskLevel Severity;
    };

    struct ServiceAnomalyInfo {
        ServiceInfoDto Service;
        String Reason;
        RiskLevel Severity;
    };

    class AnomalyEngine : public IAnomalyEngine {
    public:
        RiskLevel EvaluateRisk(const EventRecord& eventRec) override;

        // Process anomaly detection:
        // - LOLBin detection with encoded scripts (powershell -enc, certutil -urlcache, mshta)
        // - Binaries executing from Temp, Public, Downloads, or AppData
        // - Unquoted executable paths
        // - Resource spikes (CPU > 75%, RAM > 2GB)
        static bool EvaluateProcess(const ProcessResourceDto& proc, ProcessAnomalyInfo& outAnomaly);

        // Session anomaly detection:
        // - Remote IP outside private RFC1918 subnets (10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16, 127.0.0.1)
        // - Lingering disconnected / shadow sessions
        static bool EvaluateSession(const RdpSessionDto& rdp, SessionAnomalyInfo& outAnomaly);

        // Principal anomaly detection:
        // - Enabled Guest or default disabled accounts
        // - Disabled accounts with active logins
        // - Non-standard administrator accounts
        // - Accounts in Remote Desktop Users without admin class
        static bool EvaluateUser(const UserPrincipalDto& user, UserAnomalyInfo& outAnomaly);

        // Service anomaly detection:
        // - Non-Windows service running as LocalSystem
        // - Service executable path outside System32 / Program Files
        // - Correlated with recent System Event 7045
        static bool EvaluateService(const ServiceInfoDto& service, ServiceAnomalyInfo& outAnomaly);
    };
}
```

---

### C. Unified SIEM Security Context for Local LLM Engine (`LocalLlmEngine` & `AnalysisService`)

When executing an AI threat analysis query, `AnalysisService` gathers:
1. Cross-Channel High-Priority Events from `EventService`.
2. Live System Telemetry Snapshot from `TelemetryService`.
3. Telemetry Anomalies flagged by `AnomalyEngine`.

It injects a rich, structured Markdown context snapshot into the Llama 3.2 GGUF prompt:

```markdown
### SYSTEM SECURITY CONTEXT SNAPSHOT

#### 1. Cross-Channel Event Anomalies (Security, System, App, Sysmon)
- [Security #4625] An account failed to log on. User: Administrator, Workstation: REMOTE-PC
- [System #7045] A service was installed: Name: svc_updater, Path: C:\Users\Public\update.exe
- [Sysmon #1] Process Create: powershell.exe -enc SQBFAFgA... Parent: winword.exe

#### 2. Flagged Suspicious Running Processes
- PID 4120: update.exe (Path: C:\Users\Public\update.exe, Net Write: 1.4 MB) -> Flag: Execution from public directory

#### 3. Suspicious Remote RDP & User Sessions
- Session 3: User 'admin_temp' from External IP '198.51.100.45' -> Flag: External public IP login

#### 4. User Principal Anomalies
- Account 'Guest': Status ENABLED -> Flag: Insecure default account active
- Account 'backup_admin': UserClass: Admin -> Flag: Non-standard local administrator

#### 5. Suspicious / Non-Windows Services
- Service 'svc_updater': Path 'C:\Users\Public\update.exe', Status: Running, StartType: Auto
```

The Local LLM synthesizes this complete picture to output:
- **Executive Threat Summary & Overall Risk Classification** (`CRITICAL`, `HIGH`, `MEDIUM`, `LOW`).
- **Correlated Attack Vector & MITRE ATT&CK Mapping** (e.g. *T1059 Command and Scripting Interpreter*, *T1078 Valid Accounts*, *T1543.003 Windows Service*).
- **Detected Indicators of Compromise (IoCs)**.
- **Specific Remediation & Containment Actions**.

---

### D. Frontend: SIEM Security Dashboard & Threat Copilot (`SmartEventViewerApp`)

1. **Cross-Channel Anomaly Feed**:
   - Unified multi-channel table displaying Critical, Error, and Warning events across `Security`, `System`, `Application`, and `Sysmon`.
2. **Host Posture Threat Indicators**:
   - Status badges on the Dashboard:
     - 🚨 *Suspicious Processes Detected*
     - ⚠️ *External RDP Connections*
     - ⚠️ *Account Privilege Alerts*
     - ⚠️ *Unverified Non-Windows Services*
3. **Interactive AI Security Analysis Tab**:
   - One-click **"Run Full-Spectrum Host Threat Assessment"** action.
   - Natural language queries across the entire correlated state (*"Are there any lateral movement attempts or unauthorized admin accounts?"*).

---

## 4. Phased Implementation Roadmap

| Phase | Milestone | Scope of Work |
| :--- | :--- | :--- |
| **Phase 1** | **Multi-Channel Event Ingestion** | Extend `EventService` to collect and aggregate high-priority events across `Security`, `System`, `Application`, and `Sysmon`. |
| **Phase 2** | **Telemetry Threat Heuristics** | Implement heuristic evaluators in `AnomalyEngine` for Processes, RDP Sessions, User Principals, and Windows Services. |
| **Phase 3** | **SIEM RAG Prompt Injection** | Update `AnalysisService` and `LocalLlmEngine` to construct and feed the unified Markdown Security Snapshot to Llama 3.2. |
| **Phase 4** | **Web API Controllers** | Expose multi-channel and telemetry threat endpoints via `EventsController` and `AnalysisController`. |
| **Phase 5** | **Frontend UI SIEM Integration** | Implement the Full-Spectrum SIEM Threat Assessment panel, cross-channel stream viewer, and live threat indicator badges in `Dashboard.tsx`. |
