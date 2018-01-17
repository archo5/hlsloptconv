
// `function call`
source `
float4 passthru( float x ){ return x; }
float4 main() : POSITION { return passthru( 0.0 ); }`
compile_hlsl_before_after ``
compile_glsl ``

// `fcall with out`
source `
void retn( out float4 val ){ val = 1.0; }
float4 main() : POSITION
{
	float4 v;
	retn( v );
	return v;
}`
compile_hlsl_before_after ``
compile_glsl ``

// `bad fcall with out 1`
source `
void retn( out float4 val ){ val = 1.0; }
float4 main() : POSITION
{
	retn( 1.0 );
	return 1.0;
}`
compile_fail ``

// `overloaded func`
source `
float a(float a, float b){ return 1.0; }
float a(float a, int b){ return 10.0; }
float4 main():POSITION { return a(0,0) + a(0.0,0) + a(0,0.0) + a(0.0,0.0); }
`
compile_hlsl_before_after ``
compile_glsl ``

// `scalar type promotion`
source `
float a(bool a){ return 1.0; }
float a(int a){ return 2.0; }
float a(half a){ return 3.0; }
float a(float a){ return 4.0; }
float4 main():POSITION {
	bool bv = true;
	int iv = 3;
	half hv = 1.5;
	float fv = 2.5;
	return
	a(bv+bv) +
	a(bv+iv)*5 +
	a(bv+hv)*5*5 +
	a(bv+fv)*5*5*5 +
	a(iv+iv)*5*5*5*5 +
	a(iv+hv)*5*5*5*5*5 +
	a(iv+fv)*5*5*5*5*5*5 +
	a(hv+hv)*5*5*5*5*5*5*5 +
	a(hv+fv)*5*5*5*5*5*5*5*5 +
	a(fv+fv)*5*5*5*5*5*5*5*5*5; }`
compile_hlsl_before_after ``
compile_glsl ``

// `swizzled types`
source `
float a(float a){ return 1.0; }
//float a(float1 a){ return 2.0; }
float a(float3 a){ return 3.0; }
float a(float4 a){ return 4.0; }
float4 main(float4 p : POSITION):POSITION {
	return
	a(1.f) +
	a(1.f.x)*5 +
	a(1.f.xxx)*5*5 +
	a(1.f.xxxx)*5*5*5 +
	a(p)*5*5*5*5 +
	a(p.x)*5*5*5*5*5 +
	a(p.xxx)*5*5*5*5*5*5 +
	a(p.xxxx)*5*5*5*5*5*5*5; }`
compile_hlsl_before_after ``
compile_glsl ``

// `effectively equal types (for overload conflicts)`
source `
float a(float a){ return 1.0; }
float a(float1 a){ return 2.0; }
float4 main(float4 p : POSITION):POSITION { return a(1.f); }`
compile_fail_with_hlsl ``
source `
float a(float a){ return 1.0; }
float a(float1 a){ return 2.0; }
float4 main(float4 p : POSITION):POSITION { return a(float1(1.f)); }`
compile_fail_with_hlsl ``

// `assignment casts`
source `
struct stc4el
{
	float x;
	float2 yz;
	float w;
};
float4 main():POSITION {
	float vs = 1;
	float1 vv1 = 2.0;
	float4 vv4 = 2.5;
	float1x1 vm11 = 3.0;
	float1x4 vm14 = 3.2;
	float4x1 vm41 = 3.4;
	float4x4 vm44 = 3.6;
	stc4el stc = { 4, 5, 6, 7 };
	vs = vs; vv1 = vs; vv4 = vs; vm11 = vs; vm14 = vs; vm41 = vs; vm44 = vs; /*stc = vs;*/
	vs = vv1; vv1 = vv1; vv4 = vv1; vm11 = vv1; vm14 = vv1; vm41 = vv1; vm44 = vv1; /*stc = vv1;*/
	vs = vv4; vv1 = vv4; vv4 = vv4; vm11 = vv4; vm14 = vv4; vm41 = vv4; /*vm44 = vv4; stc = vv4;*/
	vs = vm11; vv1 = vm11; vv4 = vm11; vm11 = vm11; vm14 = vm11; vm41 = vm11; vm44 = vm11; /*stc = vm11;*/
	vs = vm14; vv1 = vm14; vv4 = vm14; vm11 = vm14; vm14 = vm14; /*vm41 = vm14; vm44 = vm14; stc = vm14;*/
	vs = vm41; vv1 = vm41; vv4 = vm41; vm11 = vm41; /*vm14 = vm41;*/ vm41 = vm41; /*vm44 = vm41; stc = vm41;*/
	vs = vm44; vv1 = vm44; /*vv4 = vm44;*/ vm11 = vm44; vm14 = vm44; vm41 = vm44; vm44 = vm44; /*stc = vm44;*/
	/*vs = stc; vv1 = stc; vv4 = stc; vm11 = stc; vm14 = stc; vm41 = stc; vm44 = stc;*/ stc = stc;
	return vv4;
}`
compile_hlsl_before_after ``
compile_glsl ``

// `failed assignment casts`
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	stc4el stc; float vs = 1.0; stc = vs; return 0; }`
compile_fail_with_hlsl ``

source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	stc4el stc; float1 vv1 = 2.0; stc = vv1; return 0; }`
compile_fail_with_hlsl ``

