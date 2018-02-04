## Shading language differences

#### Syntax elements

|                     | HLSL 3.0/4.0 | GLSL 1.40 | GLSL ES 1.0 |
| ------------------: | ------------ | --------- | ----------- |
|         array init: | remapped init list | element ctor | not supported |
|        struct init: | remapped init list | member ctor | member ctor |
|         type casts: | available | unavailable (use ctor) | unavailable (use ctor) |
|   array index type: | float/int (do NOT cast to int, it adds instructions) | int | int |
| shader i/o linkage: | by semantic match | by variable name match | by variable name match |
|        derivatives: | available | available | optional, ext:OES_standard_derivatives (WebGL) |

#### Texture sampling

`s` - sampler, `t` - texture, `c` - coord, `gx` - ddx(coord), `gy` - ddy(coord), `l` - LOD, `b` - bias, `*` - any value

Integer coordinate offsets are not considered here.

|                     | HLSL 3.0 | HLSL 4.0 | GLSL 1.40 | GLSL ES 1.0 |
| ---------: | -------- | -------- | --------- | ----------- |
|        1D: | `tex1D(s,c)` | `t.Sample(s,c)` | `texture(s,c)` | not supported |
|   bias 1D: | `tex1Dbias(s,{c,*,*,b})` | `t.SampleBias(s,c,b)` | `texture(s,c,b)` | not supported |
|   grad 1D: | `tex1Dgrad(s,c,gx,gy)` | `t.SampleGrad(s,c,gx,gy)` | `textureGrad(s,c,gx,gy)` | not supported |
|    LOD 1D: | `tex1Dlod(s,{c,*,*,l})` | `t.SampleLevel(s,c,l)` | `textureLod(s,c,l)` | not supported |
|   proj 1D: | `tex1Dproj(s,c)` <sup>**(1)**</sup> | `t.Sample(s,c.x/c.w)` <sup>**(2)**</sup> | `textureProj(s,c)` <sup>**(1)**</sup> | not supported |
| | | | |
|        2D: | `tex2D(s,c)` | `t.Sample(s,c)` | `texture(s,c)` | `texture2D(s,c)` |
|   bias 2D: | `tex2Dbias(s,{c,*,b})` | `t.SampleBias(s,c,b)` | `texture(s,c,b)` | `texture2D(s,c,b)` |
|   grad 2D: | `tex2Dgrad(s,c,gx,gy)` | `t.SampleGrad(s,c,gx,gy)` | `textureGrad(s,c,gx,gy)` | `texture2DGradEXT(s,c,gx,gy)` <sup>**(3)**</sup> |
|    LOD 2D: | `tex2Dlod(s,{c,*,l})` | `t.SampleLevel(s,c,l)` | `textureLod(s,c,l)` | `texture2DLodEXT(s,c,l)` <sup>**(3)**</sup>, `texture2DLod(s,c,l)` <sup>**(4)**</sup> |
|   proj 2D: | `tex2Dproj(s,c)` <sup>**(1)**</sup> | `t.Sample(s,c.xy/c.w)` <sup>**(2)**</sup> | `textureProj(s,c)` <sup>**(1)**</sup> | `texture2DProj(s,c)` <sup>**(1)**</sup> |
| | | | |
|        3D: | `tex3D(s,c)` | `t.Sample(s,c)` | `texture(s,c)` | not supported |
|   bias 3D: | `tex3Dbias(s,{c,b})` | `t.SampleBias(s,c,b)` | `texture(s,c,b)` | not supported |
|   grad 3D: | `tex3Dgrad(s,c,gx,gy)` | `t.SampleGrad(s,c,gx,gy)` | `textureGrad(s,c,gx,gy)` | not supported |
|    LOD 3D: | `tex3Dlod(s,{c,l})` | `t.SampleLevel(s,c,l)` | `textureLod(s,c,l)` | not supported |
|   proj 3D: | `tex3Dproj(s,c)` <sup>**(1)**</sup> | `t.Sample(s,c.xyz/c.w)` <sup>**(2)**</sup> | `textureProj(s,c)` <sup>**(1)**</sup> | not supported |
| | | | |
|      Cube: | `texCUBE(s,c)` | `t.Sample(s,c)` | `texture(s,c)` | `textureCube(s,c)` |
| bias Cube: | `texCUBEbias(s,{c,*,b})` | `t.SampleBias(s,c,b)` | `texture(s,c,b)` | `textureCube(s,c,b)` |
| grad Cube: | `texCUBEgrad(s,c,gx,gy)` | `t.SampleGrad(s,c,gx,gy)` | `textureGrad(s,c,gx,gy)` | `textureCubeGradEXT(s,c,gx,gy)` <sup>**(3)**</sup> |
|  LOD Cube: | `texCUBElod(s,{c,*,l})` | `t.SampleLevel(s,c,l)` | `textureLod(s,c,l)` | `textureCubeLodEXT(s,c,l)` <sup>**(3)**</sup>, `textureCubeLod(s,c,l)` <sup>**(4)**</sup> |
| proj Cube: | `texCUBEproj(s,c)` <sup>**(1)**</sup> | `t.Sample(s,c.xyz/c.w)` <sup>**(2)**</sup> | `texture(s,c.xyz/c.w)` <sup>**(2)**</sup> | `textureCube(s,c.xyz/c.w)` <sup>**(2)**</sup> |

