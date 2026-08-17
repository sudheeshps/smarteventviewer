cmake_minimum_required(VERSION 3.14)
project(SmartEventViewer VERSION 1.0.0 LANGUAGES CXX C)

# Integrate vcpkg toolchain dynamically via environment variable VCPKG_ROOT or command-line -DCMAKE_TOOLCHAIN_FILE
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    elseif(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    endif()
endif()

# Require C++20 Standard for DotNetDupe framework compatibility
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Configure llama.cpp subfolder build flags if present
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/CMakeLists.txt")
    add_subdirectory(third_party/llama.cpp)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/ggml/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/common
    )
endif()

# Include directories (Include, SmartEventViewerCore, DotNetDupe, llama.cpp, vcpkg, GoogleTest)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/SmartEventViewerCore
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.4.0.0/include
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8/build/native/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/ggml/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/common
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp
    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/x64-windows/include
)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/x64-windows/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.4.0.0/runtimes/win-x64/native
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8/lib/native/v140/windesktop/msvcstl/static/rt-dyn/x64/Release
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8/lib/native/v140/windesktop/msvcstl/static/rt-dyn/x64/Debug
)

# Core Engine Source Files
set(SMARTEVENTVIEWER_SOURCES
    SmartEventViewerCore/pch.cpp
    SmartEventViewerCore/EventRecord.cpp
    SmartEventViewerCore/AnomalyEngine.cpp
    SmartEventViewerCore/SystemTelemetryProvider.cpp
    SmartEventViewerCore/LinuxJournalReader.cpp
    SmartEventViewerCore/WindowsEtwLogReader.cpp
    SmartEventViewerCore/WindowsSystemTelemetryProvider.cpp
    SmartEventViewerCore/TelemetryChangeDetector.cpp
    SmartEventViewerCore/EventService.cpp
    SmartEventViewerCore/TelemetryService.cpp
    SmartEventViewerCore/AnalysisService.cpp
    SmartEventViewerCore/DiagnosticsService.cpp
    SmartEventViewerCore/TelemetryBackgroundWorker.cpp
    SmartEventViewerCore/LocalLlmEngine.cpp
    SmartEventViewerCore/AppLoggerManager.cpp
    SmartEventViewerCore/RagVectorStore.cpp
)

set(SMARTEVENTVIEWER_HEADERS
    Include/Dto/EventDtos.h
    Include/Dto/TelemetryDtos.h
    Include/Dto/AnalysisDtos.h
    Include/Dto/DiagnosticsDtos.h
    Include/Core/EventRecord.h
    Include/Core/IEventLogReader.h
    Include/Core/IAnomalyEngine.h
    Include/Core/ISystemTelemetryProvider.h
    Include/Core/ITelemetryPushNotifier.h
    Include/Core/IEventService.h
    Include/Core/ITelemetryService.h
    Include/Core/IAnalysisService.h
    Include/Core/IDiagnosticsService.h
    Include/Core/AnomalyEngine.h
    Include/Core/TelemetryChangeDetector.h
    Include/Core/EventService.h
    Include/Core/TelemetryService.h
    Include/Core/AnalysisService.h
    Include/Core/DiagnosticsService.h
    Include/Core/TelemetryBackgroundWorker.h
    Include/Core/SystemTelemetryProvider.h
    Include/Platform/LinuxJournalReader.h
    Include/Platform/WindowsEtwLogReader.h
    Include/Platform/WindowsSystemTelemetryProvider.h
    Include/Ai/ILlamaModelProvider.h
    Include/Ai/LocalLlmEngine.h
    Include/Ai/LlamaMessageDtos.h
    Include/Ai/RagVectorStore.h
    SmartEventViewerCore/pch.h
)

# Define Shared Library Target (SmartEventViewerCore)
add_library(SmartEventViewerCore SHARED
    ${SMARTEVENTVIEWER_SOURCES}
    ${SMARTEVENTVIEWER_HEADERS}
)

target_include_directories(SmartEventViewerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/SmartEventViewerCore
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.4.0.0/include
)

target_compile_definitions(SmartEventViewerCore PRIVATE
    SMARTEVENTVIEWERCORE_EXPORTS
)

# Link llama.cpp components to SmartEventViewerCore
target_link_libraries(SmartEventViewerCore PRIVATE
    llama
    ggml
    ggml-cpu
    ggml-base
)

target_link_directories(SmartEventViewerCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/x64-windows/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/x64-windows/debug/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/debug/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.4.0.0/runtimes/win-x64/native
)

# Platform Specific Link Libraries for DotNetDupe and Win32 APIs
if(WIN32)
    find_library(DOTNETDUPE_LIB DotNetDupe PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.4.0.0/runtimes/win-x64/native
    )
    target_link_libraries(SmartEventViewerCore PRIVATE ${DOTNETDUPE_LIB} wevtapi wtsapi32 iphlpapi psapi)
