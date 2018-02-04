
// `fxc can treat intermediate intrinsic results as double while optimizing the code..`
// `..or reorder commutative ops, which is why some values are explicitly assigned to variables`

// `abs`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return abs(true) + abs(0) + abs(0.0) + abs(p) + abs(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `acos`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return acos(0) + acos(true) + acos(0.5) + acos(p) + acos(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `all`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return all(true) + all(1) + all(0.5) + all(p) + all(MTX);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``

// `any`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return any(true) + any(1) + any(0.5) + any(p) + any(MTX);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``

// `asin`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return asin(true) + asin(0) + asin(0.5) + asin(p) + asin(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `atan`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	float b = atan(0.5);
	return atan(0) + atan(true) + b + atan(p) + atan(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `atan2`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	float q = atan2(0, true);
	float a = atan2(true, 1);
	float b = atan2(0.5, -0.5);
	float4 c = atan2(p, 0.5);
	return q + a + b + c
		+ atan2(MTX, 1.5)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
in_shader `atan(`
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``
in_shader `atan(`

// `ceil`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return ceil(p.x) + ceil(p.xy).xyxy + ceil(p.xyz).xyzx + ceil(p) + ceil(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `clip`
source `
float2x3 MTX;
float4 main(float4 p : COLOR) : COLOR
{
	clip(1);
	clip(true);
	clip(0.5);
	clip(p);
	return p;
}`
compile_hlsl_before_after `/T ps_3_0`
compile_hlsl4 `/T ps_4_0`
compile_glsl `-S frag`
in_shader `discard`
compile_glsl_es100 `-S frag`
in_shader `discard`

// `cross`
source `
float4 main(float3 p : POSITION) : POSITION
{
	return (cross(4, 5) + cross(true, false) + cross(0.5, 0.6) + cross(p, p)).xyzz;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `ddx`
source `
float4 main(float4 p : TEXCOORD) : COLOR
{
	return ddx(p.x) + ddx(p.xy).xyxy + ddx(p.xyz).xyzx + ddx(p);
}`
compile_hlsl_before_after `/T ps_3_0`
compile_hlsl4 `/T ps_4_0`
compile_glsl `-S frag`
in_shader `dFdx`
compile_glsl_es100 `-S frag`
in_shader `dFdx`

// `ddy`
source `
float4 main(float4 p : TEXCOORD) : COLOR
{
	return ddy(p.x) + ddy(p.xy).xyxy + ddy(p.xyz).xyzx + ddy(p);
}`
compile_hlsl_before_after `/T ps_3_0`
compile_hlsl4 `/T ps_4_0`
compile_glsl `-S frag`
in_shader `dFdy`
compile_glsl_es100 `-S frag`
in_shader `dFdy`

// `distance`
source `
float4 main(float3 p : POSITION) : POSITION
{
	return distance(4, 5) + distance(true, false) + distance(0.5, 0.6) + distance(p, p);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `dot`
source `
float4 main(float3 p : POSITION) : POSITION
{
	return dot(4, 5) + dot(true, false) + dot(0.5, 0.6) + dot(p, p);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `exp`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return exp(p.x) + exp(p.xy).xyxy + exp(p.xyz).xyzx + exp(p) + exp(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `exp2`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return exp2(p.x) + exp2(p.xy).xyxy + exp2(p.xyz).xyzx + exp2(p) + exp2(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `floor`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return floor(p.x) + floor(p.xy).xyxy + floor(p.xyz).xyzx + floor(p) + floor(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `frac`
source `
float4 main(float4 p : POSITION) : POSITION
{
	return frac(p.x) + frac(p.xy).xyxy + frac(p.xyz).xyzx + frac(p);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
in_shader `fract`
compile_glsl_es100 ``
in_shader `fract`

// `fwidth`
source `
float4 main(float4 p : TEXCOORD) : COLOR
{
	return fwidth(p.x) + fwidth(p.xy).xyxy + fwidth(p.xyz).xyzx + fwidth(p);
}`
compile_hlsl_before_after `/T ps_3_0`
compile_hlsl4 `/T ps_4_0`
compile_glsl `-S frag`
compile_glsl_es100 `-S frag`

// `log`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return log(p.x) + log(p.xy).xyxy + log(p.xyz).xyzx + log(p) + log(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `log10`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return log10(p.x) + log10(p.xy).xyxy + log10(p.xyz).xyzx + log10(p) + log10(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `log2`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return log2(p.x) + log2(p.xy).xyxy + log2(p.xyz).xyzx + log2(p) + log2(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `matrixCompMult (GLSL)`
source `
float2x3 MTX1;
float2x3 MTX2;
float4 main(float4 p : POSITION) : POSITION
{
	return p * (MTX1 * MTX2)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
in_shader `matrixCompMult`
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``
in_shader `matrixCompMult`

// `max`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	float q = max(0, true);
	float a = max(true, 1);
	float b = max(0.5, -0.5);
	float4 c = max(p, 0.5);
	return q + a + b + c
		+ max(MTX, 1.5)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `min`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	float q = min(0, true);
	float a = min(true, 1);
	float b = min(0.5, -0.5);
	float4 c = min(p, 0.5);
	return q + a + b + c
		+ min(MTX, 1.5)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `mul (overload 1)`
source `
float4 main(float p : POSITION) : POSITION
{
	return mul(1, p) + mul(2, true) + mul(2, p) + mul(p, p) + mul(true, true) + mul(1, 2);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `mul (overload 2)`
source `
float4 main(float4 p : POSITION) : POSITION
{
	return mul(1, p) + mul(true, p) + mul(0.5, p.xy).xyxy;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `mul (overload 3)`
source `
float2x3 MTX;
float4 main() : POSITION
{
	return (mul(1, MTX) + mul(true, MTX) + mul(0.5, MTX))._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `mul (overload 4)`
source `
float4 main(float4 p : POSITION) : POSITION
{
	return mul(p, 1) + mul(p, true) + mul(p.xy, 0.5).xyxy;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `mul (overload 5)`
source `
float4 main(float4 p : POSITION) : POSITION
{
	return mul(p, p) + mul(p.xyz, p.yzw) + mul(p.xy, p.zw);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
in_shader `dot`
compile_glsl_es100 ``
in_shader `dot`

// `mul (overload 6)`
source `
float4x4 MTX4;
float3x3 MTX3;
float2x2 MTX2;
float4 main(float4 p : POSITION) : POSITION
{
	return mul(p, MTX4) + mul(p.xyz, MTX3).xyzx + mul(p.xy, MTX2).xyyx;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `mul (overload 7)`
source `
float2x3 MTX;
float4 main() : POSITION
{
	return (mul(MTX, 1) + mul(MTX, true) + mul(MTX, 0.5))._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `mul (overload 8)`
source `
float4x4 MTX4;
float3x3 MTX3;
float2x2 MTX2;
float4 main(float4 p : POSITION) : POSITION
{
	return mul(MTX4, p) + mul(MTX3, p.xyz).xyzx + mul(MTX2, p.xy).xyyx;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `mul (overload 9)`
source `
float4x4 MTX4;
float3x3 MTX3;
float2x2 MTX2;
float4 main(float4 p : POSITION) : POSITION
{
	return mul(MTX4, MTX4)._11_44_23_32 + mul(MTX3, MTX3)._11_22_23_32 + mul(MTX2, MTX2)._11_12_21_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `mod (GLSL)`
source `
float2x3 MTX1;
float2x3 MTX2;
float4 main(float4 p : POSITION) : POSITION
{
	return (p % float4(1,2,3,4)) * (MTX1 % MTX2)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
in_shader `mod`
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``
in_shader `mod`

// `normalize`
source `
float4 main(float4 p : POSITION) : POSITION
{
	return normalize(4) + normalize(true) + normalize(0.5) + normalize(p);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `pow`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	float q = pow(0, true);
	float a = pow(true, 1);
	float b = pow(0.5, -0.5);
	float4 c = pow(p, 0.5);
	return q + a + b + c
		+ pow(MTX, 1.5)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `reflect`
source `
float4 main(float4 p : POSITION, float4 c : COLOR) : POSITION
{
	return reflect(p, c) + reflect(p, 5) + reflect(4, c)/* TODO + reflect(2, 3)*/;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `refract`
source `
float4 main(float4 p : POSITION, float4 c : COLOR) : POSITION
{
	return refract(p, c, 0.314) + refract(p, 5, 0.314) + refract(4, c, 0.314)/* TODO + refract(2, 3, 0.314)*/;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `rsqrt`
source `
float4 main(float4 p : POSITION) : POSITION
{
	return rsqrt(4) + rsqrt(true) + rsqrt(0.5) + rsqrt(p);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
in_shader `inversesqrt`
compile_glsl_es100 ``
in_shader `inversesqrt`

// `sqrt`
source `
float4 main(float4 p : POSITION) : POSITION
{
	return sqrt(4) + sqrt(true) + sqrt(0.5) + sqrt(p);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `step`
source `
float4 main(float4 p : POSITION) : POSITION
{
	return step(0, p.x) + step(true, p.xy).xyxy + step(p.xyz, true).xyzx + step(p, p.x);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `tex1D`
source `
sampler1D Sampler;
float4 main(float p : TEXCOORD) : COLOR
{
	return tex1D(Sampler, 4) + tex1D(Sampler, true) + tex1D(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1D`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample`
compile_glsl `-S frag`
in_shader `texture`
compile_glsl_es100 `-S frag`
in_shader `texture2D`

// `tex1Dbias`
source `
sampler1D Sampler;
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex1Dbias(Sampler, 4) + tex1Dbias(Sampler, true)
		+ tex1Dbias(Sampler, 0.5) + tex1Dbias(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1Dbias`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleBias`
compile_glsl `-S frag`
in_shader `texture`
compile_glsl_es100 `-S frag`
in_shader `texture2D`

// `tex1Dgrad`
source `
sampler1D Sampler;
float4 main(float p : TEXCOORD) : COLOR
{
	return tex1Dgrad(Sampler, 4, 5, 6) + tex1Dgrad(Sampler, true, false, true)
		+ tex1Dgrad(Sampler, 0.5, 0.6, 0.7) + tex1Dgrad(Sampler, p, p, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1Dgrad`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleGrad`
compile_glsl `-S frag`
in_shader `textureGrad`
compile_glsl_es100 `-S frag`
in_shader `texture2DGradEXT`

// `tex1Dlod (VS)`
source `
sampler1D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : POSITION
{
	return tex1Dlod(Sampler, 4) + tex1Dlod(Sampler, true)
		+ tex1Dlod(Sampler, 0.5) + tex1Dlod(Sampler, p);
}`
compile_hlsl_before_after ``
in_shader `tex1Dlod`
compile_hlsl4 ``
in_shader `SampleLevel`
compile_glsl ``
in_shader `textureLod`
compile_glsl_es100 ``
in_shader `texture2DLod`

// `tex1Dlod (PS)`
source `
sampler1D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex1Dlod(Sampler, 4) + tex1Dlod(Sampler, true)
		+ tex1Dlod(Sampler, 0.5) + tex1Dlod(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1Dlod`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleLevel`
compile_glsl `-S frag`
in_shader `textureLod`
compile_glsl_es100 `-S frag`
in_shader `texture2DLodEXT`

// `tex1Dproj`
source `
sampler1D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex1Dproj(Sampler, 4) + tex1Dproj(Sampler, true)
		+ tex1Dproj(Sampler, 0.5) + tex1Dproj(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex1Dproj`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample`
compile_glsl `-S frag`
in_shader `textureProj`
compile_glsl_es100 `-S frag`
in_shader `texture2DProj`

// `tex2D`
source `
sampler2D Sampler;
float4 main(float2 p : TEXCOORD) : COLOR
{
	return tex2D(Sampler, 4) + tex2D(Sampler, true)
		+ tex2D(Sampler, 0.5) + tex2D(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2D`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample`
compile_glsl `-S frag`
in_shader `texture`
compile_glsl_es100 `-S frag`
in_shader `texture2D`

// `tex2Dbias`
source `
sampler2D Sampler;
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex2Dbias(Sampler, 4) + tex2Dbias(Sampler, true)
		+ tex2Dbias(Sampler, 0.5) + tex2Dbias(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2Dbias`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleBias`
compile_glsl `-S frag`
in_shader `texture`
compile_glsl_es100 `-S frag`
in_shader `texture2D`

// `tex2Dgrad`
source `
sampler2D Sampler;
float4 main(float2 p : TEXCOORD) : COLOR
{
	return tex2Dgrad(Sampler, 4, 5, 6) + tex2Dgrad(Sampler, true, false, true)
		+ tex2Dgrad(Sampler, 0.5, 0.6, 0.7) + tex2Dgrad(Sampler, p, p, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2Dgrad`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleGrad`
compile_glsl `-S frag`
in_shader `textureGrad`
compile_glsl_es100 `-S frag`
in_shader `texture2DGradEXT`

// `tex2Dlod (VS)`
source `
sampler2D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : POSITION
{
	return tex2Dlod(Sampler, 4) + tex2Dlod(Sampler, true)
		+ tex2Dlod(Sampler, 0.5) + tex2Dlod(Sampler, p);
}`
compile_hlsl_before_after ``
in_shader `tex2Dlod`
compile_hlsl4 ``
in_shader `SampleLevel`
compile_glsl ``
in_shader `textureLod`
compile_glsl_es100 ``
in_shader `texture2DLod`

// `tex2Dlod (PS)`
source `
sampler2D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex2Dlod(Sampler, 4) + tex2Dlod(Sampler, true)
		+ tex2Dlod(Sampler, 0.5) + tex2Dlod(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2Dlod`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleLevel`
compile_glsl `-S frag`
in_shader `textureLod`
compile_glsl_es100 `-S frag`
in_shader `texture2DLodEXT`

// `tex2Dproj`
source `
sampler2D Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return tex2Dproj(Sampler, 4) + tex2Dproj(Sampler, true)
		+ tex2Dproj(Sampler, 0.5) + tex2Dproj(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex2Dproj`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample`
compile_glsl `-S frag`
in_shader `textureProj`
compile_glsl_es100 `-S frag`
in_shader `texture2DProj`

// `tex3D`
source `
sampler3D Sampler;
float4 main(float3 p : TEXCOORD) : COLOR
{
	return tex3D(Sampler, 4) + tex3D(Sampler, true)
		+ tex3D(Sampler, 0.5) + tex3D(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `tex3D`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample`
compile_glsl `-S frag`
in_shader `texture`
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
in_shader `tex3Dbias`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleBias`
compile_glsl `-S frag`
in_shader `texture`
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
in_shader `tex3Dgrad`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleGrad`
compile_glsl `-S frag`
in_shader `textureGrad`
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
in_shader `tex3Dlod`
compile_hlsl4 ``
in_shader `SampleLevel`
compile_glsl ``
in_shader `textureLod`
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
in_shader `tex3Dlod`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleLevel`
compile_glsl `-S frag`
in_shader `textureLod`
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
in_shader `tex3Dproj`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample`
compile_glsl `-S frag`
in_shader `textureProj`
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
in_shader `Sample`
compile_glsl `-S frag`
in_shader `texture`
compile_glsl_es100 `-S frag`
in_shader `textureCube`

// `texCUBEbias`
source `
samplerCUBE Sampler;
float4 main(float4 p : TEXCOORD) : COLOR
{
	return texCUBEbias(Sampler, 4) + texCUBEbias(Sampler, true)
		+ texCUBEbias(Sampler, 0.5) + texCUBEbias(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `texCUBEbias`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleBias`
compile_glsl `-S frag`
in_shader `texture`
compile_glsl_es100 `-S frag`
in_shader `textureCube`

// `texCUBEgrad`
source `
samplerCUBE Sampler;
float4 main(float3 p : TEXCOORD) : COLOR
{
	return texCUBEgrad(Sampler, 4, 5, 6) + texCUBEgrad(Sampler, true, false, true)
		+ texCUBEgrad(Sampler, 0.5, 0.6, 0.7) + texCUBEgrad(Sampler, p, p, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `texCUBEgrad`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleGrad`
compile_glsl `-S frag`
in_shader `textureGrad`
compile_glsl_es100 `-S frag`
in_shader `textureCubeGradEXT`

// `texCUBElod (VS)`
source `
samplerCUBE Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : POSITION
{
	return texCUBElod(Sampler, 4) + texCUBElod(Sampler, true)
		+ texCUBElod(Sampler, 0.5) + texCUBElod(Sampler, p);
}`
compile_hlsl_before_after ``
in_shader `texCUBElod`
compile_hlsl4 ``
in_shader `SampleLevel`
compile_glsl ``
in_shader `textureLod`
compile_glsl_es100 ``
in_shader `textureCubeLod`

// `texCUBElod (PS)`
source `
samplerCUBE Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return texCUBElod(Sampler, 4) + texCUBElod(Sampler, true)
		+ texCUBElod(Sampler, 0.5) + texCUBElod(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `texCUBElod`
compile_hlsl4 `/T ps_4_0`
in_shader `SampleLevel`
compile_glsl `-S frag`
in_shader `textureLod`
compile_glsl_es100 `-S frag`
in_shader `textureCubeLodEXT`

// `texCUBEproj`
source `
samplerCUBE Sampler : register(s2);
float4 main(float4 p : TEXCOORD) : COLOR
{
	return texCUBEproj(Sampler, 4) + texCUBEproj(Sampler, true)
		+ texCUBEproj(Sampler, 0.5) + texCUBEproj(Sampler, p);
}`
compile_hlsl_before_after `/T ps_3_0`
in_shader `texCUBEproj`
compile_hlsl4 `/T ps_4_0`
in_shader `Sample`
compile_glsl `-S frag`
in_shader `texture`
compile_glsl_es100 `-S frag`
in_shader `textureCube`
