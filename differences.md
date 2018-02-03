## Shading language differences

#### Syntax elements

|                     | HLSL 3.0 | GLSL 1.40 | GLSL ES 1.0 |
| ------------------: | -------- | --------- | ----------- |
|         array init: | remapped init list | element ctor | not supported |
|        struct init: | remapped init list | member ctor | member ctor |
|         type casts: | available | unavailable (use ctor) | unavailable (use ctor) |
|   array index type: | float/int (do NOT cast to int, it adds instructions) | int | int |
| shader i/o linkage: | by semantic match | by variable name match | by variable name match |
|        derivatives: | available | available | optional, ext:OES_standard_derivatives (WebGL) |

#### Intrinsics/operators

|                             | HLSL 3.0 | GLSL 1.40 | GLSL ES 1.0 |
| --------------------------: | -------- | --------- | ----------- |
|                int modulus: | `a % b` (only defined with equal signs) | `a % b` (only defined with equal signs) | not supported |
|         float modulus w/ %: | only defined with equal signs | not supported | not supported |
|         float mod. intrin.: | `r = fmod(a,b)` | `r = mod(a,b)` | `r = mod(a,b)` |
|           float mod. rules: | `a = i * b + r`, `i = int(i)`, `sign(r) = sign(a)`, `abs(r) < abs(b)` | defined by equation | defined by equation |
|        float mod. equation: | `x - y * trunc(x/y)` | `x - y * floor(x/y)` | `x - y * floor(x/y)` |
| componentwise matrix mult.: | `mtx1 * mtx2` | `matrixCompMult(mtx1, mtx2)` | `matrixCompMult(mtx1, mtx2)` |
|       regular matrix mult.: | `mul(mtx1, mtx2)` | `mtx1 * mtx2` | `mtx1 * mtx2` |
|      matrix-vector product: | `mul(mtx, vec)` | `mtx * vec` | `mtx * vec` |
|    fractional part intrin.: | `frac` | `fract` | `fract` |

#### Linkage

|                             | HLSL 3.0 | HLSL 4.0 | GLSL 1.40 | GLSL ES 1.0 |
| --------------------------: | -------- | -------- | --------- | ----------- |
|      vertex data -> shader: | semantic name & index | byte offset, validated by semantic name, index and order | attribute (assigned by name outside shader) | attribute (assigned by name outside shader) |
|  vertex shader -> position: | `POSITION` semantic | `SV_Position` semantic | `gl_Position` built-in var. | `gl_Position` built-in var. |
|           shader -> shader: | semantic name & index | byte offset, validated by semantic name, index and order | name, validated by type | name, validated by type |
|    pixel shader -> color #: | `COLOR` semantic (#=index) | `SV_Target` semantic (#=index) | `out` variable (# assigned outside shader, default=0) | `gl_FragColor` (#=0, MRT feature is not supported) |

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
