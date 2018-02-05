
// `tex1D`
source `
sampler1D Sampler;
float4 main(float p : TEXCOORD) : COLOR
{
	return tex1D(Sampler, 4) + tex1D(Sampler, true) + tex1D(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1D(`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample(`
compile_glsl `-S frag`
in_shader `texture(`
compile_glsl_es100 `-S frag`
in_shader `texture2D(`

// `tex1Dbias`
source `
sampler1D Sampler;
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex1Dbias(Sampler, 4) + tex1Dbias(Sampler, true)
		+ tex1Dbias(Sampler, 0.5) + tex1Dbias(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1Dbias(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleBias(`
compile_glsl `-S frag`
in_shader `texture(`
compile_glsl_es100 `-S frag`
in_shader `texture2D(`

// `tex1Dgrad`
source `
sampler1D Sampler;
float4 main(float p : TEXCOORD) : COLOR
{
	return tex1Dgrad(Sampler, 4, 5, 6) + tex1Dgrad(Sampler, true, false, true)
		+ tex1Dgrad(Sampler, 0.5, 0.6, 0.7) + tex1Dgrad(Sampler, p, p, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1Dgrad(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleGrad(`
compile_glsl `-S frag`
in_shader `textureGrad(`
compile_glsl_es100 `-S frag`
in_shader `texture2DGradEXT(`

// `tex1Dlod (VS)`
source `
sampler1D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : POSITION
{
	return tex1Dlod(Sampler, 4) + tex1Dlod(Sampler, true)
		+ tex1Dlod(Sampler, 0.5) + tex1Dlod(Sampler, p);
}`
compile_hlsl_before_after ``
in_shader `tex1Dlod(`
compile_hlsl4 ``
in_shader `SampleLevel(`
compile_glsl ``
in_shader `textureLod(`
compile_glsl_es100 ``
in_shader `texture2DLod(`

// `tex1Dlod (PS)`
source `
sampler1D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex1Dlod(Sampler, 4) + tex1Dlod(Sampler, true)
		+ tex1Dlod(Sampler, 0.5) + tex1Dlod(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1Dlod(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleLevel(`
compile_glsl `-S frag`
in_shader `textureLod(`
compile_glsl_es100 `-S frag`
in_shader `texture2DLodEXT(`

// `tex1Dproj`
source `
sampler1D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex1Dproj(Sampler, 4) + tex1Dproj(Sampler, true)
		+ tex1Dproj(Sampler, 0.5) + tex1Dproj(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1Dproj(`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample(`
compile_glsl `-S frag`
in_shader `textureProj(`
compile_glsl_es100 `-S frag`
in_shader `texture2DProj(`

// `tex2D`
source `
sampler2D Sampler;
float4 main(float2 p : TEXCOORD) : COLOR
{
	return tex2D(Sampler, 4) + tex2D(Sampler, true)
		+ tex2D(Sampler, 0.5) + tex2D(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2D(`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample(`
compile_glsl `-S frag`
in_shader `texture(`
compile_glsl_es100 `-S frag`
in_shader `texture2D(`

// `tex2Dbias`
source `
sampler2D Sampler;
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex2Dbias(Sampler, 4) + tex2Dbias(Sampler, true)
		+ tex2Dbias(Sampler, 0.5) + tex2Dbias(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2Dbias(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleBias(`
compile_glsl `-S frag`
in_shader `texture(`
compile_glsl_es100 `-S frag`
in_shader `texture2D(`

// `tex2Dgrad`
source `
sampler2D Sampler;
float4 main(float2 p : TEXCOORD) : COLOR
{
	return tex2Dgrad(Sampler, 4, 5, 6) + tex2Dgrad(Sampler, true, false, true)
		+ tex2Dgrad(Sampler, 0.5, 0.6, 0.7) + tex2Dgrad(Sampler, p, p, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2Dgrad(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleGrad(`
compile_glsl `-S frag`
in_shader `textureGrad(`
compile_glsl_es100 `-S frag`
in_shader `texture2DGradEXT(`

// `tex2Dlod (VS)`
source `
sampler2D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : POSITION
{
	return tex2Dlod(Sampler, 4) + tex2Dlod(Sampler, true)
		+ tex2Dlod(Sampler, 0.5) + tex2Dlod(Sampler, p);
}`
compile_hlsl_before_after ``
in_shader `tex2Dlod(`
compile_hlsl4 ``
in_shader `SampleLevel(`
compile_glsl ``
in_shader `textureLod(`
compile_glsl_es100 ``
in_shader `texture2DLod(`

// `tex2Dlod (PS)`
source `
sampler2D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex2Dlod(Sampler, 4) + tex2Dlod(Sampler, true)
		+ tex2Dlod(Sampler, 0.5) + tex2Dlod(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2Dlod(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleLevel(`
compile_glsl `-S frag`
in_shader `textureLod(`
compile_glsl_es100 `-S frag`
in_shader `texture2DLodEXT(`

// `tex2Dproj`
source `
sampler2D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex2Dproj(Sampler, 4) + tex2Dproj(Sampler, true)
		+ tex2Dproj(Sampler, 0.5) + tex2Dproj(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2Dproj(`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample(`
compile_glsl `-S frag`
in_shader `textureProj(`
compile_glsl_es100 `-S frag`
in_shader `texture2DProj(`

// `tex3D`
source `
sampler3D Sampler;
float4 main(float3 p : TEXCOORD) : COLOR
{
	return tex3D(Sampler, 4) + tex3D(Sampler, true)
		+ tex3D(Sampler, 0.5) + tex3D(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex3D(`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample(`
compile_glsl `-S frag`
in_shader `texture(`
compile_fail_glsl_es100 `pixel`

// `tex3Dbias`
source `
sampler3D Sampler;
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex3Dbias(Sampler, 4) + tex3Dbias(Sampler, true)
		+ tex3Dbias(Sampler, 0.5) + tex3Dbias(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex3Dbias(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleBias(`
compile_glsl `-S frag`
in_shader `texture(`
compile_fail_glsl_es100 `pixel`

// `tex3Dgrad`
source `
sampler3D Sampler;
float4 main(float3 p : TEXCOORD) : COLOR
{
	return tex3Dgrad(Sampler, 4, 5, 6) + tex3Dgrad(Sampler, true, false, true)
		+ tex3Dgrad(Sampler, 0.5, 0.6, 0.7) + tex3Dgrad(Sampler, p, p, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex3Dgrad(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleGrad(`
compile_glsl `-S frag`
in_shader `textureGrad(`
compile_fail_glsl_es100 `pixel`

// `tex3Dlod (VS)`
source `
sampler3D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : POSITION
{
	return tex3Dlod(Sampler, 4) + tex3Dlod(Sampler, true)
		+ tex3Dlod(Sampler, 0.5) + tex3Dlod(Sampler, p);
}`
compile_hlsl_before_after ``
in_shader `tex3Dlod(`
compile_hlsl4 ``
in_shader `SampleLevel(`
compile_glsl ``
in_shader `textureLod(`
compile_fail_glsl_es100 `pixel`

// `tex3Dlod (PS)`
source `
sampler3D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex3Dlod(Sampler, 4) + tex3Dlod(Sampler, true)
		+ tex3Dlod(Sampler, 0.5) + tex3Dlod(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex3Dlod(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleLevel(`
compile_glsl `-S frag`
in_shader `textureLod(`
compile_fail_glsl_es100 `pixel`

// `tex3Dproj`
source `
sampler3D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex3Dproj(Sampler, 4) + tex3Dproj(Sampler, true)
		+ tex3Dproj(Sampler, 0.5) + tex3Dproj(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex3Dproj(`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample(`
compile_glsl `-S frag`
in_shader `textureProj(`
compile_fail_glsl_es100 `pixel`

// `texCUBE`
source `
samplerCUBE Sampler;
float4 main(float3 p : TEXCOORD) : COLOR
{
	return texCUBE(Sampler, 4) + texCUBE(Sampler, true)
		+ texCUBE(Sampler, 0.5) + texCUBE(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample(`
compile_glsl `-S frag`
in_shader `texture(`
compile_glsl_es100 `-S frag`
in_shader `textureCube(`

// `texCUBEbias`
source `
samplerCUBE Sampler;
float4 main(float4 p : TEXCOORD) : COLOR
{
	return texCUBEbias(Sampler, 4) + texCUBEbias(Sampler, true)
		+ texCUBEbias(Sampler, 0.5) + texCUBEbias(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `texCUBEbias(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleBias(`
compile_glsl `-S frag`
in_shader `texture(`
compile_glsl_es100 `-S frag`
in_shader `textureCube(`

// `texCUBEgrad`
source `
samplerCUBE Sampler;
float4 main(float3 p : TEXCOORD) : COLOR
{
	return texCUBEgrad(Sampler, 4, 5, 6) + texCUBEgrad(Sampler, true, false, true)
		+ texCUBEgrad(Sampler, 0.5, 0.6, 0.7) + texCUBEgrad(Sampler, p, p, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `texCUBEgrad(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleGrad(`
compile_glsl `-S frag`
in_shader `textureGrad(`
compile_glsl_es100 `-S frag`
in_shader `textureCubeGradEXT(`

// `texCUBElod (VS)`
source `
samplerCUBE Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : POSITION
{
	return texCUBElod(Sampler, 4) + texCUBElod(Sampler, true)
		+ texCUBElod(Sampler, 0.5) + texCUBElod(Sampler, p);
}`
compile_hlsl_before_after ``
in_shader `texCUBElod(`
compile_hlsl4 ``
in_shader `SampleLevel(`
compile_glsl ``
in_shader `textureLod(`
compile_glsl_es100 ``
in_shader `textureCubeLod(`

// `texCUBElod (PS)`
source `
samplerCUBE Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return texCUBElod(Sampler, 4) + texCUBElod(Sampler, true)
		+ texCUBElod(Sampler, 0.5) + texCUBElod(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `texCUBElod(`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleLevel(`
compile_glsl `-S frag`
in_shader `textureLod(`
compile_glsl_es100 `-S frag`
in_shader `textureCubeLodEXT(`

// `texCUBEproj`
source `
samplerCUBE Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return texCUBEproj(Sampler, 4) + texCUBEproj(Sampler, true)
		+ texCUBEproj(Sampler, 0.5) + texCUBEproj(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `texCUBEproj(`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample(`
compile_glsl `-S frag`
in_shader `texture(`
compile_glsl_es100 `-S frag`
in_shader `textureCube(`


// `tex1Dcmp`
source `
sampler1Dcmp Sampler;
float4 main(float p : TEXCOORD) : COLOR
{
	return tex1Dcmp(Sampler, 4, p) + tex1Dcmp(Sampler, true, 1) + tex1Dcmp(Sampler, p, false);
}`
compile_fail_hlsl `pixel`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleCmp(`
compile_glsl `-S frag`
in_shader `texture(`
compile_fail_glsl_es100 `pixel`

// `tex1Dlod0cmp`
source `
sampler1Dcmp Sampler;
float4 main(float p : TEXCOORD) : COLOR
{
	return tex1Dlod0cmp(Sampler, 4, p) + tex1Dlod0cmp(Sampler, true, 1) + tex1Dlod0cmp(Sampler, p, false);
}`
compile_fail_hlsl `pixel`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleCmpLevelZero(`
compile_glsl `-S frag`
in_shader `texture(`
in_shader `,-32)`
compile_fail_glsl_es100 `pixel`

// `tex2Dcmp`
source `
sampler2Dcmp Sampler;
float4 main(float2 p : TEXCOORD) : COLOR
{
	return tex2Dcmp(Sampler, 4, p.x) + tex2Dcmp(Sampler, true, 1) + tex2Dcmp(Sampler, p, false);
}`
compile_fail_hlsl `pixel`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleCmp(`
compile_glsl `-S frag`
in_shader `texture(`
compile_fail_glsl_es100 `pixel`

// `tex2Dlod0cmp`
source `
sampler2Dcmp Sampler;
float4 main(float2 p : TEXCOORD) : COLOR
{
	return tex2Dlod0cmp(Sampler, 4, p.x) + tex2Dlod0cmp(Sampler, true, 1) + tex2Dlod0cmp(Sampler, p, false);
}`
compile_fail_hlsl `pixel`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleCmpLevelZero(`
compile_glsl `-S frag`
in_shader `texture(`
in_shader `,-32)`
compile_fail_glsl_es100 `pixel`

// `texCUBEcmp`
source `
samplerCUBEcmp Sampler;
float4 main(float3 p : TEXCOORD) : COLOR
{
	return texCUBEcmp(Sampler, 4, p.x) + texCUBEcmp(Sampler, true, 1) + texCUBEcmp(Sampler, p, false);
}`
compile_fail_hlsl `pixel`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleCmp(`
compile_glsl `-S frag`
in_shader `texture(`
compile_fail_glsl_es100 `pixel`

// `texCUBElod0cmp`
source `
samplerCUBEcmp Sampler;
float4 main(float3 p : TEXCOORD) : COLOR
{
	return texCUBElod0cmp(Sampler, 4, p.x) + texCUBElod0cmp(Sampler, true, 1) + texCUBElod0cmp(Sampler, p, false);
}`
compile_fail_hlsl `pixel`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleCmpLevelZero(`
compile_glsl `-S frag`
in_shader `texture(`
in_shader `,-32)`
compile_fail_glsl_es100 `pixel`