**(1)** In HLSL 3 projective coordinates are always divided by c.w, in GLSL the denominator can also be the one-after-last coordinate (y for 1D, z for 2D).

**(2)** No exact intrinsic for projective texturing, but can be easily emulated.

**(3)** `EXT_shader_texture_lod` is required.

**(4)** Only supported in a vertex shader.

Vertex shader availability:

* HLSL3: `*lod`
* HLSL4: `*grad` (`SampleGrad`), `*lod` (`SampleLevel`)
* GLSL: `*grad`, `*lod`

#### Misc. intrinsics/operators

|                             | HLSL 3.0/4.0 | GLSL 1.40 | GLSL ES 1.0 |
| --------------------------: | ------------ | --------- | ----------- |
|                int modulus: | `a % b` (only defined with equal signs) | `a % b` (only defined with equal signs) | not supported |
|         float modulus w/ %: | only defined with equal signs | not supported | not supported |
|         float mod. intrin.: | `r = fmod(a,b)` | `r = mod(a,b)` | `r = mod(a,b)` |
|           float mod. rules: | `a = i * b + r`, `i = int(i)`, `sign(r) = sign(a)`, `abs(r) < abs(b)` | defined by equation | defined by equation |
|        float mod. equation: | `x - y * trunc(x/y)` | `x - y * floor(x/y)` | `x - y * floor(x/y)` |
| | | |
| componentwise matrix mult.: | `mtx1 * mtx2` | `matrixCompMult(mtx1, mtx2)` | `matrixCompMult(mtx1, mtx2)` |
|       regular matrix mult.: | `mul(mtx1, mtx2)` | `mtx1 * mtx2` | `mtx1 * mtx2` |
|      matrix-vector product: | `mul(mtx, vec)` | `mtx * vec` | `mtx * vec` |
| | | |
|    fractional part intrin.: | `frac` | `fract` | `fract` |

#### Linkage

|                             | HLSL 3.0 | HLSL 4.0 | GLSL 1.40 | GLSL ES 1.0 |
| --------------------------: | -------- | -------- | --------- | ----------- |
|      vertex data -> shader: | semantic name & index | byte offset, validated by semantic name, index and order | attribute (assigned by name outside shader) | attribute (assigned by name outside shader) |
|  vertex shader -> position: | `POSITION` semantic | `SV_Position` semantic | `gl_Position` built-in var. | `gl_Position` built-in var. |
|           shader -> shader: | semantic name & index | byte offset, validated by semantic name, index and order | name, validated by type | name, validated by type |
|    pixel shader -> color #: | `COLOR` semantic (#=index) | `SV_Target` semantic (#=index) | `out` variable (# assigned outside shader, default=0) | `gl_FragColor` (#=0, MRT feature is not supported) |

#### Matrices

|            | HLSL 3.0/4.0 | GLSL 1.40 | GLSL ES 1.0 |
| ---------: | ------------ | --------- | ----------- |
|  syntax 1: | `float2x3` or `matrix<float, 2, 3>` | `mat3x2` | not supported |
|  syntax 2: | `float4x4` or `matrix<float, 4, 4>` | `mat4` | `mat4` |
|   row/col: | `floatROWxCOL` or `matrix<float, ROW, COL>` | `matWIDTH` or `matCOLxROW` | `matWIDTH` |
|  indexing: | `[ROW][COL]` | `[COL][ROW]` | `[COL][ROW]` |
| swizzling: | `_<ROW><COL>` and `_m<ROW-1><COL-1>` | not supported | not supported |
| `1xN/Nx1`: | supported | not supported | not supported |
| ctor pack order: | row major | column major | column major |
