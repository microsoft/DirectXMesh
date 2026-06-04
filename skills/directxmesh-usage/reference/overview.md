# DirectXMesh API Overview

All functions are in the `DirectX` namespace. Most have overloads for both `uint16_t` and `uint32_t` index buffers. See `DirectXMesh/DirectXMesh.h` for exact signatures.

## Functions

| Function | Category | Description |
| --- | --- | --- |
| `IsValidVB` | Format Utilities | Returns true if a DXGI_FORMAT is valid for use in a vertex buffer. |
| `IsValidIB` | Format Utilities | Returns true if a DXGI_FORMAT is valid for use in an index buffer (R16_UINT or R32_UINT). |
| `BytesPerElement` | Format Utilities | Returns the byte size of a single element for a given DXGI_FORMAT. |
| `IsValid` | Input Layout | Validates a D3D11 or D3D12 input layout descriptor. |
| `ComputeInputLayout` | Input Layout | Computes byte offsets and strides for each element in an input layout. |
| `ComputeSubsets` | Attributes | Returns a list of (offset, count) pairs for contiguous attribute groups in a sorted face list. |
| `ComputeVertexCacheMissRate` | Diagnostics | Computes ACMR and ATVR metrics for a given index buffer and cache size. |
| `GenerateAdjacencyAndPointReps` | Adjacency | Builds face adjacency and/or point representative arrays from positions. Epsilon=0 for topological, >0 for geometric. |
| `ConvertPointRepsToAdjacency` | Adjacency | Converts point representatives to face adjacency. |
| `GenerateGSAdjacency` | Adjacency | Produces a 6-index-per-face IB for use with geometry shader adjacency topology. |
| `ComputeNormals` | Geometry | Computes per-vertex normals from indexed triangle positions. Supports weight-by-angle, weight-by-area, or equal weight. |
| `ComputeTangentFrame` | Geometry | Computes per-vertex tangents and/or bitangents from positions, normals, and texcoords. Overload with XMFLOAT4 tangent stores handedness in .w. |
| `Validate` | Validation | Checks mesh for common problems (degenerate triangles, bowties, asymmetric adjacency). Returns S_OK if valid. |
| `Clean` | Cleanup | Repairs mesh issues by splitting vertices; outputs dupVerts array for later use with FinalizeVB. |
| `WeldVertices` | Utilities | Merges duplicate vertices using a caller-supplied comparison predicate. Produces a vertex remap. |
| `ConcatenateMesh` | Utilities | Computes face/vertex destination maps for merging multiple meshes into one. |
| `AttributeSort` | Optimization | Reorders faces by material/attribute ID; produces a face remap. |
| `OptimizeFaces` | Optimization | Reorders faces for post-transform vertex cache efficiency (Hoppe's algorithm). Requires adjacency. |
| `OptimizeFacesLRU` | Optimization | Reorders faces using an LRU vertex cache simulation. Does not require adjacency. |
| `OptimizeFacesEx` | Optimization | Per-attribute-group version of OptimizeFaces. |
| `OptimizeFacesLRUEx` | Optimization | Per-attribute-group version of OptimizeFacesLRU. |
| `OptimizeVertices` | Optimization | Reorders vertices in order of first use; produces a vertex remap. Optionally reports trailing unused count. |
| `ReorderIB` | Remap | Applies a face remap to an index buffer (in-place or copy). |
| `ReorderIBAndAdjacency` | Remap | Applies a face remap to both an index buffer and adjacency array. |
| `FinalizeIB` | Remap | Applies a vertex remap to an index buffer (in-place or copy). |
| `FinalizeVB` | Remap | Applies a vertex remap and/or vertex duplications to a vertex buffer. |
| `FinalizeVBAndPointReps` | Remap | Same as FinalizeVB but also remaps point representatives. |
| `CompactVB` | Remap | Copies a vertex buffer while removing trailing unused vertices. |
| `ComputeMeshlets` | Meshlets | Generates meshlets from indexed triangles. Optional adjacency improves locality. Subset overload available. |
| `ComputeCullData` | Meshlets | Computes per-meshlet bounding sphere and normal cone for GPU culling. |

## Classes

| Class | Description |
| --- | --- |
| `VBReader` | Reads individual vertex elements by semantic name from one or more vertex buffer streams. Initialize with input layout, add stream(s), then Read(). |
| `VBWriter` | Writes individual vertex elements by semantic name into one or more vertex buffer streams. Initialize with input layout, add stream(s), then Write(). |

## Enums (Flags)

| Enum | Values | Used By |
| --- | --- | --- |
| `CNORM_FLAGS` | `CNORM_DEFAULT`, `CNORM_WEIGHT_BY_AREA`, `CNORM_WEIGHT_EQUAL`, `CNORM_WIND_CW` | `ComputeNormals` |
| `VALIDATE_FLAGS` | `VALIDATE_DEFAULT`, `VALIDATE_BACKFACING`, `VALIDATE_BOWTIES`, `VALIDATE_DEGENERATE`, `VALIDATE_UNUSED`, `VALIDATE_ASYMMETRIC_ADJ` | `Validate` |
| `MESHLET_FLAGS` | `MESHLET_DEFAULT`, `MESHLET_WIND_CW` | `ComputeCullData` |
| `OPTFACES` | `OPTFACES_V_DEFAULT` (12), `OPTFACES_R_DEFAULT` (7), `OPTFACES_LRU_DEFAULT` (32), `OPTFACES_V_STRIPORDER` (0) | `OptimizeFaces`, `OptimizeFacesLRU` |

## Structs

| Struct | Fields | Used By |
| --- | --- | --- |
| `Meshlet` | `VertCount`, `VertOffset`, `PrimCount`, `PrimOffset` | `ComputeMeshlets`, `ComputeCullData` |
| `MeshletTriangle` | `i0:10`, `i1:10`, `i2:10` (packed uint32_t) | `ComputeMeshlets`, `ComputeCullData` |
| `CullData` | `BoundingSphere`, `NormalCone` (XMUBYTEN4), `ApexOffset` | `ComputeCullData` |

## Constants

| Constant | Value | Description |
| --- | --- | --- |
| `MESHLET_DEFAULT_MAX_VERTS` | 128 | Default maximum vertices per meshlet. |
| `MESHLET_DEFAULT_MAX_PRIMS` | 128 | Default maximum primitives per meshlet. |
| `MESHLET_MINIMUM_SIZE` | 32 | Minimum allowed meshlet size. |
| `MESHLET_MAXIMUM_SIZE` | 256 | Maximum allowed meshlet size. |

## Utility Headers (in Utilities/)

| Header | Description |
| --- | --- |
| `WaveFrontReader.h` | Template class `WaveFrontReader<index_t>` for loading Wavefront .OBJ files into memory. Provides positions, normals, texcoords, indices, attributes, and material info. |
| `FlexibleVertexFormat.h` | Converts legacy Direct3D Fixed-Function Vertex formats (FVF codes) to D3D11 input element descriptions. |
