
// `fxc can treat intermediate intrinsic results as double while optimizing the code..`
// `..or reorder commutative ops, which is why some values are explicitly assigned to variables`

// `abs`
source `
float4x4 ModelViewMatrix;
float4 main( float4 p : POSITION ) : POSITION
{
	return abs( true ) + abs( 0 ) + abs( 0.0 ) + abs( p ) + abs( ModelViewMatrix )._11_12_13_14;
}`
compile_hlsl_before_after ``

// `acos`
source `
float4x4 ModelViewMatrix;
float4 main( float4 p : POSITION ) : POSITION
{
	return acos( 0 ) + acos( true ) + acos( 0.5 ) + acos( p ) + acos( ModelViewMatrix )._11_12_13_14;
}`
compile_hlsl_before_after ``

// `all`
source `
float4x4 ModelViewMatrix;
float4 main( float4 p : POSITION ) : POSITION
{
	return all( true ) + all( 1 ) + all( 0.5 ) + all( p ) + all( ModelViewMatrix );
}`
compile_hlsl_before_after ``

// `any`
source `
float4x4 ModelViewMatrix;
float4 main( float4 p : POSITION ) : POSITION
{
	return any( true ) + any( 1 ) + any( 0.5 ) + any( p ) + any( ModelViewMatrix );
}`
compile_hlsl_before_after ``

// `asin`
source `
float4x4 ModelViewMatrix;
float4 main( float4 p : POSITION ) : POSITION
{
	return asin( true ) + asin( 0 ) + asin( 0.5 ) + asin( p ) + asin( ModelViewMatrix )._11_12_13_14;
}`
compile_hlsl_before_after ``

// `atan`
source `
float4x4 ModelViewMatrix;
float4 main( float4 p : POSITION ) : POSITION
{
	float b = atan( 0.5 );
	return atan( 0 ) + atan( true ) + b + atan( p ) + atan( ModelViewMatrix )._11_12_13_14;
}`
compile_hlsl_before_after ``

// `atan2`
source `
float4x4 ModelViewMatrix;
float4 main( float4 p : POSITION ) : POSITION
{
	float q = atan2( 0, true );
	float a = atan2( true, 1 );
	float b = atan2( 0.5, -0.5 );
	float c = atan2( p, 0.5 );
	return q + a + b + c
		+ atan2( ModelViewMatrix, 1.5 )._11_12_13_14;
}`
compile_hlsl_before_after ``

// `clip`
source `
float4x4 ModelViewMatrix;
float4 main( float4 p : COLOR ) : COLOR
{
	clip( 1 );
	clip( true );
	clip( 0.5 );
	clip( p );
	return p;
}`
compile_hlsl_before_after `/T ps_3_0`

// `cross`
source `
float4 main( float3 p : POSITION ) : POSITION
{
	return ( cross( 4, 5 ) + cross( true, false ) + cross( 0.5, 0.6 ) + cross( p, p ) ).xyzz;
}`
compile_hlsl_before_after ``

// `distance`
source `
float4 main( float3 p : POSITION ) : POSITION
{
	return distance( 4, 5 ) + distance( true, false ) + distance( 0.5, 0.6 ) + distance( p, p );
}`
compile_hlsl_before_after ``

// `dot`
source `
float4 main( float3 p : POSITION ) : POSITION
{
	return dot( 4, 5 ) + dot( true, false ) + dot( 0.5, 0.6 ) + dot( p, p );
}`
compile_hlsl_before_after ``

// `normalize`
source `
float4 main( float4 p : POSITION ) : POSITION
{
	return normalize( 4 ) + normalize( true ) + normalize( 0.5 ) + normalize( p );
}`
compile_hlsl_before_after ``

// `tex2Dgrad`
source `
sampler2D Sampler;
float4 main( float2 p : TEXCOORD ) : COLOR
{
	return tex2Dgrad( Sampler, 4, 5, 6 ) + tex2Dgrad( Sampler, true, false, true )
		+ tex2Dgrad( Sampler, 0.5, 0.6, 0.7 ) + tex2Dgrad( Sampler, p, p, p );
}`
compile_hlsl_before_after `/T ps_3_0`

// `tex2Dlod`
source `
sampler2D Sampler : register(s2);
float4 main( float4 p : POSITION ) : POSITION
{
	return tex2Dlod( Sampler, 4 ) + tex2Dlod( Sampler, true )
		+ tex2Dlod( Sampler, 0.5 ) + tex2Dlod( Sampler, p );
}`
compile_hlsl_before_after ``
