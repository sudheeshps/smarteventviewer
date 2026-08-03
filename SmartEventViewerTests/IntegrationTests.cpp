#include <cassert>

#include "System/Console.h"
#include "System/Threading/Thread.h"
#include "System/Diagnostics/Process.h"
#include "System/Path.h"
#include "System/IO/File.h"
#include "System/String.h"

using Console = DotNetDupe::System::Console;
using String = DotNetDupe::System::String;
using Path = DotNetDupe::System::IO::Path;
using File = DotNetDupe::System::IO::File;
using namespace DotNetDupe::System::Threading;
using namespace DotNetDupe::System::Diagnostics;

void Test_RealWebServer_And_ReactClient_Integration()
{
    Console::WriteLine("========================================================================");
    Console::WriteLine("  SmartEventViewer Real WebServer & React Client Integration Test Suite");
    Console::WriteLine("========================================================================");

    // 1. Resolve common IntegrationTestDeployment folder
    String sWorkingDir = Path::GetFullPath(".");
    String sDeployDir = sWorkingDir;
    
    // Check if we are running in the deployed folder or output folder
    if (sWorkingDir.IndexOf("IntegrationTestDeployment") == -1)
    {
        sDeployDir = Path::Combine({ sWorkingDir, "IntegrationTestDeployment" });
    }

    String sUiAppDir = Path::Combine({ sDeployDir, "SmartEventViewerApp" });
    String sServerExePath = Path::Combine({ sDeployDir, "SmartEventViewerServer.exe" });

    if (!File::Exists(Path::Combine({ sUiAppDir, "package.json" })))
    {
        // Fallback: If executed from repository root or source directory
        String sRootDir = Path::GetFullPath(".");
        if (sRootDir.IndexOf("bin") != -1)
        {
            sRootDir = Path::GetDirectoryName(Path::GetDirectoryName(sRootDir));
        }
        sUiAppDir = Path::Combine({ sRootDir, "SmartEventViewerApp" });
        sServerExePath = Path::Combine({ sWorkingDir, "SmartEventViewerServer.exe" });
    }

    Console::WriteLine("[INTEGRATION TEST] Deployment folder: {0}", sDeployDir);
    Console::WriteLine("[INTEGRATION TEST] Target React app directory: {0}", sUiAppDir);
    Console::WriteLine("[INTEGRATION TEST] Target Server Executable: {0}", sServerExePath);

    // 2. Launch real SmartEventViewerServer.exe backend process via DotNetDupe Process API
    ProcessStartInfo serverStartInfo;
    serverStartInfo.FileName = sServerExePath;
    serverStartInfo.WorkingDirectory = sDeployDir;
    serverStartInfo.CreateNoWindow = true;
    
    auto pServerProc = Process::Start(serverStartInfo);
    assert(!pServerProc.IsNull());

    // Wait for server process to start and bind port 8080
    Console::WriteLine("[INTEGRATION TEST] Waiting 2 seconds for server to initialize and bind http://127.0.0.1:8080/ ...");
    Thread::Sleep(2000);

    // 3. Execute React Vitest Integration Suite (src/integration.test.ts) via DotNetDupe Process API
    Console::WriteLine("[INTEGRATION TEST] Executing React Vitest integration suite (SmartEventViewerApp/src/integration.test.ts)...");
    
    ProcessStartInfo vitestStartInfo;
    vitestStartInfo.FileName = String("cmd.exe");
    vitestStartInfo.Arguments = String("/c npx vitest run src/integration.test.ts");
    vitestStartInfo.WorkingDirectory = sUiAppDir;

    auto pVitestProc = Process::Start(vitestStartInfo);
    if (!pVitestProc.IsNull())
    {
        pVitestProc->WaitForExit(15000);
        int iExitCode = pVitestProc->GetExitCode();
        Console::WriteLine("[React Vitest Integration] Process exit code: {0}", iExitCode);

        if (iExitCode != 0)
        {
            Console::WriteLine("[FAIL] React Vitest integration suite returned non-zero exit code!");
            if (!pServerProc.IsNull())
            {
                pServerProc->Kill();
            }
            exit(iExitCode);
        }
    }

    // 4. Clean up server process via DotNetDupe Process API
    if (!pServerProc.IsNull())
    {
        Console::WriteLine("[INTEGRATION TEST] Terminating SmartEventViewerServer.exe process via DotNetDupe Process::Kill()...");
        pServerProc->Kill();
    }

    Console::WriteLine("[PASS] Test_RealWebServer_And_ReactClient_Integration PASSED CLEANLY!");
}
