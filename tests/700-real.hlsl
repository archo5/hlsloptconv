
// `basic vertex shader`
source `
struct a2v { float4 Position : POSITION; };
struct v2p { float4 Position : POSITION; };
float4x4 ModelViewMatrix;
void main( in a2v IN, out v2p OUT )
{
	OUT.Position = mul( IN.Position, ModelViewMatrix );
}`
compile_hlsl_before_after ``
compile_glsl ``
