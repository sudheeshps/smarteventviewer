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
#include "System/IO/Directory.h"
#include "System/String.h"

using Console = DotNetDupe::System::Console;
using String = DotNetDupe::System::String;
using Path = DotNetDupe::System::IO::Path;
using File = DotNetDupe::System::IO::File;
using Directory = DotNetDupe::System::IO::Directory;
using namespace DotNetDupe::System::Threading;
using namespace DotNetDupe::System::Diagnostics;

static String FindExecutableDir(const String& sBaseDir) {
    if (File::Exists(Path::Combine({ sBaseDir, "SmartEventViewerServer.exe" }))) {
        return sBaseDir;
    }
    String sParent = Path::GetDirectoryName(sBaseDir);
    String sRelDebug = Path::Combine({ sParent, "bin", "x64", "Debug" });
    if (File::Exists(Path::Combine({ sRelDebug, "SmartEventViewerServer.exe" }))) {
        return sRelDebug;
    }
    String sRelRelease = Path::Combine({ sParent, "bin", "x64", "Release" });
    if (File::Exists(Path::Combine({ sRelRelease, "SmartEventViewerServer.exe" }))) {
        return sRelRelease;
    }
    return sBaseDir;
}

static String ResolveBinaryDir() {
#if defined(_WIN32) || defined(_WIN64)
    char szPath[MAX_PATH] = { 0 };
    ::GetModuleFileNameA(NULL, szPath, MAX_PATH);
    String sExeDir = Path::GetDirectoryName(String(szPath));
    if (File::Exists(Path::Combine({ sExeDir, "SmartEventViewerServer.exe" }))) {
        return sExeDir;
    }
#endif
    return FindExecutableDir(Path::GetFullPath("."));
}

static String SearchRepoUiAppDir(const String& sStartDir) {
    String sCur = Path::GetFullPath(sStartDir);
    for (int i = 0; i < 5 && !sCur.IsEmpty(); ++i) {
        String sCandidate = Path::Combine({ sCur, "SmartEventViewerApp" });
        if (Directory::Exists(sCandidate) && File::Exists(Path::Combine({ sCandidate, "package.json" }))) {
            return sCandidate;
        }
        String sParent = Path::GetDirectoryName(sCur);
        if (sParent == sCur) break;
        sCur = sParent;
    }
    return "";
}

static String ResolveUiAppDir(const String& sBinDir) {
    String sAppInBin = Path::Combine({ sBinDir, "SmartEventViewerApp" });
    if (Directory::Exists(sAppInBin) && File::Exists(Path::Combine({ sAppInBin, "package.json" }))) {
        return sAppInBin;
    }
    String sFromBin = SearchRepoUiAppDir(sBinDir);
    if (!sFromBin.IsEmpty()) return sFromBin;
    String sFromCwd = SearchRepoUiAppDir(Path::GetFullPath("."));
    if (!sFromCwd.IsEmpty()) return sFromCwd;
    return sAppInBin;
}

static ProcessStartInfo CreateServerProcessInfo(const String& sBinDir) {
    ProcessStartInfo serverStartInfo;
    serverStartInfo.FileName = Path::Combine({ sBinDir, "SmartEventViewerServer.exe" });
    serverStartInfo.WorkingDirectory = sBinDir;
    serverStartInfo.CreateNoWindow = true;
    return serverStartInfo;
}

static ProcessStartInfo CreateVitestProcessInfo(const String& sUiAppDir) {
    ProcessStartInfo vitestStartInfo;
    vitestStartInfo.FileName = "cmd.exe";
    vitestStartInfo.Arguments = "/c npx vitest run src/integration.test.ts";
    vitestStartInfo.WorkingDirectory = sUiAppDir;
    vitestStartInfo.CreateNoWindow = true;
    return vitestStartInfo;
}

static void ExecuteIntegrationTest(const String& sBinDir, const String& sUiAppDir) {
    ProcessStartInfo serverInfo = CreateServerProcessInfo(sBinDir);
    auto pServerProc = Process::Start(serverInfo);
    ASSERT_FALSE(pServerProc.IsNull()) << "Failed to start SmartEventViewerServer process from directory: " << sBinDir.GetRawString();

    Thread::Sleep(2000);

    ProcessStartInfo vitestInfo = CreateVitestProcessInfo(sUiAppDir);
    auto pVitestProc = Process::Start(vitestInfo);
    if (!pVitestProc.IsNull()) {
        pVitestProc->WaitForExit(30000);
        int iExitCode = pVitestProc->GetExitCode();
        EXPECT_EQ(iExitCode, 0) << "React Vitest integration suite returned non-zero exit code";
    }

    if (!pServerProc.IsNull()) {
        pServerProc->Kill();
    }
}

TEST(IntegrationTests, GivenDeployedServerAndReactClient_WhenLaunched_ThenValidatesEndToEndIntegration) {
    String sBinDir = ResolveBinaryDir();
    String sUiAppDir = ResolveUiAppDir(sBinDir);

    EXPECT_TRUE(File::Exists(Path::Combine({ sBinDir, "SmartEventViewerServer.exe" })));
    EXPECT_TRUE(Directory::Exists(sUiAppDir));

    ExecuteIntegrationTest(sBinDir, sUiAppDir);
}
