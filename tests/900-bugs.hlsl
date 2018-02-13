
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
check_err `__eq:1:32: error: unexpected token: #
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
