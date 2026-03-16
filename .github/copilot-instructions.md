# GitHub Copilot Instructions

These instructions define how GitHub Copilot should assist with this project. The goal is to ensure consistent, high-quality code generation aligned with our conventions, stack, and best practices.

## Context

- **Project Type**: Graphics Library / DirectX / Direct3D 11 / Direct3D 12 / Geometry Processing
- **Project Name**: DirectXMesh Geometry Processing Library
- **Language**: C++
- **Framework / Libraries**: STL / CMake / CTest
- **Architecture**: Modular / RAII / OOP

## Getting Started

- See the tutorial at [Getting Started](https://github.com/microsoft/DirectXMesh/wiki/Getting-Started).
- The recommended way to integrate *DirectXMesh* into your project is by using the *vcpkg* Package Manager.
- You can make use of the nuget.org packages **directxmesh_desktop_2019**, **directxmesh_desktop_win10**, or **directxmesh_uwp**.
- You can also use the library source code directly in your project or as a git submodule.

## General Guidelines

- **Code Style**: The project uses an .editorconfig file to enforce coding standards. Follow the rules defined in `.editorconfig` for indentation, line endings, and other formatting. Additional information can be found on the wiki at [Implementation](https://github.com/microsoft/DirectXTK/wiki/Implementation). The library implementation is written to be compatible with C++14 features, but C++17 is required to build the project for the command-line tools which utilize C++17 filesystem for long file path support.
> Notable `.editorconfig` rules: C/C++ files use 4-space indentation, `crlf` line endings, and `latin1` charset — avoid non-ASCII characters in source files. HLSL files have separate indent/spacing rules defined in `.editorconfig`.
- **Documentation**: The project provides documentation in the form of wiki pages available at [Documentation](https://github.com/microsoft/DirectXMesh/wiki/).
- **Error Handling**: Use C++ exceptions for error handling and uses RAII smart pointers to ensure resources are properly managed. For some functions that return HRESULT error codes, they are marked `noexcept`, use `std::nothrow` for memory allocation, and should not throw exceptions.
- **Testing**: Unit tests for this project are implemented in this repository [Test Suite](https://github.com/walbourn/directxmeshtest/) and can be run using CTest per the instructions at [Test Documentation](https://github.com/walbourn/directxmeshtest/wiki).
- **Security**: This project uses secure coding practices from the Microsoft Secure Coding Guidelines, and is subject to the `SECURITY.md` file in the root of the repository. Functions that read input from geometry files are subject to OneFuzz fuzz testing to ensure they are secure against malformed files.
- **Dependencies**: The project uses CMake and VCPKG for managing dependencies, making optional use of DirectXMath and DirectX-Headers. The project can be built without these dependencies, relying on the Windows SDK for core functionality.
- **Continuous Integration**: This project implements GitHub Actions for continuous integration, ensuring that all code changes are tested and validated before merging. This includes building the project for a number of configurations and toolsets, running a subset of unit tests, and static code analysis including GitHub super-linter, CodeQL, and MSVC Code Analysis.
- **Code of Conduct**: The project adheres to the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). All contributors are expected to follow this code of conduct in all interactions related to the project.

## File Structure

```txt
.azuredevops/ # Azure DevOps pipeline configuration and policy files.
.github/      # GitHub Actions workflow files and linter configuration files.
.nuget/       # NuGet package configuration files.
build/        # Miscellaneous build files and scripts.
DirectXMesh/  # DirectXMesh implementation files.
Utilities/    # Utility headers such as a WaveFront .obj file loader and a FVF converter.
Meshconvert/  # CLI tool for importing WaveFront .obj files and converting to CMO, SDKMESH, or VBO formats.
Tests/        # Tests are designed to be cloned from a separate repository at this location.
wiki/         # Local clone of the GitHub wiki documentation repository.
```

## Patterns

### Patterns to Follow

- Use RAII for all resource ownership (memory, file handles, etc.).
- Many classes utilize the pImpl idiom to hide implementation details, and to enable optimized memory alignment in the implementation.
- Use `std::unique_ptr` for exclusive ownership and `std::shared_ptr` for shared ownership.
- Use `Microsoft::WRL::ComPtr` for COM object management.
- Make use of anonymous namespaces to limit scope of functions and variables.
- Make use of `assert` for debugging checks, but be sure to validate input parameters in release builds.
- Explicitly `= delete` copy constructors and copy-assignment operators on all classes that use the pImpl idiom.
- Explicitly utilize `= default` or `=delete` for copy constructors, assignment operators, move constructors and move-assignment operators where appropriate.
- Use 16-byte alignment (`_aligned_malloc` / `_aligned_free`) to support SIMD operations in the implementation, but do not expose this requirement in public APIs.
> For non-Windows support, the implementation uses C++17 `aligned_alloc` instead of `_aligned_malloc`.

#### SAL Annotations

All public API functions must use SAL annotations on every parameter. Use `_Use_decl_annotations_` at the top of each implementation that has SAL in the header declaration — never repeat the annotations in the `.cpp` or `.inl` file.

Common annotations:

| Annotation | Meaning |
| --- | --- |
| `_In_` | Input parameter |
| `_In_opt_` | Optional input pointer |
| `_In_z_` | Null-terminated input string |
| `_In_reads_(n)` | Input array with element count |
| `_In_reads_bytes_(n)` | Input buffer with byte count |
| `_In_reads_opt_(n)` | Optional input array with element count |
| `_Out_` | Output parameter |
| `_Out_opt_` | Optional output pointer |
| `_Out_writes_(n)` | Output array with element count |
| `_Out_writes_opt_(n)` | Optional output array with element count |
| `_Out_writes_bytes_(n)` | Output buffer with byte count |
| `_Inout_` | Bidirectional parameter |
| `_Inout_updates_all_(n)` | In-place update of entire array |

Example:

```cpp
// Header (DirectXMesh.h)
DIRECTX_MESH_API HRESULT __cdecl ComputeNormals(
    _In_reads_(nFaces * 3) const uint16_t* indices, _In_ size_t nFaces,
    _In_reads_(nVerts) const XMFLOAT3* positions, _In_ size_t nVerts,
    _In_ CNORM_FLAGS flags,
    _Out_writes_(nVerts) XMFLOAT3* normals) noexcept;

// Implementation (.cpp)
_Use_decl_annotations_
HRESULT DirectX::ComputeNormals(
    const uint16_t* indices,
    size_t nFaces,
    const XMFLOAT3* positions,
    size_t nVerts,
    CNORM_FLAGS flags,
    XMFLOAT3* normals) noexcept
{ ... }
```

#### Calling Convention and DLL Export

- All public functions use `__cdecl` explicitly for ABI stability.
- All public function declarations are prefixed with `DIRECTX_MESH_API`, which wraps `__declspec(dllexport)` / `__declspec(dllimport)` (or the MinGW `__attribute__` equivalent) when the `DIRECTX_MESH_EXPORT` or `DIRECTX_MESH_IMPORT` preprocessor symbols are defined. CMake sets these automatically when `BUILD_SHARED_LIBS=ON`.

#### `noexcept` Rules

- All query and utility functions that cannot fail (e.g., `IsValidVB`, `IsValidIB`) are marked `noexcept`.
- All HRESULT-returning I/O and processing functions are also `noexcept` — errors are communicated via return code, never via exceptions.
- Constructors and functions that perform heap allocation or utilize Standard C++ containers that may throw are marked `noexcept(false)`.

#### Enum Flags Pattern

Flags enums follow this pattern — a `uint32_t`-based unscoped enum with a `_NONE = 0x0` base case, followed by a call to `DEFINE_ENUM_FLAG_OPERATORS` (defined in `DirectXMesh.inl`) to enable `|`, `&`, and `~` operators:

```cpp
enum CNORM_FLAGS : uint32_t
{
    CNORM_DEFAULT = 0x0,
    // Default is to compute normals using weight-by-angle

    CNORM_WEIGHT_BY_AREA = 0x1,
    // Computes normals using weight-by-area

    CNORM_WEIGHT_EQUAL = 0x2,
    // Compute normals with equal weights

    CNORM_WIND_CW = 0x4,
    // Vertices are clock-wise (defaults to CCW)
};

DEFINE_ENUM_FLAG_OPERATORS(CNORM_FLAGS);
```

See [this blog post](https://walbourn.github.io/modern-c++-bitmask-types/) for more information on this pattern.

### Patterns to Avoid

- Don’t use raw pointers for ownership.
- Avoid macros for constants—prefer `constexpr` or `inline` `const`.
- Don’t put implementation logic in header files unless using templates, although the SimpleMath library does use an .inl file for performance.
- Avoid using `using namespace` in header files to prevent polluting the global namespace.

## Naming Conventions

| Element | Convention | Example |
| --- | --- | --- |
| Classes / structs | PascalCase | `Meshlet`, `MeshletTriangle` |
| Public functions | PascalCase + `__cdecl` | `ComputeInputLayout` |
| Private data members | `m_` prefix | `m_count` |
| Enum type names | UPPER_SNAKE_CASE | `CNORM_FLAGS` |
| Enum values | UPPER_SNAKE_CASE | `CNORM_DEFAULT` |
| Files | PascalCase | `DirectXMesh.h` |

## File Header Convention

Every source file (`.cpp`, `.h`, etc.) must begin with this block:

```cpp
//-------------------------------------------------------------------------------------
// {FileName}
//
// {One-line description}
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//-------------------------------------------------------------------------------------
```

Section separators within files use:
- Major sections: `//-------------------------------------------------------------------------------------`
- Subsections:   `//---------------------------------------------------------------------------------`

The project does **not** use Doxygen. API documentation is maintained exclusively on the GitHub wiki.

## References

- [Source git repository on GitHub](https://github.com/microsoft/DirectXMesh.git)
- [DirectXMesh documentation git repository on GitHub](https://github.com/microsoft/DirectXMesh.wiki.git)
- [DirectXMesh test suite git repository on GitHub](https://github.com/walbourn/directxmeshtest.wiki.git).
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Microsoft Secure Coding Guidelines](https://learn.microsoft.com/en-us/security/develop/secure-coding-guidelines)
- [CMake Documentation](https://cmake.org/documentation/)
- [VCPKG Documentation](https://learn.microsoft.com/vcpkg/)
- [Games for Windows and the DirectX SDK blog - June 2014](https://walbourn.github.io/directxmesh/)
- [Games for Windows and the DirectX SDK blog - April 2025](https://walbourn.github.io/github-project-updates-2025/)

## No speculation

When creating documentation:

### Document Only What Exists

- Only document features, patterns, and decisions that are explicitly present in the source code.
- Only include configurations and requirements that are clearly specified.
- Do not make assumptions about implementation details.

### Handle Missing Information

- Ask the user questions to gather missing information.
- Document gaps in current implementation or specifications.
- List open questions that need to be addressed.

### Source Material

- Always cite the specific source file and line numbers for documented features.
- Link directly to relevant source code when possible.
- Indicate when information comes from requirements vs. implementation.

### Verification Process

- Review each documented item against source code whenever related to the task.
- Remove any speculative content.
- Ensure all documentation is verifiable against the current state of the codebase.

## Cross-platform Support Notes

- The code supports building for Windows and Linux.
- Portability and conformance of the code is validated by building with Visual C++, clang/LLVM for Windows, MinGW, and GCC for Linux compilers.

### Platform and Compiler `#ifdef` Guards

Use these established guards — do not invent new ones:

| Guard | Purpose |
| --- | --- |
| `_WIN32` | Windows platform (desktop, UWP, Xbox) |
| `_GAMING_XBOX` | Xbox One or Xbox Series X\|S |
| `_GAMING_XBOX_SCARLETT` | Xbox Series X\|S |
| `_XBOX_ONE && _TITLE` | Legacy Xbox One XDK — **no longer supported**; triggers a `#error` at compile time |
| `_MSC_VER` | MSVC-specific (and MSVC-like clang-cl) pragmas and warning suppression |
| `__clang__` | Clang/LLVM diagnostic suppressions |
| `__MINGW32__` | MinGW compatibility headers |
| `__GNUC__` | MinGW/GCC DLL attribute equivalents |
| `_M_ARM64` / `_M_X64` / `_M_IX86` | Architecture-specific code paths for MSVC (`#ifdef`) |
| `_M_ARM64EC` | ARM64EC ABI (ARM64 code with x64 interop) for MSVC |
| `__aarch64__` / `__x86_64__` / `__i386__` | Additional architecture-specific symbols for MinGW/GNUC (`#if`) |
| `USING_DIRECTX_HEADERS` | External DirectX-Headers package in use |

> `_M_ARM`/ `__arm__` is legacy 32-bit ARM which is deprecated.

Non-Windows builds (Linux/WSL) omit WIC entirely and use `<directx/dxgiformat.h>` and `<wsl/winadapter.h>` from the DirectX-Headers package instead of the Windows SDK.

### Error Codes

The following symbols are not custom error codes, but aliases for `HRESULT_FROM_WIN32` error values.

| Symbol | Standard Win32 HRESULT |
| -------- | ------------- |
| `HRESULT_E_ARITHMETIC_OVERFLOW` | `HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW)` |
| `HRESULT_E_NOT_SUPPORTED` | `HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)` |
| `HRESULT_E_INVALID_NAME` | `HRESULT_FROM_WIN32(ERROR_INVALID_NAME)` |

## Code Review Instructions

When reviewing code, focus on the following aspects:

- Adherence to coding standards defined in `.editorconfig` and on the [wiki](https://github.com/microsoft/DirectXTK/wiki/Implementation).
- Make coding recommendations based on the *C++ Core Guidelines*.
- Proper use of RAII and smart pointers.
- Correct error handling practices and C++ Exception safety.
- Clarity and maintainability of the code.
- Adequate comments where necessary.
- Public interfaces located in `DirectXMesh.h` should be clearly defined and documented on the GitHub wiki.
- Compliance with the project's architecture and design patterns.
- Ensure that all public functions and classes are covered by unit tests located on [GitHub](https://github.com/walbourn/directxmeshtest.git) where applicable. Report any gaps in test coverage.
- Check for performance implications, especially in geometry processing algorithms.
- Provide brutally honest feedback on code quality, design, and potential improvements as needed.

## Documentation Review Instructions

When reviewing documentation, do the following:

- Read the code located in [this git repository](https://github.com/microsoft/DirectXMesh.git) in the main branch.
- Review the public interface defined in `DirectXMesh.h`, as well as the headers in the `Utilities` folder.
- Read the documentation on the wiki located in [this git repository](https://github.com/microsoft/DirectXMesh.wiki.git).
- Report any specific gaps in the documentation compared to the public interface.
