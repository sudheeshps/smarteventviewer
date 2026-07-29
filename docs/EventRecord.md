# EventRecord API Documentation

The `EventRecord` class represents a unified cross-platform system event entity (Windows Event Log or Linux Journal entry).

## Header
`#include "Core/EventRecord.h"`

## Methods
- `unsigned int GetEventId() const`: Returns the numerical Event ID.
- `EventLevel GetLevel() const`: Returns event severity (`Critical`, `Error`, `Warning`, `Informational`, `Verbose`).
- `RiskLevel GetRiskLevel() const`: Returns SIEM risk classification (`Low`, `Medium`, `High`, `Critical`).
- `const char* GetProviderName() const`: Returns event provider / publisher name.
- `const char* GetChannel() const`: Returns event channel / log path.
- `const char* GetMessage() const`: Returns formatted event description.
- `const char* GetTimeCreated() const`: Returns ISO 8601 timestamp string.

## Example Code Usage
```cpp
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"
#include <iostream>

int main()
{
    SmartEventViewer::EventRecord eventRec(
        4625,
        SmartEventViewer::EventLevel::Warning,
        "Microsoft-Windows-Security-Auditing",
        "Security",
        "An account failed to log on.",
        "2026-07-29T11:15:02Z"
    );

    SmartEventViewer::RiskLevel risk = SmartEventViewer::AnomalyEngine::EvaluateRisk(eventRec);
    eventRec.SetRiskLevel(risk);

    std::cout << "Event ID: " << eventRec.GetEventId() << "\n";
    std::cout << "Provider: " << eventRec.GetProviderName() << "\n";
    std::cout << "Risk Level: " << static_cast<int>(eventRec.GetRiskLevel()) << "\n";

    return 0;
}
```
