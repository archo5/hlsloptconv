
// `basic preprocessor define`
source `
#define Q 0.0
float4 main() : POSITION { return Q; }`
compile_hlsl_before_after ``
compile_glsl ``

// `basic preprocessor define-undef-fail`
source `
#define Q 0.0
#undef
float4 main() : POSITION { return Q; }`
compile_fail ``

// `preprocessor repeated expansion`
source `
#define AA A
#define A 1.0
#define B 2.0
#define BB B
float4 main() : POSITION { return AA + BB; }`
compile_hlsl_before_after ``
compile_glsl ``

// `preprocessor ifdef-endif`
source `
#define ON
#ifdef ON
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#ifdef ON
float4 main() : POSITION { return 0.0; }
#endif`
compile_fail ``

source `
#ifdef ON
float4 main() : POSITION { return 0.0; }
#endif
float4 main() : POSITION { return 0.0; }`
compile_hlsl_before_after ``
compile_glsl ``

// `preprocessor ifdef-else-endif`
source `
#ifdef ON
#else
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#define ON
#ifdef ON
#else
float4 main() : POSITION { return 0.0; }
#endif`
compile_fail ``

source `
#ifdef ON
#else
float4 main() : POSITION { return 0.0; }
#endif
float4 main() : POSITION { return 0.0; }`
compile_fail ``

// `preprocessor ifndef-endif`
source `
#ifndef ON
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#define ON
#ifndef ON
float4 main() : POSITION { return 0.0; }
#endif`
compile_fail ``

// `preprocessor if-endif`
source `
#if 1
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#if 0
float4 main() : POSITION { return 0.0; }
#endif`
compile_fail ``

source `
#if 123 + 456 + 789
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#if 123 - 123
float4 main() : POSITION { return 0.0; }
#endif`
compile_fail ``

// `preprocessor if-elif-else-endif`
source `
#if 1
float4 main() : POSITION { return 0.0; }
#elif 1
float4 main() : POSITION { return 0.0; }
#else
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#if 0
float4 main() : POSITION { return 0.0; }
#elif 1
float4 main() : POSITION { return 0.0; }
#else
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#if 0
float4 main() : POSITION { return 0.0; }
#elif 0
float4 main() : POSITION { return 0.0; }
#else
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

// `preprocessor macro if-elif-else-endif`
source `
#define A 1
#define B 1
#if A
float4 main() : POSITION { return 0.0; }
#elif B
float4 main() : POSITION { return 0.0; }
#else
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#define A 0
#define B 1
#if A
float4 main() : POSITION { return 0.0; }
#elif B
float4 main() : POSITION { return 0.0; }
#else
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

source `
#define A 0
#define B 0
#if A
float4 main() : POSITION { return 0.0; }
#elif B
float4 main() : POSITION { return 0.0; }
#else
float4 main() : POSITION { return 0.0; }
#endif`
compile_hlsl_before_after ``
compile_glsl ``

// `preprocessor function-style macro`
source `
#define RET( a ) COMB( return, a )
#define COMB( first, second ) first second
float4 main() : POSITION { RET( 0.0 ); }`
compile_hlsl_before_after ``
compile_glsl ``

// `preprocessor error`
source `
#if 0
#error "but not this"
#endif
#error "wat"
float4 main() : POSITION { RET( 0.0 ); }`
compile_fail ``
check_err `<memory>:5:1: error: wat
`

// `preprocessor line + error`
source `
#line 101
#error "one"`
compile_fail ``
check_err `<memory>:101:1: error: one
`

// `preprocessor line + source + syntax error`
source `
#line 101 "other"
+`
compile_fail ``
check_err `other:101:1: error: unexpected token: +
`

// `preprocessor include`
rminc ``
addinc `fna=main()`
source `
float4
#include "fna"
: POSITION { return 0.0; }
`
compile_hlsl ``
compile_glsl ``

// `preprocessor include + syntax error`
rminc ``
addinc `real=
+`
source `
#include "real"`
compile_fail ``
check_err `real:2:1: error: unexpected token: +
`
