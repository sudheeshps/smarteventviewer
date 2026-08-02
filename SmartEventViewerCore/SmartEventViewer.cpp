#include "pch.h"
#include "../Include/Common.h"
#include "../Include/Core/EventRecord.h"
#include "../Include/Core/IEventLogReader.h"
#include "../Include/Core/AnomalyEngine.h"
#include "../Include/Platform/WinEventLogReader.h"
#include "../Include/Platform/LinuxJournalReader.h"
#include "../Include/Ai/LocalLlmEngine.h"

// Instantiate header inline functions inside compilation unit
namespace SmartEventViewer
{
    void ForceInstantiation()
    {
        EventRecord record;
        AnomalyEngine::EvaluateRisk(record);
        WinEventLogReader winReader;
        LinuxJournalReader linuxReader;
        LocalLlmEngine llm;
    }
}
