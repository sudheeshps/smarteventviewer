# DotNetDupe API Proposal: Native Process Port & Connection Telemetry Integration

## 1. Executive Summary & Problem Statement

### Current Limitation: Segregated Process Telemetry & Network Tables
In `DotNetDupe::System::Diagnostics::SystemMetrics`, process hardware telemetry (`ProcessInfo`) and process network socket/port tables (`ProcessNetworkConnectionInfo`) are maintained in separate, disconnected APIs:

```cpp
// General process enumeration
static List<ProcessInfo> GetAllProcesses(int iSessionId = -1);
static SmartPointer<ProcessStreamer> CreateProcessStreamer(const ProcessStreamOptions& options);

// Name-based network queries
static List<int> GetProcessNetworkPort(const String& sProcessName);
static ProcessNetworkConnectionInfo GetProcessNetworkInfo(const String& sProcessName);
```

### Challenges with the Current Architecture
1. **Ambiguous Process Resolution by Name**:
   `GetProcessNetworkPort` and `GetProcessNetworkInfo` accept `sProcessName` (e.g., `"svchost.exe"`, `"chrome.exe"`, `"node.exe"`). When multiple instances with the same executable name run concurrently across different PIDs and user sessions, name-based resolution cannot accurately attribute open ports or sockets to the specific instance.
2. **Double Query & Allocation Overhead**:
   Consumer applications (like SIEM dashboards and system monitoring daemons) that need a comprehensive process table must first enumerate processes via `ProcessStreamer` or `GetAllProcesses()`, and then perform secondary queries per process by name, creating $O(N)$ redundant system table queries.
3. **Empty Ports in Progressive Streaming**:
   During progressive streaming via `ProcessStreamer`, `ProcessInfo` emitted via `OnBatch` and `OnProcessUpdated` lacks open port and connection status, forcing client mappers (e.g. `MapProcessResourceDto`) to leave `OpenPorts` blanked (`"-"`) or make synchronous blocking calls on UI/event worker threads.

---

## 2. Proposed DotNetDupe API Enhancements

### A. Extended `ProcessInfo` Data Structure
Extend `DotNetDupe::System::Diagnostics::ProcessInfo` to natively encapsulate listening port numbers, active TCP socket connections, and connection state:

```cpp
namespace DotNetDupe::System::Diagnostics {

    struct ProcessInfo {
        int iProcessId;
        int iSessionId;
        String sName;
        String sPath;
        String sCommandLine;
        double dCpuUsagePercent;
        MemoryInfo memory;
        DiskInfo disk;
        NetworkUsageInfo network;

        // --- NEW ENHANCEMENTS ---
        /// <summary>
        /// Active listening TCP and UDP ports owned by this specific PID (e.g., [80, 443, 8080]).
        /// </summary>
        Collections::Generic::List<int> lstOpenPorts;

        /// <summary>
        /// Active TCP/UDP socket connection details (local/remote addresses, ports, TCP state).
        /// </summary>
        Collections::Generic::List<NetworkConnectionInfo> lstConnections;

        /// <summary>
        /// Indicates whether the process has at least one active ESTABLISHED TCP connection.
        /// </summary>
        bool bHasEstablishedConnection;

        ProcessInfo()
            : iProcessId(0),
              iSessionId(0),
              dCpuUsagePercent(0.0),
              bHasEstablishedConnection(false) {}
    };
}
```

---

### B. PID-Based Direct Network Inspection APIs in `SystemMetrics`
Add direct PID-targeted overloads in `SystemMetrics` to avoid process name collisions:

```cpp
namespace DotNetDupe::System::Diagnostics {

    class SystemMetrics : public Object {
    public:
        // --- NEW PID-TARGETED APIS ---
        /// <summary>
        /// Retrieves listening TCP/UDP ports bound to the specified Process ID.
        /// </summary>
        DOTNETDUPE_API static Collections::Generic::List<int> GetProcessNetworkPort(int iProcessId);

        /// <summary>
        /// Retrieves complete socket connection and port tables bound to the specified Process ID.
        /// </summary>
        DOTNETDUPE_API static ProcessNetworkConnectionInfo GetProcessNetworkInfo(int iProcessId);
    };
}
```

---

## 3. Native Platform Implementation Architecture

### Windows (`win-x64`)
1. **Extended TCP/UDP Table Inspection**:
   - Use `GetExtendedTcpTable` with `TCP_TABLE_OWNER_PID_ALL` to snapshot all TCP endpoints and mapping to `dwOwningPid`.
   - Use `GetExtendedUdpTable` with `UDP_TABLE_OWNER_PID` to snapshot all UDP endpoints.
