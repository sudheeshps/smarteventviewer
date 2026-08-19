# Threat Score Calculation Refinement & Fallback Elimination Walkthrough

## Summary of Completed Changes

We analyzed the threat score calculation engine, identified the root causes of the inflated **`POSTURE: CRITICAL (100/100)`** score, eliminated false positives across telemetry evaluators, established category score ceilings, and removed all artificial fallback floors.

---

### Root Causes Identified & Fixed

| Area | Previous Behavior (Root Cause) | Refined Behavior |
| :--- | :--- | :--- |
| **Service Evaluation** ([`AnomalyEngine.cpp`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/AnomalyEngine.cpp#L110-L130)) | Flagged **every running Windows service** not starting with `"win"`, `"app"`, or `"sys"` (+4 points each &times; 50–100 services = 200+ points &rarr; 100/100). | Only flags confirmed malicious signatures (`mimikatz`, `meterpreter`, `psexec`, `cobaltstrike`, `backdoor`) or disabled security defenses (`windefend` stopped). |
| **User Evaluation** ([`AnomalyEngine.cpp`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/AnomalyEngine.cpp#L88-L108)) | Flagged any administrator account not named `"administrator"` or `"system"`, incorrectly flagging standard workstation developer accounts. | Only flags enabled Guest accounts or accounts with suspicious backdoor names (`backdoor`, `tempadmin`, `hacker`). |
| **LOLBin Detection** ([`AnomalyEngine.cpp`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/AnomalyEngine.cpp#L14-L26)) | Flagged PowerShell/CMD containing any `http://` or `https://` (common in `git clone`, `curl`, `npm`). | Checks for actual malicious execution and evasion flags (`-enc`, `downloadstring`, `iex`, `/urlcache`). |
| **Score Aggregation** ([`AnomalyEngine.cpp`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/AnomalyEngine.cpp#L132-L175)) | Linear uncapped addition across all categories. | Multi-vector weighted scoring with category ceilings (Processes max 35, Sessions max 30, Users max 20, Services max 15). |
| **Engine Score Floors** ([`LocalLlmEngine.cpp`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/LocalLlmEngine.cpp#L56-L68)) | `ComputeThreatScore` returned fallback `12` on `total == 0`. `FormatSiemThreatReport` forced fallback `compositeScore = (posture.ThreatScore > 0) ? posture.ThreatScore : 10;`. | Clean baseline returns `0`. Score combines actual telemetry posture with weighted event risks without arbitrary floors. |
| **UI Fallbacks** ([`RiskCenter.tsx`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerApp/src/components/RiskCenter.tsx#L34-L42)) | Initial state and network catch blocks defaulted to `threatScore: 10`. | Default initial and fallback state is `threatScore: 0` (`LOW` risk). |

---

### Threat Posture Scoring Matrix

The unified SIEM threat posture scale is now structured as follows:

```
Score:   0 ------- 14 ----------------- 39 ----------------- 69 ---------------- 100
Risk:       LOW             MEDIUM               HIGH                 CRITICAL
Desc:    Baseline     Elevated Monitoring   Active Investigation   Immediate Containment
```

- **Category Ceilings**:
  - **Host Processes**: Max 35 points (High: +15, Medium: +8)
  - **Remote / RDP Sessions**: Max 30 points (Critical: +25, High: +15)
  - **User Principals**: Max 20 points (High: +12, Medium: +6)
  - **System Services**: Max 15 points (High: +8, Low: +4)

---

### Changes by File

#### 1. [`SmartEventViewerCore/AnomalyEngine.cpp`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/AnomalyEngine.cpp)
- Replaced broad service prefix filtering with precision signature matching (`IsSuspiciousServiceName`) and critical security service monitoring.
- Replaced standard admin user filtering with suspicious naming check (`IsSuspiciousAdminName`).
- Implemented `AccumulateProcessScore`, `AccumulateSessionScore`, `AccumulateUserScore`, and `AccumulateServiceScore` with category ceilings.
- Kept every function &le; 15 LLOC adhering to 1TBS brace placement and Microsoft C++ naming conventions.

#### 2. [`SmartEventViewerCore/LocalLlmEngine.cpp`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/LocalLlmEngine.cpp)
- Removed `if (total == 0) return 12;` fallback floor from [`ComputeThreatScore`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/LocalLlmEngine.cpp#L56-L60), returning `0` on clean baselines.
- Implemented [`CalculateCompositeScore`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/LocalLlmEngine.cpp#L222-L228) to compute composite host and cross-channel event threat score without artificial floors.
- Aligned [`GetSeverityLabel`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerCore/LocalLlmEngine.cpp#L62-L67) with unified risk tiers.

#### 3. [`SmartEventViewerApp/src/components/RiskCenter.tsx`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerApp/src/components/RiskCenter.tsx)
- Updated initial React state to `threatScore: 0` (`LOW`).
- Updated API catch handlers to fallback to `threatScore: 0`.
- Aligned `threatScoreColor` thresholds with the refined risk levels.

#### 4. [`SmartEventViewerTests/Unit/EventRecordTests.cpp`](file:///d:/Personal/Projects/C++/smarteventviewer/SmartEventViewerTests/Unit/EventRecordTests.cpp)
- Added `GivenCleanTelemetry_WhenEvaluatePostureCalled_ThenReturnsZeroThreatScoreAndLowRisk`
- Added `GivenStandardWindowsServices_WhenEvaluateServiceCalled_ThenDoesNotFlagFalsePositives`
- Added `GivenStandardAdminUser_WhenEvaluateUserCalled_ThenDoesNotFlagAnomaly`
- Added `GivenZeroAnomalies_WhenComputeThreatScoreCalled_ThenReturnsZeroWithoutFallback`
