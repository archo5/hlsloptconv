
// `one initialized variable`
source `float4 main( float4 p : POSITION ) : POSITION { float a = 1; return a; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `two initialized variables`
source `float4 main( float4 p : POSITION ) : POSITION { float a = 1; float b = 2; return a + b; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `two initialized variables in same decl`
source `float4 main( float4 p : POSITION ) : POSITION { float a = 1, b = 2; return a + b; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `initializer list (basic)`
source `float4 main() : POSITION { float4 x = { 1, 2, 3, 4 }; return x; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `initializer list (elements)`
source `float4 main( float2 p : POSITION ) : POSITION { float4 x = { 3, p, 4 }; return x; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `initializer list (brace spam)`
source `float4 main( float2 p : POSITION ) : POSITION { float4 x = { 3, {{p}}, {{{4},}}, }; return x; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `numeric type ctors (basic)`
source `float4 main() : POSITION { return float4( 1, 2, 3, 4 ) + float( 5 ); }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `numeric type ctors (elements)`
source `float4 main( float2 p : POSITION ) : POSITION { return float4( 3, p, 4 ); }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `numeric type ctors (vm1)`
source `float4 main( float2 p : POSITION ) : POSITION { return float1( 3 ); }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `array type`
source `float4 main() : POSITION { float4 arr[2]; arr[0] = 0; arr[1] = 1; return arr[0] + arr[1]; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `array with initializer`
source `float4 main() : POSITION { float4 arr[2] = { float4(0,2,4,6), float4(1,2,3,4) }; return arr[0] + arr[1]; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `scalar swizzle 1`
source `void main( out float4 OUT : POSITION ){ OUT = 1.0.xxxx; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `scalar swizzle 2`
source `void main( out float4 OUT : POSITION ){ OUT = 1.0.x; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `bad scalar swizzle 1`
source `void main( out float4 OUT : POSITION ){ OUT = 1.0.xxxy; }`
compile_fail ``

// `bad read swizzle 1`
source `void main( in float4 IN : POSITION, out float4 OUT : POSITION ){ OUT = IN.xyzq; }`
compile_fail ``

// `bad read swizzle 2`
source `void main( in float4 IN : POSITION, out float4 OUT : POSITION ){ OUT = IN.xyrg; }`
compile_fail ``

// `bad write swizzle 1`
source `void main( in float4 IN : POSITION, out float4 OUT : POSITION ){ OUT.xyzq = IN; }`
compile_fail ``

// `bad write swizzle 2`
source `void main( in float4 IN : POSITION, out float4 OUT : POSITION ){ OUT.xyzy = IN; }`
compile_fail ``

// `piecewise variable init`
source `float4 main() : POSITION
{
	float4 o;
	o.x = 1.0; o.yz = 0.0; o.w = 1.0;
	return o;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `piecewise variable init 2`
source `float4 main() : POSITION
{
	float4 o;
	o.x = 1.0;
	return o.x;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `piecewise variable init fail`
source `float4 main() : POSITION
{
	float4 o;
	o.xz = 1.0; o.w = 1.0;
	return o;
}`
compile_fail ``

// `piecewise variable init fail 2`
source `float4 main() : POSITION
{
	float4 o;
	o.xz = 1.0; o.w = 1.0;
	return o.y;
}`
compile_fail ``

// `piecewise variable init fail 3`
source `float4 main() : POSITION
{
	float arr[2];
//	arr[0] = 0;
	arr[1] = 1;
	return arr[0] + arr[1];
}`
compile_fail ``

// `matrix RW swizzle x1`
source `float4 main() : POSITION
{
	float4x3 o;
	o._m21 = 0.0;
	return o._32;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float4x3=>float4x4`
compile_glsl_es100 ``

// `matrix RW swizzle x4`
source `float4 main() : POSITION
{
	float4x3 o;
	o._m00_m21_m12_m32 = 0.0;
	return o._11_32_23_43;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float4x3=>float4x4`
compile_glsl_es100 ``

// `struct`
source `
struct tmp { float4 val; };
float4 main() : POSITION { tmp v; v.val = 1; return v.val; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `array in struct`
source `
struct tmp { float4 val[2]; };
float4 main() : POSITION { tmp v; v.val[1] = 1; return v.val[1]; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `basic I/O`
source `float4 main(float4 p : POSITION) : POSITION { return p; }`
request_vars ``
compile_hlsl_before_after ``
in_shader `POSITION`
verify_vars `
VSInput Float32x4 p :POSITION #0
`
compile_hlsl4 ``
in_shader `SV_POSITION`
compile_glsl ``
in_shader `gl_Position`
compile_glsl_es100 ``
in_shader `gl_Position`

// `basic I/O 2 vars`
source `float4 main(float4 p : POSITION, float2 tex : TEXCOORD3) : POSITION { return p + tex.y; }`
request_vars ``
compile_hlsl_before_after ``
in_shader `POSITION`
verify_vars `
VSInput Float32x4 p :POSITION #0
VSInput Float32x2 tex :TEXCOORD #3
`
compile_hlsl4 ``
in_shader `SV_POSITION`
compile_glsl ``
in_shader `gl_Position`
compile_glsl_es100 ``
in_shader `gl_Position`

// `struct I/O 1`
source `
struct v2p { float4 Position : POSITION; };
void main( out v2p OUT )
{
	OUT.Position = 0.0;
}`
compile_hlsl_before_after ``
in_shader `POSITION`
compile_hlsl4 ``
in_shader `SV_POSITION`
compile_glsl ``
in_shader `gl_Position`
compile_glsl_es100 ``
in_shader `gl_Position`

// `struct I/O 2`
source `
struct a2v { float4 Position : POSITION; };
struct v2p { float4 Position : POSITION; };
float4x4 ModelViewMatrix;
void main( in a2v IN, out v2p OUT )
{
	OUT.Position = IN.Position;
}`
request_vars ``
compile_hlsl_before_after ``
in_shader `POSITION`
verify_vars `
VSInput Float32x4 IN_Position :POSITION #0
`
request_vars ``
compile_hlsl4 ``
in_shader `SV_POSITION`
verify_vars `
VSInput Float32x4 IN_Position :POSITION #0
`
request_vars ``
compile_glsl ``
in_shader `gl_Position`
verify_vars `
VSInput Float32x4 IN_Position :POSITION #0
`
request_vars ``
compile_glsl_es100 ``
in_shader `gl_Position`
verify_vars `
VSInput Float32x4 IN_Position :POSITION #0
`

// `struct I/O 3a`
source `
struct vdata { float4 Position : POSITION; };
float4x4 ModelViewMatrix;
void main( in vdata IN, out vdata OUT )
{
	OUT = IN;
}`
request_vars ``
compile_hlsl_before_after ``
in_shader `POSITION`
verify_vars `
VSInput Float32x4 IN_Position :POSITION #0
`
compile_hlsl4 ``
in_shader `SV_POSITION`
compile_glsl ``
in_shader `gl_Position`
compile_glsl_es100 ``
in_shader `gl_Position`

// `struct I/O 3b`
source `
struct vdata { float4 Position : POSITION; };
struct vdataw { vdata data; };
float4x4 ModelViewMatrix;
void main( in vdataw IN, out vdataw OUT )
{
	OUT = IN;
}`
compile_hlsl_before_after ``
in_shader `POSITION`
compile_hlsl4 ``
in_shader `SV_POSITION`
compile_glsl ``
in_shader `gl_Position`
compile_glsl_es100 ``
in_shader `gl_Position`

// `explicit uniform`
source `
uniform float4 outval;
float4 main() : POSITION { return outval; }`
request_vars ``
compile_hlsl_before_after ``
verify_vars `
Uniform Float32x4 outval
`
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `uniform array`
source `
uniform float4 outval[4];
float4 main() : POSITION { return outval[1]; }`
request_vars `0 1`
compile_hlsl_before_after ``
verify_vars `
Uniform Float32x4[4] outval
`
request_vars ``
compile_hlsl4 ``
verify_vars `
Uniform Float32x4[4] outval
`
request_vars ``
compile_glsl ``
verify_vars `
Uniform Float32x4[4] outval
`
request_vars ``
compile_glsl_es100 ``
verify_vars `
Uniform Float32x4[4] outval
`

// `explicit constant`
source `
static const float4 outval1 = {1,2,3,4};
static const float4 outval2 = float4(5,7,9,11);
static const float4 outval3 = outval1 + outval2;
float4 main() : POSITION { return outval1 + outval2 * outval3; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `bad explicit constant`
source `
const float4 outval;
float4 main() : POSITION { return outval; }`
compile_fail ``

// `explicit constant write 1`
source `
static const float4 outval1 = {1,2,3,4};
float4 main() : POSITION { outval1 = 3; return outval1; }`
compile_fail ``

// `explicit constant write 2`
source `
static const float4 outval1 = {1,2,3,4};
float4 main() : POSITION { outval1.y = 2; return outval1; }`
compile_fail ``

// `static variable`
source `
static float4 outval1 = {1,2,3,4};
float4 main() : POSITION { outval1 = 0.5; return outval1; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `basic cbuffer`
source `
cbuffer mybuf
{
	float4 outval;
}
float4 main() : POSITION { return outval; }`
request_vars ``
compile_hlsl_before_after ``
verify_vars `
UniformBlockBegin None mybuf
Uniform Float32x4 outval
UniformBlockEnd None mybuf
`
request_vars ``
compile_hlsl4 ``
verify_vars `
UniformBlockBegin None mybuf
Uniform Float32x4 outval
UniformBlockEnd None mybuf
`
request_vars ``
compile_glsl ``
verify_vars `
UniformBlockBegin None mybuf
Uniform Float32x4 outval
UniformBlockEnd None mybuf
`
request_vars ``
compile_glsl_es100 ``
verify_vars `
UniformBlockBegin None mybuf
Uniform Float32x4 outval
UniformBlockEnd None mybuf
`

// `cbuffer w/ registers`
source `
cbuffer mybuf : register(b3)
{
	float2 outval : packoffset(c3.z);
}
float4 main() : POSITION { return outval.xyxy; }`
compile_hlsl_before_after ``
request_vars ``
compile_hlsl4 ``
verify_vars `
UniformBlockBegin None mybuf #3
Uniform Float32x2 outval #14
UniformBlockEnd None mybuf #3
`
compile_glsl ``
compile_glsl_es100 ``

// `multiple cbuffers`
source `
cbuffer mybuf
{
	float4 outval;
}
cbuffer mybuf2
{
	float4 ou2val[1];
}
float4 main() : POSITION { return outval + ou2val[0]; }`
request_vars ``
compile_hlsl_before_after ``
verify_vars `
UniformBlockBegin None mybuf
Uniform Float32x4 outval
UniformBlockEnd None mybuf
UniformBlockBegin None mybuf2
Uniform Float32x4[1] ou2val
UniformBlockEnd None mybuf2
`
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `multiple cbuffers with locked registers`
source `
cbuffer mybuf
{
	float4 outval;
	float4 outval2;
	float4 outval3;
}
cbuffer mybuf2
{
	float4 ou2val[2];
	float4 ou2val2[3];
	float4 ou2val3[5];
}
float4 main() : POSITION { return outval + outval2 + outval3
	+ ou2val[0] + ou2val2[2] + ou2val3[1]; }`
request_vars ``
request_specify_registers ``
compile_hlsl_before_after ``
verify_vars `
UniformBlockBegin None mybuf #0
Uniform Float32x4 outval #0
Uniform Float32x4 outval2 #4
Uniform Float32x4 outval3 #8
UniformBlockEnd None mybuf #0
UniformBlockBegin None mybuf2 #1
Uniform Float32x4[2] ou2val #0
Uniform Float32x4[3] ou2val2 #8
Uniform Float32x4[5] ou2val3 #20
UniformBlockEnd None mybuf2 #1
`
compile_hlsl4 ``

// `cbuffer containing array of structs`
source `
struct STR
{
	float4 a;
	float4 b[2];
};
cbuffer mybuf
{
	STR base;
	STR extra[3];
}
float4 main() : POSITION { return base.a + base.b[1] + extra[2].a + extra[1].b[0]; }`
request_vars ``
compile_hlsl_before_after ``
verify_vars `
UniformBlockBegin None mybuf
TODO
UniformBlockEnd None mybuf
`
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `samplers`
source `
sampler1D s1;
sampler2D s2;
sampler3D s3;
samplerCUBE sc;
float4 main() : POSITION { return 0; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `comparison samplers`
source `
sampler1Dcmp s1c;
sampler2Dcmp s2c;
samplerCUBEcmp scc;
float4 main() : POSITION { return 0; }`
compile_hlsl4 ``
compile_glsl ``

// `samplers w/ registers`
source `
sampler1D s1 : register(s0);
sampler2D s2 : register(s3);
sampler3D s3 : register(s1);
samplerCUBE sc : register(s6);
float4 main() : POSITION { return 0; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `increment, decrement scalar`
source `float4 main() : POSITION { float a = 1;
	++a; ++a; --a;
	a++; a--; a--;
	return a; }`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `bad increment`
source `float4 main() : POSITION { float a;
	++a; a = 1; return a; }`
compile_fail ``
