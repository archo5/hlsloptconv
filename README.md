## HLSL Optimizing Converter

#### What is it?

This compiler takes HLSL 3.0/4.0 shader code and converts it to one of the following output formats:

* HLSL 3.0
* HLSL 4.0
* GLSL 1.40
* GLSL ES 1.0 (for WebGL 1)

It has an extensive test suite, including a [HTML5 WebGL 1 demo](http://archo.work/html5-hlsloptconvtest.htm) using a shader that has been compiled from HLSL, and a "four API test" for Windows featuring D3D9, D3D11, GL2 and GL3.1 running the same shader simultaneously:

![Four API test screenshot](http://archo.work/lib/host/hlsloptconv-screenshot-min.png)

The main test suite checks most converted code with `glslangValidator` as well as does a before/after comparison with `fxc`, the DirectX shader compiler, to make sure that the meaning of the code is not lost in translation.

#### What is missing (and may or may not appear later)?

* Parsing support for the following intrinsics: `frexp`, `modf`, `sincos`, `transpose`
* GLSL output support for some renamed intrinsics
* Non-square matrix emulation for GLSL ES 1.0
* Array emulation for GLSL ES 1.0
* Geometry shader support
* Validation of certain syntax constructs such as register notation
* Full semantic remapping support for HLSL 4.0

#### Other differences from HLSL:

* `static const` requires an initialization expression, but it is disallowed to have one for just `const` or other types. This is to avoid creating constants that are not actually initialized in the shader, but just look like they might be.
* `tex1D/tex2D/tex3D/texCUBE` overloads that work same as their `*grad` versions are not recognized.

#### Inherent incompatibilities between shader languages/APIs:

* `pow` intrinsic has different (reduced) output guarantees when converted to GLSL (do not use with `x < 0`).
* [`sampler1D`, `tex1D`] are converted to [`sampler2D`, `texture2D`] for GLSL ES 1.0 (there are no 1D textures)
* `tex3D` is not supported for GLSL ES 1.0 (there are no 3D textures)
* floating point `%` (modulus) works differently in GLSL than in HLSL (see differences.md), though the main guarantee (defined values when both signs are equal) still holds

#### Other planned improvements:

* C API
* possibility to predetermine and export location of uniform data
* removal of exceptions
* removal of STL to improve compile times
* improved optimization capabilities