source `float4 main():POSITION { float4x4 vm44; float4 vv4 = 2.5; vm44 = vv4; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	stc4el stc; float4 vv4 = 2.5; stc = vv4; return 0; }`
compile_fail_with_hlsl ``

source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	stc4el stc; float1x1 vm11 = 3.0; stc = vm11; return 0; }`
compile_fail_with_hlsl ``

source `float4 main():POSITION { float4x1 vm41; float1x4 vm14 = 3.2; vm41 = vm14; return 0; }`
compile_fail_with_hlsl ``
source `float4 main():POSITION { float4x4 vm44; float1x4 vm14 = 3.2; vm44 = vm14; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	stc4el stc; float1x4 vm14 = 3.2; stc = vm14; return 0; }`
compile_fail_with_hlsl ``

source `float4 main():POSITION { float1x4 vm14; float4x1 vm41 = 3.4; vm14 = vm41; return 0; }`
compile_fail_with_hlsl ``
source `float4 main():POSITION { float4x4 vm44; float4x1 vm41 = 3.4; vm44 = vm41; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	stc4el stc; float4x1 vm41 = 3.4; stc = vm41; return 0; }`
compile_fail_with_hlsl ``

source `float4 main():POSITION { float4 vv4; float4x4 vm44 = 3.6; vv4 = vm44; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	stc4el stc; float4x4 vm44 = 3.6; stc = vm44; }`
compile_fail_with_hlsl ``

source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	float vs = 1; stc4el stc; vs = stc; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	float1 vv1 = 2.0; stc4el stc; vv1 = stc; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	float4 vv4 = 2.5; stc4el stc; vv4 = stc; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	float1x1 vm11 = 3.0; stc4el stc; vm11 = stc; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	float1x4 vm14 = 3.2; stc4el stc; vm14 = stc; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	float4x1 vm41 = 3.4; stc4el stc; vm41 = stc; return 0; }`
compile_fail_with_hlsl ``
source `struct stc4el { float x; float2 yz; float w; }; float4 main():POSITION {
	float4x4 vm44 = 3.6; stc4el stc; vm44 = stc; return 0; }`
compile_fail_with_hlsl ``

// `vector type casts in calls`
source `
float s(float a){ return 1; }
float v1(float1 a){ return 1; }
float v4(float4 a){ return 1; }
float m11(float1x1 a){ return 1; }
float m14(float1x4 a){ return 1; }
float m41(float4x1 a){ return 1; }
float m44(float4x4 a){ return 1; }
float4 main():POSITION {
	float vs = 1;
	float1 vv1 = 2.0;
	float4 vv4 = 2.5;
	float1x1 vm11 = 3.0;
	float1x4 vm14 = 3.2;
	float4x1 vm41 = 3.4;
	float4x4 vm44 = 3.6;
	return
	s(vs) + s(vv1) + s(vv4) + s(vm11) + s(vm14) + s(vm41) + s(vm44)
	+ v1(vs) + v1(vv1) + v1(vv4) + v1(vm11) + v1(vm14) + v1(vm41) + v1(vm44)
	+ v4(vs) + v4(vv1) + v4(vv4) + v4(vm11) + v4(vm14) + v4(vm41) /*+ v4(vm44)*/
	+ m11(vs) + m11(vv1) + m11(vv4) + m11(vm11) + m11(vm14) + m11(vm41) + m11(vm44) 
	+ m14(vs) + m14(vv1) + m14(vv4) + m14(vm11) + m14(vm14) /*+ m14(vm41)*/ + m14(vm44)
	+ m41(vs) + m41(vv1) + m41(vv4) + m41(vm11) /*+ m41(vm14)*/ + m41(vm41) /*+ m41(vm44)*/
	+ m44(vs) + m44(vv1) /*+ m44(vv4)*/ + m44(vm11) /*+ m44(vm14)*/ /*+ m44(vm41)*/ + m44(vm44)
	; }`
compile_hlsl_before_after ``
compile_glsl ``

// `failed vector type casts in calls`
source `float v4(float4 a){ return 1; }
float4 main():POSITION { float4x4 vm44 = 3.6; return v4(vm44); }`
compile_fail_with_hlsl ``

source `float m14(float1x4 a){ return 1; }
float4 main():POSITION { float4x1 vm41 = 3.4; return m14(vm41); }`
compile_fail_with_hlsl ``

source `float m41(float4x1 a){ return 1; }
float4 main():POSITION { float1x4 vm14 = 3.2; return m14(vm14); }`
compile_fail_with_hlsl ``
source `float m41(float4x1 a){ return 1; }
float4 main():POSITION { float4x4 vm44 = 3.6; return m14(vm44); }`
compile_fail_with_hlsl ``

source `float m44(float4x4 a){ return 1; }
float4 main():POSITION { float4 vv4 = 2.5; return m14(vv4); }`
compile_fail_with_hlsl ``
source `float m44(float4x4 a){ return 1; }
float4 main():POSITION { float1x4 vm14 = 3.2; return m14(vm14); }`
compile_fail_with_hlsl ``
source `float m44(float4x4 a){ return 1; }
float4 main():POSITION { float4x1 vm41 = 3.4; return m14(vm41); }`
compile_fail_with_hlsl ``
