# SmartEventViewer Project Instructions
## Project Overview
SmartEventViewer is a C++17 Windows Application designed to list all event sources same as Windows eventviewer.
But can analyze and show any anomalies or risks identified in events like security breaches, unusual process/network activities etc.
There shall be a comprehensive dashboard that allows to drill down to details. The user shall be able to ask natural language queries to analyze events. Local LLM combined with RAG handles user queries.
Design with cross-platform requirements in perspective.

### Core Technologies
- **Language**: C++17
- **Compiler**: MSVC (Visual Studio 2022 / v143 toolset preferred)
- **Build System**: MSBuild (`.sln`, `.vcxproj`)
- **Testing**: Google Test (gtest)
- **Platform**: Windows (Win32/x64)
- **Infrastructure**: Uses `DotNetDupe` library from NuGet for base infrastructure (e.g., `String`, `List`, `Thread`, `Task`, `SmartPointer`). Refrain from using STL types in public APIs. Suggest missing features in DotNetDupe.

## Project Structure
- `SmartEventViewer/`: Source code (`.cpp`) for the `SmartEventViewerCore` main library.
- `SmartEventViewerServer/`: C++ HTTP Web API and Static Web Server.
- `SmartEventViewerApp/`: React/TypeScript SIEM Dashboard UI.
- `SmartEventViewerTests/`: Unit tests using Google Test.
- `docs/`: Markdown documentation for various classes and comparisons.
- `bin/`: Output directory for compiled binaries.
- `obj/`: Intermediate directory for build artifacts.

## Building and Running

### Build via MSBuild
To build the solution from a Developer Command Prompt:
```powershell
msbuild SmartEventViewer.sln /p:Configuration=Release /p:Platform=x64
```

### Running Tests
After building, execute unit tests via the test runner:
```powershell
.\bin\x64\Release\SmartEventViewerTests.exe
```

## Development Conventions

### Naming & Style
- **Namespaces**: Use `SmartEventViewer` as root.
- **Classes**: PascalCase (e.g., `EventRecord`, `AnomalyEngine`).
- **Methods**: PascalCase (e.g., `GetLevel()`, `ProcessQuery()`).
- **Variables**: Follow Microsoft C++ coding standards:
  - **Local Variables & Parameters**: Hungarian notation with type-based prefixes (e.g., `sChannelName`, `iIndex`, `pBuffer`).
  - **Private Members**: Prefixed with `m_` followed by type prefix (e.g., `m_nItems`, `m_hHandle`).
  - **Static Fields**: Prefixed with `s_` (e.g., `s_defaultInstance`).
- **Headers**: Use `#pragma once` and include files relative to `Include/`.
- **Brace Style Convention**: **Opening braces `{` for classes, functions, namespaces, control statements, and loops must always be placed on the SAME line** as the declaration (1TBS / K&R style), e.g. `class MyClass {`, `void MyMethod() {`, `if (condition) {`.
- **Unit Test Naming**: Unit test names must strictly follow the `GivenWhenThen` naming convention format as established in DotNetDupe (e.g., `TEST(EventRecordTest, GivenValidRecord_WhenGetLevelCalled_ThenReturnsCorrectLevel)`).
- **Test Coverage Rules**: Every feature or component must include unit tests covering positive scenarios (happy path), negative scenarios (invalid inputs, exception throwing), and edge cases (empty strings, zero bounds, null pointers, boundary limits).

### Code Patterns & Logical Line Constraints
- **DotNetDupe Framework First**: **Use DotNetDupe data structures and classes exclusively (e.g., `DotNetDupe::System::String`, `Path`, `Collections::Generic::List`, `Thread`, `Task`) for internal logic and implementation.** Refrain from using STL types (`std::string`, `std::vector`, `std::map`) unless a feature is genuinely missing in `DotNetDupe`. If an STL type or algorithm must be used due to a missing `DotNetDupe` feature, **explicitly report the missing feature to the user**.
- **Public APIs**: Avoid using STL types (`std::string`, `std::vector`) in public function arguments or return types. Use `DotNetDupe::System::String` or `DotNetDupe::System::Collections::Generic::List`.
- **Function Length Limit**: **No function (member method, free function, or static helper function) can have more than 15 logical lines of code (LLOC).** (LLOC excludes blank lines, single-line braces `{` / `}`, and standalone comment lines). Break down complex functions into focused helper subroutines.
- **Class Length Limit**: **No single class implementation can have more than 500 logical lines of code (LLOC).** Split multi-responsibility classes into smaller, focused helper components.
- **File Length Limit**: **No single `.cpp` file can have more than 600 logical lines of code (LLOC).** Split larger implementation files across logical module files.
- **Code Readability & Grouping**: Add necessary blank lines inside functions to separate logical blocks (e.g., validation, initialization, main execution, return preparation) for improved readability.
- **Memory Management**: **SmartPointer from DotNetDupe shall be used in all places; no direct pointer manipulation (raw `new`, `delete`, raw pointers for ownership) is allowed.** Always follow RAII patterns.
- **Exceptions**: Do not throw standard C++ exceptions (`std::runtime_error`). Throw custom exception types inheriting from `DotNetDupe::System::Exception` (`SystemException`, `ArgumentException`, `InvalidOperationException`, `IOException`). **When catching exceptions, always explicitly catch `const DotNetDupe::System::Exception&` (or its derivatives) before generic `std::exception` or `...` blocks to properly handle DotNetDupe framework exceptions.**
- **Test Integrity**: **Existing unit tests shall not be modified or deleted during refactoring.** All unit tests must continue to pass cleanly.

## Contextual Precedence
The instructions in this file are foundational. Always adhere to these patterns when extending or modifying the codebase to maintain architectural consistency.
