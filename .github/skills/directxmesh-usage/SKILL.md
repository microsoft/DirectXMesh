---
name: directxmesh-usage
description: Guide for integrating the DirectXMesh geometry processing library into a C++ project.
---

# Using DirectXMesh in Your Project

This skill provides guidance for integrating the DirectXMesh geometry processing library into a C++ project.

## When to Use

Invoke this skill when:

- Adding DirectXMesh as a dependency to a new or existing project.
- Writing code that processes mesh geometry (normals, tangents, optimization, meshlets).
- Needing to understand the typical DirectXMesh processing pipeline.

## Integration Methods

### vcpkg manifest-mode (Recommended)

In your `vcpkg.json` file, add the following:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "directxmesh"
  ]
}
```

### vcpkg (classic)

```bash
vcpkg install directxmesh
```

Features: `dx12` (DirectX 12 input layout support). Triplets: `x64-windows`, `x64-linux`, `arm64-windows`, etc.

For DLL usage (`x64-windows` default triplet), define `DIRECTX_MESH_IMPORT` in your consuming project.

### CMake (FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
  DirectXMesh
  GIT_REPOSITORY https://github.com/microsoft/DirectXMesh.git
  GIT_TAG main
)
FetchContent_MakeAvailable(DirectXMesh)
target_link_libraries(${PROJECT_NAME} PRIVATE DirectXMesh)
```

Key CMake options: `BUILD_DX12` (ON by default), `BUILD_SHARED_LIBS` (OFF by default).

### NuGet

- **directxmesh_desktop_win10** — Windows desktop (Win10/Win11), includes DX12 and ARM64.
- **directxmesh_uwp** — Universal Windows Platform apps.

### Project Reference

Add the appropriate `.vcxproj` from the `DirectXMesh/` folder to your solution and add a project reference. Add the `DirectXMesh` directory to your Additional Include Directories.

## Header Usage

```cpp
#include <d3d12.h>        // or <d3d11_1.h> — include BEFORE DirectXMesh.h
#include "DirectXMesh.h"

using namespace DirectX;
```

All functions reside in the `DirectX` namespace. Most functions have overloads for both 16-bit and 32-bit index buffers.

## Typical Processing Pipeline

```
Load geometry (positions, indices, texcoords)
    │
    ▼
GenerateAdjacencyAndPointReps  ──→  adjacency[], pointRep[]
    │
    ▼
Validate (optional diagnostics)
    │
    ▼
Clean (fix bowties, degenerate triangles) ──→ dupVerts[]
    │
    ▼
ComputeNormals / ComputeTangentFrame
    │
    ▼
AttributeSort ──→ faceRemap[]
    │
    ▼
OptimizeFaces ──→ faceRemap[]
    │
    ▼
ReorderIB (apply face remap)
    │
    ▼
OptimizeVertices ──→ vertexRemap[]
    │
    ▼
FinalizeIB + FinalizeVB (apply vertex remap and dupVerts)
    │
    ▼
CompactVB (optional, remove trailing unused vertices)
    │
    ▼
ComputeMeshlets ──→ meshlets[], uniqueVertexIB (uint8_t[]), primitiveIndices[]
    │
    ▼
ComputeCullData (for mesh shader GPU culling)
    NOTE: uniqueVertexIB from ComputeMeshlets is a packed buffer of
    uint16_t or uint32_t indices (matching the original IB format).
    Reinterpret-cast it to the appropriate type for ComputeCullData.
```

## Error Handling

- Most functions return `HRESULT`. Check with `SUCCEEDED(hr)` or `FAILED(hr)`.
- Functions marked `noexcept` never throw; they return `E_OUTOFMEMORY` or `E_INVALIDARG` on failure.
- Functions using `std::vector` or `std::function` may throw on allocation failure

## Key References

- **Public API header**: `DirectXMesh/DirectXMesh.h` — read this for exact signatures and SAL annotations.
- **API overview table**: See `reference/overview.md` in this skill folder.
- **Wiki documentation**: https://github.com/microsoft/DirectXMesh/wiki
- **Utility headers**: `Utilities/WaveFrontReader.h` (OBJ file loading), `Utilities/FlexibleVertexFormat.h` (legacy FVF conversion).

## Platform Notes

- Windows desktop (Win8.1+), UWP, Xbox (GDK), and Linux are supported.
- Non-Windows builds require the [DirectX-Headers](https://github.com/microsoft/DirectX-Headers) package and define `USING_DIRECTX_HEADERS`.
- The library requires C++17 to build but the public API is C++11 compatible.
