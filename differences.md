## Shading language differences

#### Syntax elements

|                       | HLSL 3.0 | GLSL 1.40 | GLSL ES 1.0 |
| -----------: | -------- | --------- | ----------- |
|  array init: | remapped init list | element ctor | not supported |
| struct init: | remapped init list | member ctor | member ctor |

#### Matrices

|            | HLSL 3.0 | GLSL 1.40 | GLSL ES 1.0 |
| ---------: | -------- | --------- | ----------- |
|  syntax 1: | `float2x3` or `matrix<float, 2, 3>` | `mat3x2` | not supported |
|  syntax 2: | `float4x4` or `matrix<float, 4, 4>` | `mat4` | `mat4` |
|   row/col: | `floatROWxCOL` or `matrix<float, ROW, COL>` | `matWIDTH` or `matCOLxROW` | `matWIDTH` |
|  indexing: | `[ROW][COL]` | `[COL][ROW]` | `[COL][ROW]` |
| swizzling: | `_<ROW><COL>` and `_m<ROW-1><COL-1>` | not supported | not supported |
| `1xN/Nx1`: | supported | not supported | not supported |
| ctor pack order: | row major | column major | column major |
