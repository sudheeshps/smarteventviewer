#include "pch.h"
#include "Core/TelemetryBackgroundWorker.h"
#include "Core/TelemetryCacheManager.h"
#include "Core/TelemetryWebSocketHandler.h"
#include "System/Console.h"
#include "System/SystemException.h"
#include "System/Threading/Thread.h"
#include <exception>

using Console = DotNetDupe::System::Console;
using Thread = DotNetDupe::System::Threading::Thread;
using ThreadStart = DotNetDupe::System::Threading::ThreadStart;

namespace SmartEventViewer {
    bool TelemetryBackgroundWorker::s_bRunning = false;
    DotNetDupe::System::SmartPointer<Thread> TelemetryBackgroundWorker::s_spWorkerThread = nullptr;

    void TelemetryBackgroundWorker::Start() {
        if (s_bRunning) return;
        s_bRunning = true;
        s_spWorkerThread = DotNetDupe::System::SmartPointer<Thread>::NewShared(ThreadStart([]() {
            WorkerThreadProc();
        }));
        s_spWorkerThread->Start();
        Console::WriteLine("[TELEMETRY_WORKER] Background sampling worker thread started via DotNetDupe::System::Threading::Thread.");
    }

    void TelemetryBackgroundWorker::Stop() {
        if (!s_bRunning) return;
        s_bRunning = false;
        if (!s_spWorkerThread.IsNull()) {
            s_spWorkerThread->Join(2000);
            s_spWorkerThread = nullptr;
        }
        Console::WriteLine("[TELEMETRY_WORKER] Background sampling worker thread stopped.");
    }

    bool TelemetryBackgroundWorker::IsRunning() {
        return s_bRunning;
    }

    void TelemetryBackgroundWorker::WorkerThreadProc() {
        uint64_t uIterationCount = 0;

        while (s_bRunning) {
            Thread::Sleep(1000);
            if (!s_bRunning) break;

            uIterationCount++;

            // 1. Every 1 second: update Summary
            try {
                TelemetryCacheManager::GetInstance().GetSummary();
                TelemetryWebSocketHandler::GetInstance()->BroadcastCategoryUpdate("summary");
            } catch (const DotNetDupe::System::SystemException& sysEx) {
                Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Summary sampling failed (DotNetDupe SystemException): {0}", sysEx.What()));
            } catch (const DotNetDupe::System::Exception& dupeEx) {
                Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Summary sampling failed (DotNetDupe BasicException): {0}", dupeEx.What()));
            } catch (const std::exception& ex) {
                Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Summary sampling failed (std::exception): {0}", ex.what()));
            } catch (...) {
                Console::WriteLine("[TELEMETRY_WORKER_ERROR] Summary sampling failed (Unknown Exception).");
            }

            // 2. Every 1 second: update Processes
            try {
                TelemetryCacheManager::GetInstance().GetProcesses();
                TelemetryWebSocketHandler::GetInstance()->BroadcastCategoryUpdate("processes");
            } catch (const DotNetDupe::System::SystemException& sysEx) {
                Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Processes sampling failed (DotNetDupe SystemException): {0}", sysEx.What()));
            } catch (const DotNetDupe::System::Exception& dupeEx) {
                Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Processes sampling failed (DotNetDupe BasicException): {0}", dupeEx.What()));
            } catch (const std::exception& ex) {
                Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Processes sampling failed (std::exception): {0}", ex.what()));
            } catch (...) {
                Console::WriteLine("[TELEMETRY_WORKER_ERROR] Processes sampling failed (Unknown Exception).");
            }

            // 3. Every 10 seconds (every 10 iterations): update Sessions
            if (uIterationCount % 10 == 0) {
                try {
                    TelemetryCacheManager::GetInstance().GetSessions();
                    TelemetryWebSocketHandler::GetInstance()->BroadcastCategoryUpdate("sessions");
                } catch (const DotNetDupe::System::SystemException& sysEx) {
                    Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Sessions sampling failed (DotNetDupe SystemException): {0}", sysEx.What()));
                } catch (const DotNetDupe::System::Exception& dupeEx) {
                    Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Sessions sampling failed (DotNetDupe BasicException): {0}", dupeEx.What()));
                } catch (const std::exception& ex) {
                    Console::WriteLine(String::Format("[TELEMETRY_WORKER_ERROR] Sessions sampling failed (std::exception): {0}", ex.what()));
                } catch (...) {
                    Console::WriteLine("[TELEMETRY_WORKER_ERROR] Sessions sampling failed (Unknown Exception).");
                }
            }
        }
    }
}
