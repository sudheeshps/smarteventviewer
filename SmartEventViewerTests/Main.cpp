#include <gtest/gtest.h>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "System/Console.h"
#include "System/Threading/Thread.h"
#include "System/Diagnostics/Process.h"
#include "System/IO/Path.h"
#include "System/IO/File.h"
#include "System/String.h"
#include "System/Exception.h"
#include "System/InvalidOperationException.h"
#include "System/Net/Sockets/TcpClient.h"

using namespace DotNetDupe::System;
using namespace DotNetDupe::System::IO;
using namespace DotNetDupe::System::Threading;
using namespace DotNetDupe::System::Diagnostics;
using namespace DotNetDupe::System::Net::Sockets;

static SmartPointer<Process> s_pServerProcess = nullptr;

static String FindServerExeInBin(const String& sBaseDir) {
    String sExact = Path::Combine({ sBaseDir, "SmartEventViewerServer.exe" });
    if (File::Exists(sExact)) return sExact;
    String sParent = Path::GetDirectoryName(sBaseDir);
    String sRelease = Path::Combine({ sParent, "bin", "x64", "Release", "SmartEventViewerServer.exe" });
    if (File::Exists(sRelease)) return sRelease;
    String sDebug = Path::Combine({ sParent, "bin", "x64", "Debug", "SmartEventViewerServer.exe" });
    if (File::Exists(sDebug)) return sDebug;
    return sExact;
}

static String ResolveServerExePath() {
#if defined(_WIN32) || defined(_WIN64)
    char szPath[MAX_PATH] = { 0 };
    ::GetModuleFileNameA(NULL, szPath, MAX_PATH);
    String sExeDir = Path::GetDirectoryName(String(szPath));
    String sFound = FindServerExeInBin(sExeDir);
    if (File::Exists(sFound)) return sFound;
#endif
    return FindServerExeInBin(Path::GetFullPath("."));
}

static void AssertAndLogExePath(const String& sExePath) {
    Console::WriteLine("[INTEGRATION_TEST] Found Server Exe Path: {0}", sExePath);
    if (!File::Exists(sExePath)) {
        Console::WriteLine("[INTEGRATION_TEST_FATAL] Server executable not found at: {0}", sExePath);
        throw InvalidOperationException("Server executable does not exist at specified path.");
    }
}

static void LaunchServerProcess(const String& sExePath) {
    AssertAndLogExePath(sExePath);
    ProcessStartInfo startInfo;
    startInfo.FileName = sExePath;
    startInfo.WorkingDirectory = Path::GetDirectoryName(sExePath);
    startInfo.CreateNoWindow = true;
    Console::WriteLine("[INTEGRATION_TEST] Starting server via Process::Start: {0}", sExePath);
    s_pServerProcess = Process::Start(startInfo);
}

static bool IsServerPortListening(const String& sHost, int iPort) {
    try {
        TcpClient client;
        client.Connect(sHost, iPort);
        client.Close();
        return true;
    }
    catch (const Exception&) {
        return false;
    }
}

static void WaitForServerReady(int iMaxAttempts) {
    Console::WriteLine("[INTEGRATION_TEST] Waiting for server listener on port 8080...");
    for (int iAttempt = 0; iAttempt < iMaxAttempts; ++iAttempt) {
        if (IsServerPortListening("127.0.0.1", 8080)) {
            Console::WriteLine("[INTEGRATION_TEST] Server is alive and responsive on port 8080.");
            return;
        }
        Thread::Sleep(500);
    }
    Console::WriteLine("[INTEGRATION_TEST] Server listener wait completed.");
}

static void SendCtrlCToServer(DWORD dwPid) {
#if defined(_WIN32) || defined(_WIN64)
    if (dwPid > 0) {
        Console::WriteLine("[INTEGRATION_TEST] Sending Ctrl+C signal to server (PID: {0})...", static_cast<int>(dwPid));
        ::SetConsoleCtrlHandler(NULL, TRUE);
        ::GenerateConsoleCtrlEvent(CTRL_C_EVENT, dwPid);
        ::GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, dwPid);
        ::GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
        ::SetConsoleCtrlHandler(NULL, FALSE);
    }
#endif
}

static void StopServerProcess() {
    Console::WriteLine("[INTEGRATION_TEST] Checking server process handle...");
    if (!s_pServerProcess.IsNull()) {
        Console::WriteLine("[INTEGRATION_TEST] Stopping background server process...");
        try {
            //SendCtrlCToServer(static_cast<DWORD>(s_pServerProcess->GetId()));
            if (!s_pServerProcess->WaitForExit(2500) && !s_pServerProcess->GetHasExited()) {
                s_pServerProcess->Kill();
                s_pServerProcess->WaitForExit(1000);
            }
        } catch (const Exception& ex) {
            Console::WriteLine("[INTEGRATION_TEST] Server stop exception: {0}", ex.What());
        }
        s_pServerProcess = nullptr;
    }
}

bool g_bEnableModelDownloadTest = false;

static void ParseTestFlags(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--test-download") == 0 || std::strcmp(argv[i], "--test-model-download") == 0) {
            g_bEnableModelDownloadTest = true;
            Console::WriteLine(String::Format("[TEST_RUNNER] Model download live test enabled via '{0}'.", String(argv[i])));
        }
    }
}

static bool ShouldPauseAfterRun(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--pause") == 0) return true;
    }
    return false;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ParseTestFlags(argc, argv);
    bool bPause = ShouldPauseAfterRun(argc, argv);

    String sExePath = ResolveServerExePath();
    LaunchServerProcess(sExePath);
    WaitForServerReady(30);

    int iResult = RUN_ALL_TESTS();

    StopServerProcess();

    if (bPause) {
        Console::WriteLine("\nPress Enter to continue . . .");
        Console::Read();
    }
    return iResult;
}