endif()

# Native C++ REST Server Executable Target (SmartEventViewerServer)
set(SERVER_SOURCES
    SmartEventViewerServer/main.cpp
    SmartEventViewerServer/Controllers/EventsController.cpp
    SmartEventViewerServer/Controllers/TelemetryController.cpp
    SmartEventViewerServer/Controllers/LlmAnalysisController.cpp
    SmartEventViewerServer/Controllers/DiagnosticsController.cpp
    SmartEventViewerServer/WebSockets/TelemetryWebSocketHandler.cpp
)

add_executable(SmartEventViewerServer ${SERVER_SOURCES})
target_include_directories(SmartEventViewerServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/SmartEventViewerServer
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.4.0.0/include
)
target_link_libraries(SmartEventViewerServer PRIVATE SmartEventViewerCore ${DOTNETDUPE_LIB})

if(WIN32)
    target_link_libraries(SmartEventViewerServer PRIVATE wevtapi wtsapi32 iphlpapi psapi)
endif()

# Unit Tests Executable Target (SmartEventViewerTests)
find_library(GTEST_LIB NAMES gtest gtestd PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8/lib/native/v140/windesktop/msvcstl/static/rt-dyn/x64/Release
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8/lib/native/v140/windesktop/msvcstl/static/rt-dyn/x64/Debug
)
find_library(GTEST_MAIN_LIB NAMES gtest_main gtest_maind PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8/lib/native/v140/windesktop/msvcstl/static/rt-dyn/x64/Release
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/Microsoft.googletest.v140.windesktop.msvcstl.static.rt-dyn.1.8.1.8/lib/native/v140/windesktop/msvcstl/static/rt-dyn/x64/Debug
)

set(TEST_SOURCES
    SmartEventViewerTests/EventRecordTests.cpp
    SmartEventViewerTests/ServiceUnitTests.cpp
)

add_executable(SmartEventViewerTests ${TEST_SOURCES})
target_include_directories(SmartEventViewerTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/SmartEventViewerTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.4.0.0/include
)
target_link_libraries(SmartEventViewerTests PRIVATE SmartEventViewerCore ${DOTNETDUPE_LIB} ${GTEST_LIB})

if(WIN32)
    target_link_libraries(SmartEventViewerTests PRIVATE wevtapi wtsapi32 iphlpapi psapi)
endif()

# Enable Testing
enable_testing()
add_test(NAME SmartEventViewerTests COMMAND SmartEventViewerTests)

# Integration Tests Executable Target (SmartEventViewerIntegrationTests)
set(INTEGRATION_TEST_SOURCES
    SmartEventViewerIntegrationTests/Main.cpp
    SmartEventViewerIntegrationTests/EventsControllerTests.cpp
    SmartEventViewerIntegrationTests/TelemetryControllerTests.cpp
    SmartEventViewerIntegrationTests/DiagnosticsControllerTests.cpp
    SmartEventViewerIntegrationTests/LlmAnalysisControllerTests.cpp
    SmartEventViewerIntegrationTests/PushNotificationTests.cpp
)

add_executable(SmartEventViewerIntegrationTests ${INTEGRATION_TEST_SOURCES})
target_link_libraries(SmartEventViewerIntegrationTests PRIVATE SmartEventViewerCore ${DOTNETDUPE_LIB} ${GTEST_LIB})

if(WIN32)
    target_link_libraries(SmartEventViewerIntegrationTests PRIVATE wevtapi wtsapi32 iphlpapi psapi shell32)
endif()

add_test(NAME SmartEventViewerIntegrationTests COMMAND SmartEventViewerIntegrationTests)

if(WIN32)
    add_custom_command(TARGET SmartEventViewerCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.4.0.0/runtimes/win-x64/native
        $<TARGET_FILE_DIR:SmartEventViewerCore>
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/x64-windows/bin")
        add_custom_command(TARGET SmartEventViewerCore POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/x64-windows/bin
            $<TARGET_FILE_DIR:SmartEventViewerCore>
        )
    endif()
endif()

# Deployment & Packaging (CPack / Standalone ZIP)
install(TARGETS SmartEventViewerCore SmartEventViewerServer SmartEventViewerTests SmartEventViewerIntegrationTests
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)

install(DIRECTORY SmartEventViewerApp/ DESTINATION UI OPTIONAL)
install(DIRECTORY models/ DESTINATION models OPTIONAL)

set(CPACK_PACKAGE_NAME "SmartEventViewer")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "SmartEventViewer Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SmartEventViewer SIEM Server & Dashboard Package")
set(CPACK_GENERATOR "ZIP;NSIS")
include(CPack)
