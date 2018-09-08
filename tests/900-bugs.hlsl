
// `bug 1 - macros defined even if disabled`
source `
#define HLSL_D3D11
#line 1 "sys_batchvtx"
#line 1 "__eq"
#ifdef HLSL_D3D9
#define NEEDS_TEXTURE_2D( id ) sampler2D Tex###id : register(s###id);
#define TEXSAMPLE_2D( name, tc ) tex2D( name, tc )

#elif defined(HLSL_D3D11)
#define NEEDS_TEXTURE_2D(id)
#define TEXSAMPLE_2D(name, tc) float4(1,1,1,1)
#endif
#line 2 "sys_batchvtx"
NEEDS_TEXTURE_2D(0);
void main( float2 itex : TEXCOORD0, float4 icol : COLOR0
	, out float4 RT0 : COLOR0  )
{
	RT0 = icol * TEXSAMPLE_2D( Tex0, itex );
}
`
compile_hlsl4 `/T ps_4_0`

// `bug 2 - wrong location of macro replacement tokens`
source `
#line 1 "__eq"
#define NEEDS_TEXTURE_2D( id ) ###
#line 2 "sys_batchvtx"
NEEDS_TEXTURE_2D(0);
`
compile_fail `/T ps_3_0`
check_err `__eq:1:32: error: unexpected token: ##
`

// `bug 3 - unexpected token in if expression`
source `
#if !defined(SHADOW_PASS) || SHADOW_PASS_TO_RT
#endif
float4 main() : POSITION { return 0; }
`
compile_hlsl ``

// `bug 4 - unexpected tokens in if expression - '0 diffuseCol'`
source `
float4 main() : POSITION
{
	float3 diffuseCol = 2;
#if defined(MOD_NODIFFCOL) // -- ignore diffuse color
	diffuseCol = float3(1,1,1);
#endif
	return diffuseCol.xyzz;
}`
compile_hlsl ``

// `bug 5 - crash while parsing unknown declref inside intrinsic`
source `
float4 main() : POSITION
{
	float4 ret = 1;
	RT0.rgb = lerp(RT0.rgb, 0.5, 0.5);
	return ret;
}`
compile_fail ``

// `bug 6 - wrong computed preprocessor expression (bad macro replacements)`
source `
#define SHADOW_PASS_TO_RT 0
void main(
#if !defined(SHADOW_PASS) || SHADOW_PASS_TO_RT
	out float4 RT0 : COLOR0
#endif
)
{
	RT0 = 1;
}`
compile_hlsl `/T ps_3_0`

// `bug 7 - source leaking from ifndef`
source `
#define SHADOW_PASS_TO_RT 0
#define PARTICLE

#if __VERTEX_SHADER__
#ifdef PARTICLE
float4 main() : POSITION { return 0; }
#else // PARTICLE
#endif // PARTICLE
#elif __PIXEL_SHADER__
void main
(
	VS2PS input	
#if !defined(SHADOW_PASS) || SHADOW_PASS_TO_RT
	,out float4 RT0 : COLOR0
#endif
#ifndef SHADOW_PASS
	,out float4 RT1 : COLOR1
	,out float4 RT2 : COLOR2
#endif // SHADOW_PASS
){}
#endif
`
compile_hlsl ``

// `bug 8 - not used before sufficient initialization`
source `
struct VS2PS
{
	float3 viewPos : TEXCOORD2;
	float3 N : TEXCOORD3;
	float4 T : TEXCOORD4;
};
void main(out VS2PS vsout, out float4 opos : POSITION0)
{
	vsout.T = 1;
	vsout.N = 1;
	vsout.T = vsout.T;
	vsout.T.xyz = vsout.T.xyz;
	opos = 1;
	vsout.viewPos = 1;
}`
compile_hlsl ``

// `bug 9 - extra parentheses`
source `
float4 main() : POSITION
{
	float3 a = 1;
	a *= ( float3(1,1,1) + 1 );
	a *= (((( float3(1,((1)),1) + 1 ))));
	return a.xyzz;
}`
compile_hlsl ``

// `bug 10 - extra parentheses in preprocessor expression`
source `
#define WAT 3
#if (WAT == 0)
#endif
#if (((((WAT))) == (0)))
#endif
float4 main() : POSITION { return 1; }`
compile_hlsl ``

// `bug 11 - non-function macros with parentheses`
source `
#define WAT (1)
#define WAT2 (notarg)
#define notarg
float4 main WAT2 : POSITION { return WAT; }`
compile_hlsl ``

// `bug 12 - use of 'input' in GLSL is not allowed`
source `
float4 main() : POSITION {
	float input = 1;
	float output = 2;
	return input + output; }`
compile_hlsl ``
compile_glsl ``
compile_glsl_es100 ``

// `bug 13 - lvalue casts from swizzles`
source `
float4 main() : POSITION {
	float o = 0;
	float4 s = 1.0;
	o = s.r;
	o += s.r;
	return o; }`
compile_hlsl ``
compile_glsl ``
compile_glsl_es100 ``

// `bug 14 - matrix used before sufficient initialization/crash`
source `
float4 main() : POSITION {
	float2x2 m1 = float2x2(float2(0.25,0.5), float2(0.75, 1));
	return float4(m1._m00, m1._m01, m1._m10, m1._m11); }`
compile_hlsl ``
