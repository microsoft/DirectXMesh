---
name: directxmesh-usage
description: Guide for integrating the DirectXMesh geometry processing library into a C++ project.
---

# DirectXMesh Usage Guide

This skill provides guidance for integrating the DirectXMesh geometry processing library into a C++ project.

## When to Use

Invoke this skill when:

- Adding DirectXMesh as a dependency to a new or existing project.
- Writing code that processes mesh geometry (normals, tangents, optimization, meshlets).
- Needing to understand the typical DirectXMesh processing pipeline.

## Overview

DirectXMesh is a geometry processing library for Direct3D 11 and Direct3D 12 applications. It provides support for loading and saving mesh data, and performing various geometry processing operations including normal and tangent computation, adjacency generation, and mesh optimization.

- **Repository**: <https://github.com/microsoft/DirectXMesh>
- **Documentation**: <https://github.com/microsoft/DirectXMesh/wiki>
- **NuGet Packages**: `directxmesh_desktop_win10`, `directxmesh_uwp`
- **vcpkg Port**: `directxmesh`

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

Features: `dx12` (DirectX 12 input layout support), `tools` (command-line tools). Triplets: `x64-windows`, `x64-linux`, `arm64-windows`, etc.

For DLL usage (`x64-windows` default triplet), define `DIRECTX_MESH_IMPORT` in your consuming project. For static library usage, use `-static-md` triplet variants.

CMakeLists.txt:

```cmake
find_package(directxmesh CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectXMesh)
```

### NuGet

Use `directxmesh_desktop_win10` for Win32 desktop applications or `directxmesh_uwp` for UWP apps.

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

## Further Reading

- [Getting Started](https://github.com/microsoft/DirectXMesh/wiki/Getting-Started)
- [DirectXMesh Wiki](https://github.com/microsoft/DirectXMesh/wiki)
- [API Reference](reference/overview.md)
