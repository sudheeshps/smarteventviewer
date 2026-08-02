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
    ${CMAKE_CURRENT_SOURCE_DIR}/SmartEventViewerCore
)

# Core Engine Source Files
set(SMARTEVENTVIEWER_SOURCES
    SmartEventViewerCore/pch.cpp
    SmartEventViewerCore/EventRecord.cpp
    SmartEventViewerCore/AnomalyEngine.cpp
    SmartEventViewerCore/WinEventLogReader.cpp
    SmartEventViewerCore/LinuxJournalReader.cpp
    SmartEventViewerCore/LocalLlmEngine.cpp
    SmartEventViewerCore/RagVectorStore.cpp
    
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
    Include/Core/NativeRestServer.h
    SmartEventViewerCore/pch.h
)

# Define Shared Library Target
add_library(SmartEventViewerCore SHARED
    ${SMARTEVENTVIEWER_SOURCES}
    ${SMARTEVENTVIEWER_HEADERS}
)

target_compile_definitions(SmartEventViewerCore PRIVATE SMARTEVENTVIEWER_EXPORTS)

# Platform Specific Link Libraries
if(WIN32)
    target_link_libraries(SmartEventViewerCore PRIVATE wevtapi)
endif()

# Dedicated Native C++ REST Server Executable Target
set(SERVER_SOURCES
    SmartEventViewerServer/main.cpp
)

add_executable(SmartEventViewerServer ${SERVER_SOURCES})
target_link_libraries(SmartEventViewerServer PRIVATE SmartEventViewerCore)

if(WIN32)
    target_link_libraries(SmartEventViewerServer PRIVATE wevtapi)
endif()

# Unit Tests Executable Target
set(TEST_SOURCES
    SmartEventViewerTests/EventRecordTests.cpp
)

add_executable(SmartEventViewerTests ${TEST_SOURCES})
target_link_libraries(SmartEventViewerTests PRIVATE SmartEventViewerCore)

if(WIN32)
    target_link_libraries(SmartEventViewerTests PRIVATE wevtapi)
endif()

# Enable Testing
enable_testing()

# ----------------------------------------------------
# Deployment & Packaging (CPack / Standalone ZIP)
# ----------------------------------------------------
install(TARGETS SmartEventViewerCore SmartEventViewerServer SmartEventViewerTests
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)

install(DIRECTORY UI/ DESTINATION UI)
install(DIRECTORY models/ DESTINATION models OPTIONAL)

set(CPACK_PACKAGE_NAME "SmartEventViewer")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "SmartEventViewer Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SmartEventViewer SIEM Server & Dashboard Package")
set(CPACK_GENERATOR "ZIP;NSIS")
include(CPack)

add_test(NAME SmartEventViewerTests COMMAND SmartEventViewerTests)
