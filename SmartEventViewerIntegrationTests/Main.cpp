#include <gtest/gtest.h>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
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

#if defined(_WIN32) || defined(_WIN64)
static HANDLE s_hElevatedProcess = NULL;
#endif
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

#if defined(_WIN32) || defined(_WIN64)
static bool TryLaunchWithShellExecute(const String& sExePath, const String& sWorkDir) {
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "runas";
    sei.lpFile = sExePath.GetRawString();
    sei.lpDirectory = sWorkDir.GetRawString();
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    if (ShellExecuteExA(&sei) && sei.hProcess != NULL) {
        s_hElevatedProcess = sei.hProcess;
        Console::WriteLine("[INTEGRATION_TEST] Server process started via ShellExecute 'runas'.");
        return true;
    }
    return false;
}
#endif

static void LaunchServerFallback(const String& sExePath) {
    ProcessStartInfo startInfo;
    startInfo.FileName = sExePath;
    startInfo.WorkingDirectory = Path::GetDirectoryName(sExePath);
    startInfo.CreateNoWindow = true;
    Console::WriteLine("[INTEGRATION_TEST] Starting server via standard Process::Start...");
    s_pServerProcess = Process::Start(startInfo);
}

static void LaunchServerProcess(const String& sExePath) {
    AssertAndLogExePath(sExePath);
    String sWorkDir = Path::GetDirectoryName(sExePath);
    Console::WriteLine("[INTEGRATION_TEST] Starting server in elevated mode ('runas'): {0}", sExePath);
#if defined(_WIN32) || defined(_WIN64)
    if (TryLaunchWithShellExecute(sExePath, sWorkDir)) {
        return;
    }
#endif
    LaunchServerFallback(sExePath);
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

static void StopServerProcess() {
#if defined(_WIN32) || defined(_WIN64)
    if (s_hElevatedProcess != NULL) {
        Console::WriteLine("[INTEGRATION_TEST] Stopping elevated server process...");
        ::TerminateProcess(s_hElevatedProcess, 0);
        ::CloseHandle(s_hElevatedProcess);
        s_hElevatedProcess = NULL;
    }
#endif
    if (!s_pServerProcess.IsNull() && !s_pServerProcess->GetHasExited()) {
        Console::WriteLine("[INTEGRATION_TEST] Stopping background server process...");
        s_pServerProcess->Kill();
        s_pServerProcess = nullptr;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    String sExePath = ResolveServerExePath();
    LaunchServerProcess(sExePath);
    WaitForServerReady(30);

    int iResult = RUN_ALL_TESTS();

    StopServerProcess();
    return iResult;
}
