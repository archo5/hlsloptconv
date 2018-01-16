
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
	a(fv+fv)*5*5*5*5*5*5*5*5*5; }
`
compile_hlsl_before_after ``
compile_glsl ``

// `vector type casts`
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
	+ m14(vs) + m14(vv1) + m14(vv4) + m14(vm11) + m14(vm14) /*+ m14(vm41)*/ /*+ m14(vm44)*/
	+ m41(vs) + m41(vv1) + m41(vv4) + m41(vm11) /*+ m41(vm14)*/ + m41(vm41) /*+ m41(vm44)*/
	+ m44(vs) + m44(vv1) /*+ m44(vv4)*/ + m44(vm11) /*+ m44(vm14)*/ /*+ m44(vm41)*/ + m44(vm44)
	; }`
compile_hlsl_before_after ``
compile_glsl ``
