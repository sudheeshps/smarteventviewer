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

# Include directories (Include, SmartEventViewerCore, DotNetDupe, llama.cpp, vcpkg)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/SmartEventViewerCore
    ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.3.0.2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/DotNetDupe.3.0.2/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/ggml/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/common
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp
    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/include
)

# Core Engine Source Files
set(SMARTEVENTVIEWER_SOURCES
    SmartEventViewerCore/pch.cpp
    SmartEventViewerCore/EventRecord.cpp
    SmartEventViewerCore/AnomalyEngine.cpp
    SmartEventViewerCore/WinEventLogReader.cpp
    SmartEventViewerCore/SystemTelemetryProvider.cpp
    SmartEventViewerCore/LinuxJournalReader.cpp
    SmartEventViewerCore/LocalLlmEngine.cpp
    SmartEventViewerCore/RagVectorStore.cpp
)

set(SMARTEVENTVIEWER_HEADERS
    Include/Common.h
    Include/Core/EventRecord.h
    Include/Core/IEventLogReader.h
    Include/Core/AnomalyEngine.h
    Include/Platform/WinEventLogReader.h
    Include/Platform/LinuxJournalReader.h
    Include/Platform/SystemTelemetryProvider.h
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

target_compile_definitions(SmartEventViewerCore PRIVATE SMARTEVENTVIEWER_EXPORTS)

# Link llama.cpp package from vcpkg
find_package(llama CONFIG QUIET)
if(llama_FOUND)
    target_link_libraries(SmartEventViewerCore PRIVATE llama::llama)
elseif(TARGET llama)
    target_link_libraries(SmartEventViewerCore PRIVATE llama common)
endif()

# Platform Specific Link Libraries for DotNetDupe and Win32 APIs
if(WIN32)
    find_library(DOTNETDUPE_LIB DotNetDupe PATHS 
        ${CMAKE_CURRENT_SOURCE_DIR}/packages/DotNetDupe.3.0.2/runtimes/win-x64/native 
        ${CMAKE_CURRENT_SOURCE_DIR}/DotNetDupe.3.0.2/runtimes/win-x64/native
    )
    target_link_libraries(SmartEventViewerCore PRIVATE ${DOTNETDUPE_LIB} wevtapi wtsapi32 iphlpapi psapi)
endif()

# Native C++ REST Server Executable Target (SmartEventViewerServer)
set(SERVER_SOURCES
    SmartEventViewerServer/main.cpp
    SmartEventViewerServer/EventsController.cpp
)

add_executable(SmartEventViewerServer ${SERVER_SOURCES})
target_link_libraries(SmartEventViewerServer PRIVATE SmartEventViewerCore ${DOTNETDUPE_LIB})

if(WIN32)
    target_link_libraries(SmartEventViewerServer PRIVATE wevtapi wtsapi32 iphlpapi psapi)
endif()

# Unit Tests Executable Target (SmartEventViewerTests)
set(TEST_SOURCES
    SmartEventViewerTests/EventRecordTests.cpp
)

add_executable(SmartEventViewerTests ${TEST_SOURCES})
target_link_libraries(SmartEventViewerTests PRIVATE SmartEventViewerCore ${DOTNETDUPE_LIB})

if(WIN32)
    target_link_libraries(SmartEventViewerTests PRIVATE wevtapi wtsapi32 iphlpapi psapi)
endif()

# Enable Testing
enable_testing()
add_test(NAME SmartEventViewerTests COMMAND SmartEventViewerTests)

# Deployment & Packaging (CPack / Standalone ZIP)
install(TARGETS SmartEventViewerCore SmartEventViewerServer SmartEventViewerTests
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
