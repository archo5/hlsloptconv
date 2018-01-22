
// `one initialized variable`
source `float4 main( float4 p : POSITION ) : POSITION { float a = 1; return a; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `two initialized variables`
source `float4 main( float4 p : POSITION ) : POSITION { float a = 1; float b = 2; return a + b; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `two initialized variables in same decl`
source `float4 main( float4 p : POSITION ) : POSITION { float a = 1, b = 2; return a + b; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `initializer list (basic)`
source `float4 main() : POSITION { float4 x = { 1, 2, 3, 4 }; return x; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `initializer list (elements)`
source `float4 main( float2 p : POSITION ) : POSITION { float4 x = { 3, p, 4 }; return x; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `initializer list (brace spam)`
source `float4 main( float2 p : POSITION ) : POSITION { float4 x = { 3, {{p}}, {{{4},}}, }; return x; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `numeric type ctors (basic)`
source `float4 main() : POSITION { return float4( 1, 2, 3, 4 ) + float( 5 ); }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `numeric type ctors (elements)`
source `float4 main( float2 p : POSITION ) : POSITION { return float4( 3, p, 4 ); }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `numeric type ctors (vm1)`
source `float4 main( float2 p : POSITION ) : POSITION { return float1( 3 ); }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `array type`
source `float4 main() : POSITION { float4 arr[2]; arr[0] = 0; arr[1] = 1; return arr[0] + arr[1]; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `array with initializer`
source `float4 main() : POSITION { float4 arr[2] = { float4(0,2,4,6), float4(1,2,3,4) }; return arr[0] + arr[1]; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `scalar swizzle 1`
source `void main( out float4 OUT : POSITION ){ OUT = 1.0.xxxx; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `scalar swizzle 2`
source `void main( out float4 OUT : POSITION ){ OUT = 1.0.x; }`
compile_hlsl_before_after ``
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
compile_glsl ``
source_replace `float4x3=>float4x4`
compile_glsl_es100 ``

// `struct I/O 1`
source `
struct v2p { float4 Position : POSITION; };
void main( out v2p OUT )
{
	OUT.Position = 0.0;
}`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `struct I/O 2`
source `
struct a2v { float4 Position : POSITION; };
struct v2p { float4 Position : POSITION; };
float4x4 ModelViewMatrix;
void main( in a2v IN, out v2p OUT )
{
	OUT.Position = IN.Position;
}`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `struct I/O 3a`
source `
struct vdata { float4 Position : POSITION; };
float4x4 ModelViewMatrix;
void main( in vdata IN, out vdata OUT )
{
	OUT = IN;
}`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

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
compile_glsl ``
compile_glsl_es100 ``

// `explicit uniform`
source `
uniform float4 outval;
float4 main() : POSITION { return outval; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `uniform array`
source `
uniform float4 outval[4];
float4 main() : POSITION { return outval[1]; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `explicit constant`
source `
static const float4 outval1 = {1,2,3,4};
static const float4 outval2 = float4(5,7,9,11);
static const float4 outval3 = outval1 + outval2;
float4 main() : POSITION { return outval1 + outval2 * outval3; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `bad explicit constant`
source `
const float4 outval;
float4 main() : POSITION { return outval; }`
compile_fail ``

// `basic cbuffer`
source `
cbuffer mybuf
{
	float4 outval;
}
float4 main() : POSITION { return outval; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `cbuffer w/ registers`
source `
cbuffer mybuf : register(b3)
{
	float2 outval : packoffset(c3.z);
}
float4 main() : POSITION { return outval.xyxy; }`
compile_hlsl_before_after ``
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
	float4 ou2val;
}
float4 main() : POSITION { return outval + ou2val; }`
compile_hlsl_before_after ``
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
compile_glsl ``
compile_glsl_es100 ``

// `samplers w/ registers`
source `
sampler1D s1 : register(s0);
sampler2D s2 : register(s3);
sampler3D s3 : register(s1);
samplerCUBE sc : register(s6);
float4 main() : POSITION { return 0; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `increment, decrement scalar`
source `float4 main() : POSITION { float a = 1;
	++a; ++a; --a;
	a++; a--; a--;
	return a; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `bad increment`
source `float4 main() : POSITION { float a;
	++a; a = 1; return a; }`
compile_fail ``
