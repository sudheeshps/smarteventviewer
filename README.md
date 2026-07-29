# SmartEventViewer

SmartEventViewer is a C++17 cross-platform Windows Application & Linux SIEM tool designed to ingest, monitor, analyze, and detect security risks across system event logs.

## Core Capabilities
- **Event Ingestion**: Ingests Windows Event Logs (`winevt`) and Linux system journals (`systemd-journal`/`syslog`).
- **SIEM Anomaly Detection**: Real-time risk scoring for security incidents (brute force attacks, unauthorized process creation, suspicious service installations).
- **RAG & Natural Language Q&A**: Perform natural language queries over log archives using the bundled **Llama-3-8B-Instruct (GGUF Q4_K_M)** offline local LLM with conversation history and follow-up support.
- **Modern Dashboard**: Embedded rich UI dashboard with real-time statistics, live toast popup notifications, day-wise grouping, criticality filters, and event detail inspection.

## Building and Running

### 1. CMake Build Scripts (Recommended for Cross-Platform)
Use `build_cmake.bat` with optional parameters: `release` (default), `debug`, `clean`, or `rebuild`.
```powershell
# Build Release (Default)
.\build_cmake.bat

# Build Debug
.\build_cmake.bat debug

# Rebuild Release
.\build_cmake.bat release rebuild

# Clean Build Directories
.\build_cmake.bat clean
```

### 2. MSBuild Scripts (Windows Visual Studio)
Use `build_msbuild.bat` with optional parameters: `release` (default), `debug`, `clean`, or `rebuild`.
```powershell
# Build Release (Default)
.\build_msbuild.bat

# Build Debug
.\build_msbuild.bat debug

# Rebuild Solution
.\build_msbuild.bat release rebuild

# Clean Solution
.\build_msbuild.bat clean
```

### 3. Running / Viewing the UI Dashboard
To launch and view the interactive SIEM Dashboard:
1. Navigate to the `UI/` folder.
2. Open `index.html` in any web browser:
   ```powershell
   Start-Process UI/index.html
   ```

## Documentation
- [EventRecord API Documentation](docs/EventRecord.md)
- [AnomalyEngine API Documentation](docs/AnomalyEngine.md)
- [LocalLlmEngine API Documentation](docs/LocalLlmEngine.md)
- [Local LLM & RAG Guide](docs/LocalLlmAndRagGuide.md)
