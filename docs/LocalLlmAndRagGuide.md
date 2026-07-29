# SmartEventViewer RAG Knowledge Base & Local Model Guide

## Recommended Local LLM for Standalone SIEM Security Analysis

For a standalone C++ SIEM application running locally on Windows or Linux, **Llama 3 8B Instruct (GGUF Q4_K_M)** or **Qwen 2.5 7B Instruct (GGUF)** bundled with `llama.cpp` dynamic bindings is the industry standard recommendation.

### Recommended Model Specifications
- **Model**: `Llama-3-8B-Instruct.Q4_K_M.gguf` or `Qwen2.5-7B-Instruct-Q4_K_M.gguf`
- **Memory Footprint**: ~4.3 GB RAM / VRAM
- **Inference Runtime**: Embedded `llama.cpp` (C++ native linkage, zero python dependency)
- **Security Benchmark**: Exceptional performance on MITRE ATT&CK TTP mapping, Windows Event ID analysis, and POSIX syslog anomaly triage.

---

## Standalone Ingested Knowledge Base Architecture (RAG)

```
┌─────────────────────────────────────────────────────────────┐
│                 System Event Ingestion                       │
│     (Windows Event Log API / Linux Systemd Journal)         │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                  Local Vector Store (RAG)                   │
│   - Embeddings: all-MiniLM-L6-v2 (quantized GGUF/ggml)     │
│   - Index: In-Memory / SQLite Vector Extension (vss)       │
└──────────────────────────────┬──────────────────────────────┘
                               │ Relevant Context Embeddings
┌──────────────────────────────▼──────────────────────────────┐
│           Embedded Local LLM (Llama 3 8B GGUF)             │
│        - Answers natural language query with context        │
└─────────────────────────────────────────────────────────────┘
```

---

## C++ RAG Architecture & Classes in SmartEventViewer

### 1. Vector Store & Embeddings Index (`RagVectorStore`)
Maintains an in-memory vector index of ingested event records.

```cpp
#pragma once
#include "Common.h"
#include "Core/EventRecord.h"
#include "DotNetDupe/String.h"
#include "DotNetDupe/List.h"

namespace SmartEventViewer
{
    using String = DotNetDupe::System::String;
    using EventList = DotNetDupe::System::Collections::Generic::List<EventRecord>;

    class SMARTEVENTVIEWER_API RagVectorStore
    {
    public:
        void IndexEvent(const EventRecord& eventRec);
        EventList QuerySimilarEvents(const String& sNaturalLanguageQuery, size_t nTopK);
    };
}
```

### 2. Embedded LLM Engine (`LocalLlmEngine`)
Loads the bundled GGUF model binary and streams natural language security responses.

```cpp
#include "Ai/LocalLlmEngine.h"
#include "Ai/RagVectorStore.h"

// Example Natural Language Security Query Workflow
SmartEventViewer::LocalLlmEngine llm;
llm.Initialize(String("models/Llama-3-8B-Instruct.Q4_K_M.gguf"));

SmartEventViewer::RagVectorStore vectorStore;
// Search RAG index for relevant events matching query
auto relevantEvents = vectorStore.QuerySimilarEvents(String("Find any privilege escalation attempts today"), 5);

// Generate security response
String response = llm.ProcessQuery(String("Summarize security threats found in the events"), &relevantEvents[0], (unsigned int)relevantEvents.GetCount());
```

---

## Bundling & Distribution Packaging
1. **Directory Structure**:
   ```
   SmartEventViewer/
   ├── bin/x64/Release/
   │   ├── SmartEventViewer.dll
   │   ├── SmartEventViewerTests.exe
   │   └── models/
   │       └── Llama-3-8B-Instruct.Q4_K_M.gguf  (~4.3 GB bundled model)
   ```
2. **Standalone Operation**: No network calls or cloud APIs required. Runs 100% offline with complete privacy for sensitive enterprise event logs.
