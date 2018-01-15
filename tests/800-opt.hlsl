
// `optimize out unused uniforms`
source `
float4x4 ModelViewMatrix;
float4 main() : POSITION { return 0.0; }
`
not_in_shader `ModelViewMatrix`
compile_hlsl_before_after ``
compile_glsl ``

// `basic constant propagation`
source `
float4 main() : POSITION { return 12345.0 + 23456.0; }
`
compile_hlsl_before_after ``
not_in_shader `12345`
not_in_shader `23456`
compile_glsl ``

// `unused function removal`
source `
float4 adder( float a, float b ){ return a + b; }
float4 main() : POSITION { return 12345.0 + 23456.0; }
`
compile_hlsl_before_after ``
not_in_shader `adder`
compile_glsl ``

// `function inlining`
source `
float4 adder( float a, float b ){ return a + b; }
float4 main() : POSITION { return adder( 12345.0, 23456.0 ); }
`
//compile_hlsl_before_after ``
//not_in_shader `__ret F5adder`
//not_in_shader `__ret adder`
//not_in_shader `float4 adder`
//compile_glsl ``

// `if/else - constant condition 1`
source `
float4 main() : POSITION { if( true + true ) return 12345; else return 23456; }
`
compile_hlsl_before_after ``
not_in_shader `23456`
compile_glsl ``

// `if/else - constant condition 2`
source `
float4 main() : POSITION { if( true - true ) return 12345; else return 23456; }
`
compile_hlsl_before_after ``
not_in_shader `12345`
compile_glsl ``

// `if/else - constant condition 3`
source `
float4 main() : POSITION { if( true - true ) return 12345; return 23456; }
`
compile_hlsl_before_after ``
not_in_shader `12345`
compile_glsl ``
