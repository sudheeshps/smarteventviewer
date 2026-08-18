# DotNetDupe API Proposal: Progressive Process Enumeration & Telemetry Streaming

## 1. Executive Summary & Problem Statement

### Current Limitation: "Fire and Hang" Synchronous Batching
In `DotNetDupe::System::Diagnostics::SystemMetrics`, the existing process query API:
```cpp
static List<ProcessInfo> GetTopProcesses(SystemResource resource, int count);
```
operates as a **monolithic, synchronous batch operation**.

To return the top $N$ processes, the engine must:
1. Snapshot all running processes on Windows ($200$ to $400+$ active processes).
2. Open process handles, sample CPU time deltas, inspect virtual/working set memory, resolve image file paths, query command line parameters from the PEB/WMI, and query TCP/UDP tables for **every single running process**.
3. Sort the entire collection in-memory on the backend.
4. Return only the top $N$ processes.

### Consequences on Client Applications
1. **Thread Blocking & Starvation**: The calling thread (such as an HTTP request worker or UI telemetry poller) is held in a synchronous loop for $1.5$–$3.0$ seconds.
2. **Delayed First Paint**: The client UI displays an empty/frozen table for seconds before all rows pop in at once.
3. **Restricted Client Interactivity**: If the server sorts by CPU, the client cannot sort by Memory, Disk I/O, or Network without issuing another multi-second server request.

---

## 2. Proposed Architecture & Dataflow

```
[Windows Kernel / Win32 Toolhelp / NtQuery]
                     │
                     ▼
[DotNetDupe: SystemMetrics::EnumerateProcessesAsync / ProcessStreamer]
  ├── Stage 1: Fast Base Scan (PIDs, Names, Session IDs, Memory) ──► Yield immediately
  └── Stage 2: Deep Enrichment (CPU deltas, I/O rates, Network)    ──► Stream as resolved
                     │
                     ▼ (Streaming Delegate / Event Stream)
[SmartEventViewerServer (TelemetryWebSocketHandler)]
  └── Batches chunks (5–10 items) ──► WebSocket SendAsync
                     │
                     ▼ (JSON Stream Chunks)
[SmartEventViewerApp (React + Web Worker)]
  ├── Progressive Table Ingestion (Rows populate incrementally in real time)
  └── Client-Side Dynamic Sorting & Filtering (CPU, RAM, Disk, Net, Search in 0ms)
```

---

## 3. Proposed DotNetDupe API Designs

### Option A: Streaming Delegate API (`SystemMetrics`)

A lightweight, zero-allocation visitor pattern supporting cancellation tokens and early termination:

```cpp
namespace DotNetDupe::System::Diagnostics {

    // Return true to continue streaming, false to cancel/stop enumeration early
    using ProcessYieldHandler = DotNetDupe::System::Func<bool, const ProcessInfo&>;

    class SystemMetrics {
    public:
        /// <summary>
        /// Progressively enumerates all processes, invoking the callback as each
        /// ProcessInfo is resolved, without allocating an aggregate collection.
        /// </summary>
        /// <param name="onProcessYield">Callback invoked per process record.</param>
        /// <param name="cancellationToken">Optional token to cancel mid-stream.</param>
        static void EnumerateProcesses(
            const ProcessYieldHandler& onProcessYield,
            const DotNetDupe::System::Threading::CancellationToken& cancellationToken = 
                DotNetDupe::System::Threading::CancellationToken::None);

        /// <summary>
        /// Asynchronously streams processes on a background thread pool task.
        /// </summary>
        static DotNetDupe::System::SmartPointer<DotNetDupe::System::Threading::Tasks::Task> 
        EnumerateProcessesAsync(
            const ProcessYieldHandler& onProcessYield,
            const DotNetDupe::System::Threading::CancellationToken& cancellationToken = 
                DotNetDupe::System::Threading::CancellationToken::None);
    };
}
```

---

### Option B: Event-Driven Observable Streamer (`ProcessStreamer`)

For advanced subscription patterns (progress percentages, chunk batching, pause/resume):

