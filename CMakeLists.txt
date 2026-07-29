cmake_minimum_required(VERSION 3.16)
project(SmartEventViewer VERSION 1.0.0 LANGUAGES CXX)

# Require C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for toolings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/SmartEventViewer
)

# Core Engine Source Files
set(SMARTEVENTVIEWER_SOURCES
    SmartEventViewer/pch.cpp
    SmartEventViewer/EventRecord.cpp
    SmartEventViewer/AnomalyEngine.cpp
    SmartEventViewer/WinEventLogReader.cpp
    SmartEventViewer/LinuxJournalReader.cpp
    SmartEventViewer/LocalLlmEngine.cpp
    SmartEventViewer/RagVectorStore.cpp
)

set(SMARTEVENTVIEWER_HEADERS
    Include/Common.h
    Include/DotNetDupe/String.h
    Include/DotNetDupe/List.h
    Include/Core/EventRecord.h
    Include/Core/IEventLogReader.h
    Include/Core/AnomalyEngine.h
    Include/Platform/WinEventLogReader.h
    Include/Platform/LinuxJournalReader.h
    Include/Ai/LocalLlmEngine.h
    Include/Ai/RagVectorStore.h
    SmartEventViewer/pch.h
)

# Define Shared Library Target
add_library(SmartEventViewer SHARED
    ${SMARTEVENTVIEWER_SOURCES}
    ${SMARTEVENTVIEWER_HEADERS}
)

target_compile_definitions(SmartEventViewer PRIVATE SMARTEVENTVIEWER_EXPORTS)

# Platform Specific Link Libraries
if(WIN32)
    target_link_libraries(SmartEventViewer PRIVATE wevtapi)
endif()

# Unit Tests Executable Target
set(TEST_SOURCES
    SmartEventViewerTests/EventRecordTests.cpp
)

add_executable(SmartEventViewerTests ${TEST_SOURCES})
target_link_libraries(SmartEventViewerTests PRIVATE SmartEventViewer)

if(WIN32)
    target_link_libraries(SmartEventViewerTests PRIVATE wevtapi)
endif()

# Enable Testing
enable_testing()
add_test(NAME SmartEventViewerTests COMMAND SmartEventViewerTests)
