
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
