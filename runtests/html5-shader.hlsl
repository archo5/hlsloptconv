
cbuffer uniformData
{
	float2 iResolution;
	float4x4 viewMatrix;
}

#if VS
#if D3D11
static const float4 verts[3] =
{
	-1, -1, 0.5f, 1,
	3, -1, 0.5f, 1,
	-1, 3, 0.5f, 1,
};
void main(uint vtxid : SV_VertexID, out float2 otex : TEXCOORD0, out float4 opos : POSITION0)
{
	opos = verts[vtxid];
	otex = (opos.xy * 0.5 + 0.5) * iResolution;
}
#else
void main(float4 pos : POSITION0, out float4 opos : POSITION0, out float2 otex : TEXCOORD0)
{
	opos = pos;
	otex = (pos.xy * 0.5 + 0.5) * iResolution;
#if D3D9
	opos.xy += float2(-1,1) / iResolution;
#endif
}
#endif
#elif PS

float csgUnion(float d1, float d2)
{
	return min(d1, d2);
}
float csgSubtract(float d1, float d2)
{
	return max(-d1, d2);
}
float csgIntersect(float d1, float d2)
{
	return max(d1, d2);
}

float smin_poly(float a, float b, float k)
{
	float h = saturate(0.5 + 0.5 * (b - a) / k);
	return lerp(b, a, h) - k * h * (1.0 - h);
}

float planeSDF(float3 s, float3 n, float d)
{
	return dot(s, n) - d;
}

float boxSDF(float3 s, float3 p, float3 e)
{
	return length(max(abs(s - p) - e, 0));
}

float rboxSDF(float3 s, float3 p, float3 e, float r)
{
	return length(max(abs(s - p) - e, 0)) - r;
}

float sphereSDF(float3 s, float3 p, float r)
{
	return length(s - p) - r;
}

float cylinderSDF(float3 p, float3 c)
{
	return length(p.xz - c.xy) - c.z;
}

float length8(float2 v)
{
	float2 v2 = v * v;
	float2 v4 = v2 * v2;
	float2 v8 = v4 * v4;
	return pow(dot(v8, 1), 1.0 / 8);
}

float torusSDF(float3 p, float one, float two)
{
	float2 q = float2(length(p.xz) - one, p.y);
	return length8(q) - two;
}

float3 extendBehind(float3 p, float3 n, float d)
{
	float behind = min(dot(p, n) - d, 0.0);
	return p + n * -behind;
}

float2 mod(float2 x, float2 y)
{
	return x - y * floor(x/y);
}
float3 xymod(float3 p, float m)
{
	p.xy = mod(p.xy, m) - 0.5 * m;
	return p;
}

#define PI 3.14159

float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898,78.233))) * 43758.5453);
}

float sceneSDF(float3 s)
{
	//return min(smin_poly(boxSDF(s, 0, 1), sphereSDF(s, 0, 1.1), 0.9), planeSDF(s, float3(0, 1, 0), -1));
	//return min(rboxSDF(s, 0, 1, 0.04), planeSDF(s, float3(0, 0, 1), -1));
	float3 rs = xymod(s, 4);
	float3 rs2 = xymod(s, 0.5);
	return csgUnion(
		csgUnion(
			torusSDF(extendBehind(rs, float3(0,0,1), 0), 0.5, 0.2),
			csgUnion(
				sphereSDF(rs, float3(0,0,0), 0.1),
				cylinderSDF(rs + float3(sin(rs.y*PI)*0.1,0,0), float3(0,0,0.02))
			)
		),
		csgIntersect(
			planeSDF(rs2, float3(rand(floor(s.xy)) * 0.02 - 0.01, rand(ceil(s.xy)) * 0.02 - 0.01, 1), -1),
			boxSDF(rs2, floor(rs2)+0.5, 0.5)
		)
	);
}

float raymarch(float3 eye, float3 dir, float start, float end)
{
	float depth = start;
	for (int i = 0; i < 256; ++i)
	{
		float dst = sceneSDF(eye + depth * dir);
		if (dst < 0.00001)
			return depth;
		depth += dst;
		if (depth >= end)
			return end;
	}
	return end;
}

float computeAO(float3 p, float3 n)
{
	float ao = 0;
	float delta = 0.2;
	for (int i = 0; i < 16; i++)
	{
		int i1 = i + 1;
		float diff = i1 * delta - sceneSDF(p + n * i1 * delta);
		ao += diff / exp2(i1);
	}
	return 1 - ao * 1.5;
}

float computeSoftShadow(float3 p, float3 dir, float start, float k)
{
	float res = 1.0;
	float t = start;
	for (int i = 0; i < 32; ++i)
	{
		float h = sceneSDF(p + dir * t);
		if (h < 0.001)
			return 0.0;
		res = min(res, k * h / t);
		t += h;
	}
	return res;
}

float3 rayDir(float fov, float2 size, float2 coord)
{
	float2 xy = coord - size / 2;
	float z = size.y / tan(radians(fov) / 2);
	return normalize(float3(xy, -z));
}

void main(out float4 outCol : COLOR0, in float2 coord : TEXCOORD0)
{
	float fov = 60;
	if (coord.y > iResolution.y - 10)
	{
		float3 v1 = { -0.9, 0.3, 1.6 };
		if (coord.x < 10)
		{
			outCol = float4(frac(v1), 1);
			return;
		}
		else if (coord.x < 20)
		{
			outCol = float4(v1 % 2, 1);
			return;
		}
	}
	float3 dir = mul(float4(rayDir(fov, iResolution, coord),0), viewMatrix);
	//float3 dir = rayDir(fov, iResolution, coord);
	float3 eye = mul(float4(0,0,0,1), viewMatrix);
	float dist = raymarch(eye, dir, 0, 100);
	if (dist >= 100-0.001)
	{
		outCol = float4(0,0,0,1);
		return;
	}
	float3 pos = eye + dir * dist;
	float3 normal = normalize(float3(
		sceneSDF(pos + float3(0.0001, 0, 0)) - sceneSDF(pos - float3(0.0001, 0, 0)),
		sceneSDF(pos + float3(0, 0.0001, 0)) - sceneSDF(pos - float3(0, 0.0001, 0)),
		sceneSDF(pos + float3(0, 0, 0.0001)) - sceneSDF(pos - float3(0, 0, 0.0001))
	));
	float3 lightPos = float3(0,0,8);
	float3 lightDir = normalize(lightPos - pos);//normalize(float3(2,3,4));
	float NdotL = saturate(dot(normal,lightDir));
	float NdotH = saturate(dot(normal,normalize(lightDir + normalize(eye - pos))));
	float NdotV = saturate(dot(normal,normalize(eye - pos)));
	float fresnel = lerp(0.5, 1, pow(1 - NdotV, 5));
	float ao = computeAO(pos, normal);
	float shadow = computeSoftShadow(pos, lightDir, 0.03, 20);
	float distf = 20.0 / pow(length(lightPos - pos),2);
	float3 col = (NdotL + pow(NdotH + 0.04, 128) * fresnel) * shadow * distf * float3(0.9,0.4,0.1);
	col += ao * float3(0.06,0.12,0.24);
	col = col / (1 + col);
	outCol = float4(sqrt(col)-0.05,1);//float4(coord.x % 2, coord.y % 2,1,1);
}

#endif