2. **Progressive Enrichment Pipeline**:
   - In `ProcessStreamer`, when `options.bIncludeNetworkInfo == true`:
     - **Tier 1 (Fast Discovery, $<5\text{ms}$)**: Emits initial `ProcessInfo` with `lstOpenPorts` empty for instantaneous UI rendering.
     - **Tier 2 (Metric Enrichment)**: Caches the global TCP/UDP snapshot once per stream cycle, matches sockets by `dwOwningPid == proc.iProcessId`, populates `proc.lstOpenPorts`, `proc.lstConnections`, and `proc.bHasEstablishedConnection`, and emits `OnProcessUpdated(proc)`.

### Linux (`linux-x64` / `linux-arm64`)
1. **`/proc/net` Table Parsing**:
   - Parse `/proc/net/tcp`, `/proc/net/tcp6`, `/proc/net/udp`, `/proc/net/udp6` to build an inode-to-port/state map.
   - Scan `/proc/[pid]/fd/` socket descriptors to match socket inodes to specific process IDs.
2. **Progressive Enrichment**:
   - Populate `proc.lstOpenPorts` and `proc.lstConnections` natively in `PopulateLinuxProc`.

---

## 4. Progressive Streamer Lifecycle with Port Telemetry

```
[ProcessStreamer::Start()]
         │
         ├──► [Tier 1: Fast Base Scan (<5ms)]
         │     ├── Enumerates PIDs, Names, Session IDs, Working Set RAM
         │     ├── proc.lstOpenPorts = []
         │     └── Emits: OnBatch(batch) / OnProcess(proc)
         │
         └──► [Tier 2: Asynchronous Deep Metric Enrichment]
               ├── Samples CPU % time delta over window
               ├── Queries Disk I/O read/write bytes
               ├── Queries Network I/O read/write bytes
               ├── Resolves Listening Ports & TCP Table by PID:
               │     ├── proc.lstOpenPorts = [8080, 443]
               │     └── proc.bHasEstablishedConnection = true
               └── Emits: OnProcessUpdated(proc)
```

---

## 5. Consumer Integration: `SmartEventViewer` Simplification

Once DotNetDupe is enhanced, client data mapping in `SmartEventViewerCore` simplifies to a direct, zero-allocation transformation:

```cpp
ProcessResourceDto SystemTelemetryProvider::MapProcessResourceDto(const DotNetDupe::System::Diagnostics::ProcessInfo& proc) {
    ProcessResourceDto procDto;
    try {
        procDto.ProcessId = static_cast<unsigned long>(proc.iProcessId);
        procDto.Name = proc.sName.IsEmpty() ? String("System Process") : proc.sName;
        procDto.Path = proc.sPath.IsEmpty() ? String("System Protected") : proc.sPath;
        procDto.CommandLine = FormatCommandLine(proc.sPath, proc.sCommandLine);
        
        double procCpu = proc.dCpuUsagePercent;
        procDto.CpuUsagePercent = procCpu < 0.0 ? 0.0 : (procCpu > 100.0 ? 100.0 : procCpu);
        
        long long rawRamBytes = proc.memory.lPhysicalMemoryBytes > 0 ? proc.memory.lPhysicalMemoryBytes : proc.memory.lPrivateBytes;
        procDto.MemoryUsageMB = static_cast<unsigned long long>(rawRamBytes > 0 ? (rawRamBytes / (1024 * 1024)) : 0);
        
        procDto.NetworkReadBytes = static_cast<unsigned long long>(proc.network.lNetworkReadBytes > 0 ? proc.network.lNetworkReadBytes : 0);
        procDto.NetworkWriteBytes = static_cast<unsigned long long>(proc.network.lNetworkWriteBytes > 0 ? proc.network.lNetworkWriteBytes : 0);
        
        // Native Open Ports Mapping
        if (proc.lstOpenPorts.GetCount() > 0) {
            String sPorts = "";
            for (int i = 0; i < proc.lstOpenPorts.GetCount() && i < 5; ++i) {
                if (i > 0) sPorts = sPorts + ", ";
                sPorts = sPorts + String::Format("{0}", static_cast<double>(proc.lstOpenPorts[i]));
            }
            procDto.OpenPorts = sPorts;
        } else {
            procDto.OpenPorts = String("-");
        }
        
        procDto.ConnectionEstablished = proc.bHasEstablishedConnection || 
                                       (procDto.NetworkReadBytes > 0 || procDto.NetworkWriteBytes > 0);
    } catch (...) {
    }
    return procDto;
}
```

---

## 6. Key Advantages

1. **Precision**: Eliminates process name collisions by resolving open ports strictly at the PID level.
2. **Performance**: Avoids $O(N)$ repeated table snapshots by batching socket table lookups inside `ProcessStreamer`.
3. **Clean Architecture**: `ProcessInfo` becomes a complete, self-contained record of all compute, memory, storage, and network resources for a process.
4. **Cross-Platform Consistency**: Provides identical interface semantics across Windows Win32/x64 and Linux `/proc` environments.