```cpp
namespace DotNetDupe::System::Diagnostics {

    struct ProcessBatchEventArgs {
        DotNetDupe::System::Collections::Generic::List<ProcessInfo> Processes;
        int ProcessedCount;
        int TotalEstimated;
        bool IsCompleted;
    };

    class ProcessStreamer {
    private:
        int m_iBatchSize{ 10 };
        DotNetDupe::System::Threading::CancellationTokenSource m_cts;

    public:
        explicit ProcessStreamer(int batchSize = 10);
        ~ProcessStreamer() = default;

        // Events
        DotNetDupe::System::Event<void(const ProcessInfo&)> OnProcessFound;
        DotNetDupe::System::Event<void(const ProcessBatchEventArgs&)> OnBatchReady;
        DotNetDupe::System::Event<void(int totalProcesses)> OnCompleted;
        DotNetDupe::System::Event<void(const DotNetDupe::System::Exception&)> OnError;

        void StartAsync();
        void Cancel();
        bool IsRunning() const;
    };
}
```

---

### Option C: Two-Tier Inspection Model (Instant Preview + Background Enrichment)

| Tier | Metrics Collected | Typical Latency | UI Benefit |
| :--- | :--- | :--- | :--- |
| **Tier 1: Fast Base** | PID, Name, Session ID, Base Working Set RAM | $< 5\text{ ms}$ for all 300 PIDs | Table immediately renders all active processes with accurate total count |
| **Tier 2: Deep Metric** | CPU % deltas, Disk I/O bytes, Network Ports/Sockets, Full Command Lines | Streamed progressively in chunks of 10 | Real-time CPU and I/O rates update progressively per row |

---

## 4. Client Integration Example (SmartEventViewer)

### 1. Server-Side Push over WebSocket (`SmartEventViewerServer`)
```cpp
void TelemetryWebSocketHandler::StreamAllProcesses(const SmartPointer<WebSocketContext>& pClient) {
    List<ProcessResourceDto> batch;
    
    SystemMetrics::EnumerateProcesses([&](const ProcessInfo& proc) -> bool {
        batch.Add(MapProcessResourceDto(proc));
        
        // Push every batch of 10 processes immediately over WebSocket
        if (batch.GetCount() >= 10) {
            String jsonChunk = SerializeProcessChunk(batch, /*isFinal=*/false);
            SendToClient(pClient, jsonChunk);
            batch.Clear();
        }
        return pClient->GetWebSocket()->GetState() == WebSocketState::Open;
    });

    // Flush remaining processes
    if (batch.GetCount() > 0) {
        String jsonChunk = SerializeProcessChunk(batch, /*isFinal=*/true);
        SendToClient(pClient, jsonChunk);
    }
}
```

### 2. Frontend Progressive Ingestion & Dynamic Multi-Column Sorting (`SmartEventViewerApp`)
```typescript
// telemetry.worker.ts
socket.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  if (msg.type === 'PROCESS_STREAM_CHUNK') {
    self.postMessage({
      type: 'PROCESS_STREAM_PROGRESS',
      chunk: msg.processes,
      isFinal: msg.isFinal
    });
  }
};

// Dashboard.tsx / ProcessesTab.tsx
// 1. Maintain a keyed map for O(1) row updates
// 2. Client sorts dynamically in 0ms using React useMemo:
const displayedProcesses = useMemo(() => {
  const list = Object.values(processMap);
  return list
    .filter(p => p.name.toLowerCase().includes(searchFilter.toLowerCase()))
    .sort((a, b) => {
      if (sortBy === 'cpu') return b.cpuUsagePercent - a.cpuUsagePercent;
      if (sortBy === 'memory') return b.memoryUsageMB - a.memoryUsageMB;
      if (sortBy === 'disk') return (b.diskReadMBps + b.diskWriteMBps) - (a.diskReadMBps + a.diskWriteMBps);
      if (sortBy === 'network') return (b.networkReadBytes + b.networkWriteBytes) - (a.networkReadBytes + a.networkWriteBytes);
      return a.processId - b.processId;
    });
}, [processMap, sortBy, searchFilter]);
```

---

## 5. Key Advantages

1. **Zero Thread Starvation**: Eliminates long synchronous execution in HTTP request threads and background workers.
2. **Instant First Paint ($<10\text{ms}$)**: Process entries start populating the UI within milliseconds of opening the tab.
3. **Full Client-Side Freedom**: The client UI holds all processes and can sort/filter by CPU, Memory, Disk, Network, or Name instantly without hitting the backend.
4. **Clean Cancellation**: If the user navigates to another view, the `CancellationToken` immediately halts kernel enumeration.
