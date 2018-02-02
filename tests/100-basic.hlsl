
// `basic vertex shader 1`
source `
float4 main() : POSITION { return 0.0; }
`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `comments`
source `
// NOT THIS
/* ALSO NOT
THIS */
float4 main() : POSITION { return 0.0; }
`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `all the math operators`
source `
float4 main() : POSITION { return 1.0 + 2.0 * 3.0 / 4.0 - 0.5 % 1.0; }
`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `all the math operators (noopt)`
source `
float4 vals;
float4 main() : POSITION { return vals.x + vals.y * vals.z / vals.w - vals.y % vals.x; }
`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `all the comparison operators (noopt)`
source `
float4 vals;
float4 main() : POSITION { return (vals.x == vals.y) + (vals.y != vals.z) +
	(vals.z < vals.w) + (vals.w <= vals.x) + (vals.x > vals.z) + (vals.y >= vals.w); }
`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `vector comparison operators (noopt)`
source `
float4 vals;
float4 vals2;
float4 main() : POSITION { return (vals == vals2) + (vals != vals2) +
	(vals < vals2) + (vals <= vals2) + (vals > vals2) + (vals >= vals2)
	+
	(vals == 5) + (vals != 5) +
	(vals < 5) + (vals <= 5) + (vals > 5) + (vals >= 5)
	+
	(true == vals2) + (true != vals2) +
	(true < vals2) + (true <= vals2) + (true > vals2) + (true >= vals2); }
`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `logical operators`
source `
float4 vals;
float4 main() : POSITION { return vals.x && vals.y || vals.z; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `unary operators`
source `
float4 main() : POSITION { return (+0) + (-0.1) + (~~0) + (!0) + (!!false); }
`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `parentheses in expression`
source `
float4 main() : POSITION { return ( 1.0 + 2.0 ) * 3.0 / ( 4.0 - 2.5 ) % 1.0; }
`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `basic ternary op`
source `float4 main() : POSITION { return 1 ? 2 : 3; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `complex ternary op`
source `float4 main() : POSITION { return 0 ? 1 ? 2 : 3 : 4 ? 5 : 6; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `multi-type ternary op`
source `float4 main(float4 p : POSITION) : POSITION { return (1 ? 2 : p) + (0 ? 1 : p); }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `extra semicolon parsing`
source `float4 main() : POSITION { ;; return 0; ;;;; }`
compile_hlsl_before_after ``

// `nothing returned 1`
source `float4 main() : POSITION {}`
compile_fail ``

// `nothing returned 2`
source `void main( out float4 pos : POSITION ){}`
compile_fail ``

// `not enough returned 1`
source `void main( out float4 pos : POSITION ){ pos.xyw = 1.0; }`
compile_fail ``

// `uninitialized variable returned`
source `float4 main() : POSITION { float4 o; return o; }`
compile_fail ``

// `bool constant`
source `float4 main() : POSITION { return true; }`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``
