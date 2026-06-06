# Meshconvert Options Reference

Complete reference for all `meshconvert` command-line options.

## File Options

| Option | Long Form | Description |
| --- | --- | --- |
| `-r` | | Recursive wildcard search in subdirectories |
| `-flist <file>` | `--file-list <file>` | Read input filenames from a text file |
| `-o <file>` | | Specify output filename (single input only) |
| `-l` | `--to-lowercase` | Force output path and filename to lowercase |
| `-y` | `--overwrite` | Overwrite existing output files |

## Normal and Tangent Options

| Option | Long Form | Description |
| --- | --- | --- |
| `-n` | `--normal-by-angle` | Generate normals weighted by angle |
| `-na` | `--normal-by-area` | Generate normals weighted by area |
| `-ne` | `--normal-by-equal` | Generate normals with equal weights |
| `-t` | `--tangents` | Generate tangents |
| `-tb` | `--tangent-frame` | Generate tangents and bi-tangents |
| `-fn <fmt>` | `--normal-format <fmt>` | Normal format: `float3`, `float16_4`, `r11g11b10` (SDKMESH only) |
| `-cw` | `--clockwise` | Faces are clockwise (default is counter-clockwise) |

## Coordinate and Winding Options

| Option | Long Form | Description |
| --- | --- | --- |
| `-flip` | `--flip-face-winding` | Reverse face winding |
| | `--flip-u` | Invert U texture coordinate |
| | `--flip-v` | Invert V texture coordinate |
| | `--flip-z` | Negate Z component (RH to LH conversion) |

## Vertex and Index Options

| Option | Long Form | Description |
| --- | --- | --- |
| `-ib32` | `--index-buffer-32-bit` | Force 32-bit indices (SDKMESH only) |
| `-fuv <fmt>` | `--uv-format <fmt>` | UV format: `float2`, `float16_2` (SDKMESH only) |
| `-fc <fmt>` | `--color-format <fmt>` | Color format: `bgra`, `rgba`, `float4`, `float16_4`, `rgba_10`, `r11g11b10` (SDKMESH only) |

## Optimization Options

| Option | Long Form | Description |
| --- | --- | --- |
| `-op` | `--optimize` | Vertex cache optimize using Hoppe algorithm (implies `-c`) |
| `-oplru` | `--optimize-lru` | Vertex cache optimize using Forsyth algorithm (implies `-c`) |
| `-c` | `--clean` | Clean mesh (vertex duplication for attribute sets) |
| `-ta` | `--topological-adjacency` | Use topological adjacency (default) |
| `-ga` | `--geometric-adjacency` | Use geometric adjacency (epsilon 1e-5f) |

## Output Format Options

| Option | Long Form | Description |
| --- | --- | --- |
| `-ft <type>` | `--file-type <type>` | Output format: `sdkmesh`, `sdkmesh2`, `cmo`, `vbo`, `obj` |
| `-nodds` | | Prevent renaming texture extensions to .dds in materials |

## Miscellaneous

| Option | Description |
| --- | --- |
| `-nologo` | Suppress copyright message |
| `--version` | Display version information |
| `--help` | Display help |
