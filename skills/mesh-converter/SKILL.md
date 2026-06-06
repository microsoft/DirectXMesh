---
name: mesh-converter
description: >-
  Guide for using the meshconvert command-line tool to convert Wavefront OBJ files to DirectX runtime formats (CMO, SDKMESH, VBO).
license: MIT
metadata:
  author: chuckw
  version: "1.0"
---

# Mesh Converter Skill

This skill provides guidance for using the `meshconvert` command-line tool to convert Wavefront OBJ geometry files into runtime formats consumed by DirectX Tool Kit.

## When to Use

Invoke this skill when:

- Converting Wavefront OBJ files to SDKMESH, CMO, or VBO format.
- Preparing mesh geometry for use with DirectX Tool Kit (DirectXTK).
- Generating normals, tangents, or performing vertex-cache optimization on mesh data.
- Understanding the available output formats and their requirements.

## Overview

The `meshconvert` tool imports geometry from Wavefront Object (OBJ) or VBO files and prepares it for runtime use. It can generate normals and tangent frames, perform vertex-cache optimization, and write to formats suited for DirectX applications.

- **Repository**: <https://github.com/microsoft/DirectXMesh>
- **Documentation**: <https://github.com/microsoft/DirectXMesh/wiki/Meshconvert>
- **Installation**: `winget install Microsoft.DirectX.Mesh`

## Installation

### winget (Recommended)

```bash
winget install Microsoft.DirectX.Mesh
```

### vcpkg

```bash
vcpkg install directxmesh[tools]
```

## Basic Syntax

```plaintext
meshconvert [options] [--file-list <filename>] <file-name(s)>
```

The tool uses Windows-style `-` or `/` for options, and also supports GNU long options with `--`.

## Output Formats

| Format | Switch | Description | Requirements |
| --- | --- | --- | --- |
| SDKMESH | `-ft sdkmesh` | Default output format for DirectX Tool Kit `Model` class | None |
| SDKMESH (PBR) | `-ft sdkmesh2` | SDKMESH with PBR materials | None |
| CMO | `-ft cmo` | Visual Studio CMO format for DirectX Tool Kit `Model` class | Normals, tangents, and texture coordinates |
| VBO | `-ft vbo` | Simple vertex buffer object format for DirectX Tool Kit | Normals and texture coordinates |
| OBJ | `-ft obj` | Re-export as Wavefront OBJ | None |

## Common Workflows

### Basic Conversion to SDKMESH (Default)

```bash
meshconvert model.obj
```

Produces `model.sdkmesh` with no additional processing.

### Convert with Normals and Optimization

```bash
meshconvert model.obj -n -op
```

Generates normals (weight by angle) and performs vertex-cache optimization using the Hoppe algorithm.

### Convert to CMO Format

CMO format requires normals, tangents, and texture coordinates:

```bash
meshconvert model.obj -ft cmo -n -tb
```

### Convert to VBO Format

VBO format requires normals and texture coordinates:

```bash
meshconvert model.obj -ft vbo -n
```

### Convert with Full Processing Pipeline

```bash
meshconvert model.obj -n -tb -op -y
```

Generates normals, tangent frames (tangents and bi-tangents), optimizes the vertex cache, and overwrites any existing output file.

### Convert for Direct3D Coordinate System

OBJ files from tools using OpenGL conventions may need coordinate adjustments:

```bash
meshconvert model.obj -n -op --flip-v --flip-z
```

Flips the V texture coordinate and negates Z for right-hand to left-hand conversion.

### Batch Conversion

```bash
meshconvert -r *.obj -n -op -y
```

Recursively processes all OBJ files in the current directory and subdirectories.

Or use a file list:

```bash
meshconvert --file-list models.txt -n -op -y
```

Where `models.txt` contains one filename per line (lines starting with `#` are comments).

## Option Reference

See [reference/options.md](reference/options.md) for the complete command-line options reference.

## Format-Specific Notes

### SDKMESH

- Default output format.
- Supports 16-bit and 32-bit index buffers (auto-selects 16-bit when possible).
- Supports configurable vertex formats for normals, UVs, and colors.
- Used by DirectX Tool Kit's `Model` class via `Model::CreateFromSDKMESH`.

### CMO

- Visual Studio's mesh format.
- **Requires** normals, tangents, and texture coordinates — use `-n -tb` switches.
- Fixed vertex format (no `-fn`, `-fuv`, `-fc`, or `-ib32` options).
- Used by DirectX Tool Kit's `Model` class via `Model::CreateFromCMO`.

### VBO

- Simple binary format with position, normal, and texture coordinates.
- **Requires** normals and texture coordinates — use `-n` switch.
- Fixed vertex format (no `-fn`, `-fuv`, `-fc`, or `-ib32` options).
- Limited to 65535 vertices (16-bit indices only).
- Used by DirectX Tool Kit's `Model` class via `Model::CreateFromVBO`.

## Key References

- [Meshconvert Wiki Page](https://github.com/microsoft/DirectXMesh/wiki/Meshconvert)
- [Geometry Formats](https://github.com/microsoft/DirectXMesh/wiki/Geometry-formats)
- [VBO Format](https://github.com/microsoft/DirectXMesh/wiki/VBO)
- [DirectX Tool Kit Model Class](https://github.com/microsoft/DirectXTK/wiki/Model)
- [DirectX Tool Kit 12 Model Class](https://github.com/microsoft/DirectXTK12/wiki/Model)
