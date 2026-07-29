# AnomalyEngine API Documentation

The `AnomalyEngine` class evaluates events for security threats, risks, and anomalous activities.

## Header
`#include "Core/AnomalyEngine.h"`

## Static Methods
- `static RiskLevel EvaluateRisk(const EventRecord& eventRec)`: Inspects event metadata, ID, and severity level to assign a SIEM risk rating (`Low`, `Medium`, `High`, `Critical`).

## Example Code Usage
```cpp
#include "Core/EventRecord.h"
#include "Core/AnomalyEngine.h"

void ProcessIncomingEvent(const SmartEventViewer::EventRecord& evt)
{
    SmartEventViewer::RiskLevel risk = SmartEventViewer::AnomalyEngine::EvaluateRisk(evt);
    if (risk == SmartEventViewer::RiskLevel::High)
    {
        // Trigger high severity SIEM alert
    }
}
```
