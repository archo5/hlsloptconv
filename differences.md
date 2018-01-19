## Shading language differences

#### Matrices

|            | HLSL | GLSL 1.40 |
| ---------: | ---- | --------- |
|    syntax: | `float2x3` or `matrix<float, 2, 3>` | `mat3x2` |
|   row/col: | `floatROWxCOL` or `matrix<float, ROW, COL>` | `matCOLxROW` |
|  indexing: | `[ROW][COL]` | `[COL][ROW]` |
| swizzling: | `_<ROW><COL>` and `_m<ROW-1><COL-1>` | not supported |
| `1xN/Nx1`: | supported | not supported |
| ctor pack order: | row major | column major |
