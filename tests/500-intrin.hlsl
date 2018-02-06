
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
// `TODO_FIXME compile_glsl`
// `TODO_FIXME compile_glsl_es100`

// `any`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return any(true) + any(1) + any(0.5) + any(p) + any(MTX);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
// `TODO_FIXME compile_glsl`
// `TODO_FIXME compile_glsl_es100`

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

// `clamp`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return clamp(4, 5, 2) + clamp(true, false, true) + clamp(0.5, 0.6, 0.7)
		+ clamp(p, p, p) + clamp(MTX, MTX, MTX)._11_12_13_22;
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

// `cos`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return cos(true) + cos(0) + cos(0.5) + cos(p) + cos(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `cosh`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return cosh(true) + cosh(0) + cosh(0.5) + cosh(p) + cosh(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
// `TODO_FIXME compile_glsl_es100`

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

// `degrees`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return degrees(true) + degrees(0) + degrees(0.5) + degrees(p) + degrees(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `determinant`
source `
float2x2 MTX2;
float3x3 MTX3;
float4x4 MTX4;
float4 main(float4 p : POSITION) : POSITION
{
	return determinant(MTX2) + determinant(MTX3) + determinant(MTX4);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
// `TODO_FIXME compile_glsl`
// `TODO_FIXME compile_glsl_es100`

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

// `TODO faceforward`

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

// `fmod`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	float q = fmod(0, true);
	float a = fmod(true, 1);
	float b = fmod(0.5, -0.5);
	float4 c = fmod(p, 0.5);
	return q + a + b + c
		+ fmod(MTX, 1.5)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
in_shader `trunc(`
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

// `TODO isfinite`
// `TODO isinf`
// `TODO isnan`

// `ldexp`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	float q = ldexp(0, true);
	float a = ldexp(true, 1);
	float b = ldexp(0.5, -0.5);
	float4 c = ldexp(p, 0.5);
	return q + a + b + c
		+ ldexp(MTX, 1.5)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `length`
source `
float4 main(float3 p : POSITION) : POSITION
{
	return length(4) + length(true) + length(0.5) + length(p);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `lerp`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return lerp(4, 5, 2) + lerp(true, false, true) + lerp(0.5, 0.6, 0.7)
		+ lerp(p, p, p) + lerp(MTX, MTX, MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
in_shader `lerp`
compile_hlsl4 ``
in_shader `lerp`
compile_glsl ``
in_shader `mix`
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``
in_shader `mix`

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

// `mod (GLSL, emulated on HLSL)`
source `
float2x3 MTX1;
float2x3 MTX2;
float4 main(float4 p : POSITION) : POSITION
{
	return mod(p, float4(1,2,3,4)) * mod(MTX1, MTX2)._11_12_13_22;
}`
compile_hlsl ``
in_shader `floor(`
compile_hlsl4 ``
in_shader `floor(`
compile_glsl ``
in_shader `mod(`
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``
in_shader `mod(`

// `% -> mod (GLSL)`
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
in_shader `mod(`
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``
in_shader `mod(`

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

// `radians`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return radians(p.x) + radians(p.xy).xyxy + radians(p.xyz).xyzx + radians(p) + radians(MTX)._11_12_13_22;
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
	return reflect(p, c) + reflect(p, 5) + reflect(4, c) + reflect(2, 3);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `refract`
source `
float4 main(float4 p : POSITION, float4 c : COLOR) : POSITION
{
	return refract(p, c, 0.314) + refract(p, 5, 0.314) + refract(4, c, 0.314) + refract(2, 3, 0.314);
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
compile_glsl_es100 ``

// `round`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return round(p.x) + round(p.xy).xyxy + round(p.xyz).xyzx + round(p) + round(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
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

// `saturate`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return saturate(p.x) + saturate(p.xy).xyxy + saturate(p.xyz).xyzx + saturate(p) + saturate(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `sign`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return sign(p.x) + sign(p.xy).xyxy + sign(p.xyz).xyzx + sign(p) + sign(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `sin`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return sin(p.x) + sin(p.xy).xyxy + sin(p.xyz).xyzx + sin(p) + sin(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `sinh`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return sinh(p.x) + sinh(p.xy).xyxy + sinh(p.xyz).xyzx + sinh(p) + sinh(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
// `TODO_FIXME compile_glsl_es100`

// `smoothstep`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return smoothstep(4, 5, 2) + smoothstep(true, false, true) + smoothstep(0.5, 0.6, 0.7)
		+ smoothstep(p, p, p) + smoothstep(MTX, MTX, MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

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

// `tan`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return tan(p.x) + tan(p.xy).xyxy + tan(p.xyz).xyzx + tan(p) + tan(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``

// `tanh`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return tanh(p.x) + tanh(p.xy).xyxy + tanh(p.xyz).xyzx + tanh(p) + tanh(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
// `TODO_FIXME compile_glsl_es100`

// `TODO transpose`

// `trunc`
source `
float2x3 MTX;
float4 main(float4 p : POSITION) : POSITION
{
	return trunc(p.x) + trunc(p.xy).xyxy + trunc(p.xyz).xyzx + trunc(p) + trunc(MTX)._11_12_13_22;
}`
compile_hlsl_before_after ``
compile_hlsl4 ``
compile_glsl ``
source_replace `float2x3=>float3x3`
compile_glsl_es100 ``
