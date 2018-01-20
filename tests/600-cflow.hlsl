
// `basic 'if'`
source `
float4 main( float2 tc : TEXCOORD ) : POSITION
{
	if( tc.x )
		return 0.0;
	if( tc.y )
	{
		return 1.0;
	}
	else
		return 2.0;
}`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `basic 'while'`
source `
float4 main( float2 tc : TEXCOORD ) : POSITION
{
	float2 tcc = tc;
	while( tcc.x )
		tcc = tcc + 1;
	return tcc.xyxy;
}`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `basic 'do-while'`
source `
float4 main( float2 tc : TEXCOORD ) : POSITION
{
	float2 tcc = tc;
	do
		tcc = tcc + 1;
	while( tcc.x );
	do
	{
		tcc = tcc + 10;
	}
	while( tcc.y );
	return tcc.xyxy;
}`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `basic 'for'`
source `
float4 main( float2 tc : TEXCOORD ) : POSITION
{
	float4 outval = 0;
	for( int i = 0; i < 10; i = i + 1 )
	{
		outval += tc.xyxy * i;
	}
	return outval;
}`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `basic 'break' and 'continue'`
source `
float4 main( float2 tc : TEXCOORD ) : POSITION
{
	float2 tcc = tc;
	while( true )
	{
		if( !tcc.x )
			break;
		if( !(tcc.x-tcc.y) )
			continue;
		tcc = tcc + 1;
	}
	return tcc.xyxy;
}`
compile_hlsl_before_after ``
compile_glsl ``
compile_glsl_es100 ``

// `basic 'discard'`
source `
float4 main( float2 tc : TEXCOORD ) : COLOR
{
	discard;
	return 1;
}`
compile_hlsl_before_after `/T ps_3_0`
compile_glsl `-S frag`
compile_glsl_es100 `-S frag`

// `bad 'discard' (VS)`
source `
float4 main( float2 tc : TEXCOORD ) : POSITION
{
	discard;
	return 1;
}`
compile_fail ``
