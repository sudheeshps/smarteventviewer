# Llama-3-8B Security Analyst RAG API Documentation

The `LocalLlmEngine` class embeds the **Llama-3-8B-Instruct (GGUF Q4_K_M)** standalone model for natural language SIEM threat analysis.

## Header
`#include "Ai/LocalLlmEngine.h"`
`#include "Ai/RagVectorStore.h"`

## Methods
- `bool Initialize(const String& sModelPath)`: Loads the bundled `Llama-3-8B-Instruct.Q4_K_M.gguf` binary into local memory.
- `String ProcessQuery(const String& sQuery, const EventRecord* pContextEvents, unsigned int uEventCount)`: Synthesizes threat analysis using retrieved RAG context events.

## Example Usage
```cpp
#include "Ai/LocalLlmEngine.h"
#include "Ai/RagVectorStore.h"
#include <iostream>

using String = DotNetDupe::System::String;

int main()
{
    // 1. Initialize Vector Store RAG Knowledge Base
    SmartEventViewer::RagVectorStore vectorStore;
    vectorStore.IndexEvent(SmartEventViewer::EventRecord(1102, SmartEventViewer::EventLevel::Critical, String("Security"), String("Security"), String("Audit log cleared"), String("2026-07-29T12:00:00Z")));

    // 2. Initialize Bundled Llama-3-8B LLM Engine
    SmartEventViewer::LocalLlmEngine llm;
    llm.Initialize(String("models/Llama-3-8B-Instruct.Q4_K_M.gguf"));

    // 3. Query RAG Context Events
    auto contextEvents = vectorStore.QuerySimilarEvents(String("Find anti-forensics log clearing attempts"), 5);

    // 4. Perform Natural Language AI Security Analysis
    String analysisResponse = llm.ProcessQuery(
        String("Analyze anti-forensics threats in the logs"),
        &contextEvents[0],
        static_cast<unsigned int>(contextEvents.GetCount())
    );

    std::cout << analysisResponse.CStr() << "\n";
    return 0;
}
```
