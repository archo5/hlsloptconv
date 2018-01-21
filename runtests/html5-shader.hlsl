
float2 iResolution;

#if VS
void main(float4 pos : POSITION0, out float4 opos : POSITION0, out float2 otex : TEXCOORD0)
{
	opos = pos;
	otex = (pos.xy * 0.5 + 0.5) * iResolution;
}
#elif PS

float sphereSDF(float3 s, float3 p, float r)
{
	return length(s - p) - r;
}

float sceneSDF(float3 s)
{
	return sphereSDF(s, 0, 1);
}

float raymarch(float3 eye, float3 dir, float start, float end)
{
	float depth = start;
	for (int i = 0; i < 100; i += 1)
	{
		float dst = sceneSDF(eye + depth * dir);
		if (dst < 0.001)
			return dst;
		depth += dst;
		if (depth >= end)
			return end;
	}
	return end;
}

float3 rayDir(float fov, float2 size, float2 coord)
{
	float2 xy = coord - size / 2;
	float z = size.y / tan(radians(fov) / 2);
	return normalize(float3(xy, -z));
}

void main(out float4 outCol : COLOR0, in float2 coord : TEXCOORD0)
{
	float3 dir = rayDir(45, iResolution, coord);
	float3 eye = {0,0,5};
	float dist = raymarch(eye, dir, 0, 100);
	if (dist >= 100-0.001)
	{
		outCol = 0;
		return;
	}
	outCol = float4(coord.x % 2, coord.y % 2,1,1);
}

#endif
