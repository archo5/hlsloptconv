
#include <windows.h>
#include <d3d9.h>
#if USE_D3DXCOMPILESHADER
#  include <d3dx9.h>
#  pragma comment(lib, "d3dx9.lib")
#endif
#include <d3d11.h>
#include <d3dcompiler.h>
#include <gl/gl.h>

#include "../compiler.hpp"

#define WINDOW_WIDTH (640+640)
#define WINDOW_HEIGHT (16+360+16+360)
#define PART_WIDTH 640
#define PART_HEIGHT 360

const char* SHADER_NAME = "runtests/html5-shader.hlsl";

HINSTANCE g_HInstance = nullptr;
HWND g_MainWindow = nullptr;
HFONT g_Font = nullptr;

float FULLSCREEN_TRIANGLE_VERTICES[] =
{
	-1, -1, 0.5f,
	3, -1, 0.5f,
	-1, 3, 0.5f,
};

uint8_t CUBE_MAP_FACE[16] =
{
	64,192,64,192,
	192,64,192,64,
	64,192,64,192,
	192,64,192,64,
};


#ifndef FORCEINLINE
#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline __attribute__((__always_inline__))
#endif
#endif

#define SMALL_FLOAT 0.001f

struct Vec3
{
	float x, y, z;

	FORCEINLINE Vec3 operator + () const { return *this; }
	FORCEINLINE Vec3 operator - () const { Vec3 v = { -x, -y, -z }; return v; }

	FORCEINLINE Vec3 operator + ( const Vec3& o ) const { Vec3 v = { x + o.x, y + o.y, z + o.z }; return v; }
	FORCEINLINE Vec3 operator - ( const Vec3& o ) const { Vec3 v = { x - o.x, y - o.y, z - o.z }; return v; }
	FORCEINLINE Vec3 operator * ( const Vec3& o ) const { Vec3 v = { x * o.x, y * o.y, z * o.z }; return v; }
	FORCEINLINE Vec3 operator / ( const Vec3& o ) const { Vec3 v = { x / o.x, y / o.y, z / o.z }; return v; }

	FORCEINLINE Vec3 operator + ( float f ) const { Vec3 v = { x + f, y + f, z + f }; return v; }
	FORCEINLINE Vec3 operator - ( float f ) const { Vec3 v = { x - f, y - f, z - f }; return v; }
	FORCEINLINE Vec3 operator * ( float f ) const { Vec3 v = { x * f, y * f, z * f }; return v; }
	FORCEINLINE Vec3 operator / ( float f ) const { Vec3 v = { x / f, y / f, z / f }; return v; }

	FORCEINLINE Vec3& operator += ( const Vec3& o ){ x += o.x; y += o.y; z += o.z; return *this; }
	FORCEINLINE Vec3& operator -= ( const Vec3& o ){ x -= o.x; y -= o.y; z -= o.z; return *this; }
	FORCEINLINE Vec3& operator *= ( const Vec3& o ){ x *= o.x; y *= o.y; z *= o.z; return *this; }
	FORCEINLINE Vec3& operator /= ( const Vec3& o ){ x /= o.x; y /= o.y; z /= o.z; return *this; }

	FORCEINLINE Vec3& operator += ( float f ){ x += f; y += f; z += f; return *this; }
	FORCEINLINE Vec3& operator -= ( float f ){ x -= f; y -= f; z -= f; return *this; }
	FORCEINLINE Vec3& operator *= ( float f ){ x *= f; y *= f; z *= f; return *this; }
	FORCEINLINE Vec3& operator /= ( float f ){ x /= f; y /= f; z /= f; return *this; }

	FORCEINLINE bool operator == ( const Vec3& o ) const { return x == o.x && y == o.y && z == o.z; }
	FORCEINLINE bool operator != ( const Vec3& o ) const { return x != o.x || y != o.y || z != o.z; }

	FORCEINLINE bool IsZero() const { return x == 0 && y == 0 && z == 0; }
	FORCEINLINE bool NearZero() const { return fabs(x) < SMALL_FLOAT && fabs(y) < SMALL_FLOAT && fabs(z) < SMALL_FLOAT; }
	FORCEINLINE float LengthSq() const { return x * x + y * y + z * z; }
	FORCEINLINE float Length() const { return sqrtf( LengthSq() ); }
	FORCEINLINE Vec3 Normalized() const
	{
		float lensq = LengthSq();
		if( lensq == 0 )
		{
			Vec3 v = { 0, 0, 0 };
			return v;
		}
		float invlen = 1.0f / sqrtf( lensq );
		Vec3 v = { x * invlen, y * invlen, z * invlen };
		return v;
	}

	void Dump( FILE* f ) const
	{
		fprintf( f, "Vec3 ( %.2f %.2f %.2f )\n", x, y, z );
	}
};

FORCEINLINE Vec3 operator + ( float f, const Vec3& v ){ Vec3 out = { f + v.x, f + v.y, f + v.z }; return out; }
FORCEINLINE Vec3 operator - ( float f, const Vec3& v ){ Vec3 out = { f - v.x, f - v.y, f - v.z }; return out; }
FORCEINLINE Vec3 operator * ( float f, const Vec3& v ){ Vec3 out = { f * v.x, f * v.y, f * v.z }; return out; }
FORCEINLINE Vec3 operator / ( float f, const Vec3& v ){ Vec3 out = { f / v.x, f / v.y, f / v.z }; return out; }

static FORCEINLINE Vec3 V3( float x ){ Vec3 o = { x, x, x }; return o; }
static FORCEINLINE Vec3 V3( float x, float y, float z ){ Vec3 o = { x, y, z }; return o; }
static FORCEINLINE Vec3 V3P( const float* x ){ Vec3 o = { x[0], x[1], x[2] }; return o; }

FORCEINLINE float Vec3Dot( const Vec3& v1, const Vec3& v2 ){ return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
FORCEINLINE Vec3 Vec3Cross( const Vec3& v1, const Vec3& v2 )
{
	Vec3 out =
	{
		v1.y * v2.z - v1.z * v2.y,
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x,
	};
	return out;
}

void LookAtMatrixInv(Vec3 eye, Vec3 at, Vec3 up, float outMtx[16])
{
	Vec3 zaxis = (eye - at).Normalized();
	Vec3 xaxis = Vec3Cross(zaxis, up).Normalized();
	Vec3 yaxis = Vec3Cross(xaxis, zaxis);

	outMtx[0] = xaxis.x; outMtx[1] = yaxis.x; outMtx[2] = zaxis.x; outMtx[3] = eye.x;
	outMtx[4] = xaxis.y; outMtx[5] = yaxis.y; outMtx[6] = zaxis.y; outMtx[7] = eye.y;
	outMtx[8] = xaxis.z; outMtx[9] = yaxis.z; outMtx[10] = zaxis.z; outMtx[11] = eye.z;
	outMtx[12] = outMtx[13] = outMtx[14] = 0; outMtx[15] = 1;
}


struct TitleInfo
{
	const char* name;
	int x, y;
};
void Paint(HWND window)
{
	TitleInfo titles[] =
	{
		{ "Direct3D 9 (SM 3.0)", 0, 0 },
		{ "Direct3D 11 (SM 4.0, feature level 10)", PART_WIDTH, 0 },
		{ "OpenGL 2.0 (GLSL 1.00)", 0, PART_HEIGHT+16 },
		{ "OpenGL 3.1 (GLSL 1.40)", PART_WIDTH, PART_HEIGHT+16 },
	};

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(window, &ps);
	SelectObject(hdc, g_Font);
	SetBkMode(hdc, TRANSPARENT);
	SetBkColor(hdc, RGB(0,0,0));
	SetTextAlign(hdc, TA_CENTER);
	SetTextColor(hdc, RGB(200,200,200));
	for (TitleInfo& t : titles)
	{
		TRIVERTEX verts[4] =
		{
			{ t.x, t.y, 0x10<<8, 0x20<<8, 0x40<<8, 0xff00 },
			{ t.x + PART_WIDTH/2, t.y + 16, 0x10<<8, 0x10<<8, 0x10<<8, 0xff00 },
			{ t.x + PART_WIDTH/2, t.y, 0x10<<8, 0x10<<8, 0x10<<8, 0xff00 },
			{ t.x + PART_WIDTH, t.y + 16, 0x10<<8, 0x20<<8, 0x40<<8, 0xff00 },
		};
		GRADIENT_RECT gradRects[2] = { { 0, 1 }, { 2, 3 } };
		GradientFill(hdc, verts, 4, gradRects, 2, GRADIENT_FILL_RECT_H);
		TextOut(hdc, t.x + PART_WIDTH / 2, t.y, t.name, strlen(t.name));
	}
	EndPaint(window, &ps);
}
void ExitImminentFreeOpenGL();
LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_CLOSE:
		ExitImminentFreeOpenGL();
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		Paint(window);
		return 0;
	}

	return DefWindowProc(window, msg, wp, lp);
}

LRESULT CALLBACK SubWindowProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	}

	return DefWindowProc(window, msg, wp, lp);
}


void _Check_Succeeded(HRESULT hr, const char* code, int line)
{
	if (FAILED(hr))
	{
		fprintf(stderr, "HRESULT (hr=0x%08X) call failed (line %d): %s\n", hr, line, code);
		exit(1);
	}
}
#define CHK(x) _Check_Succeeded(x, #x, __LINE__)


void _WinCheck_Succeeded(BOOL result, const char* code, int line)
{
	if (!result)
	{
		DWORD error = GetLastError();
		fprintf(stderr, "Win32 call failed (error=%08X, line %d): %s\n", error, line, code);
		exit(1);
	}
}
#define WINCHK(x) _WinCheck_Succeeded(x, #x, __LINE__)


namespace D3D9
{
	HWND                    apiWin = nullptr;
	IDirect3D9*             d3d    = nullptr;
	IDirect3DDevice9*       dev    = nullptr;
	IDirect3DCubeTexture9*  cmtex  = nullptr;
	IDirect3DVertexShader9* vs     = nullptr;
	IDirect3DPixelShader9*  ps     = nullptr;

	void CompileShader(bool pixelShader)
	{
		Compiler compiler;
		FILEStream errStream(stderr);
		StringStream codeStream;
		compiler.errorOutputStream = &errStream;
		compiler.codeOutputStream = &codeStream;
		compiler.outputFmt = OSF_HLSL_SM3;
		compiler.stage = pixelShader ? ShaderStage_Pixel : ShaderStage_Vertex;
		ShaderMacro macros[] = { { pixelShader ? "PS" : "VS", "1" }, { "D3D9", "1" }, { nullptr, nullptr } };
		compiler.defines = macros;
		String inCode = GetFileContents(SHADER_NAME, true);
		if (!compiler.CompileFile(SHADER_NAME, inCode.c_str()))
		{
			fprintf(stderr, "compilation failed, no output generated\n");
			exit(1);
		}
#if USE_D3DXCOMPILESHADER
		ID3DXBuffer* codeBuf = nullptr;
		ID3DXBuffer* errBuf = nullptr;
	//	puts(codeStream.str().c_str());
		D3DXCompileShader(codeStream.str().c_str(), codeStream.str().size(), nullptr, nullptr,
	//	D3DXMACRO xmacros[] = { { pixelShader ? "PS" : "VS", "1" }, { "D3D9", "1" }, { nullptr, nullptr } };
	//	D3DXCompileShader(inCode.c_str(), inCode.size(), xmacros, nullptr,
			"main", pixelShader ? "ps_3_0" : "vs_3_0",
			/* slow otherwise: */ D3DXSHADER_SKIPOPTIMIZATION, &codeBuf, &errBuf, nullptr);
#else
		ID3DBlob* codeBuf = nullptr;
		ID3DBlob* errBuf = nullptr;
		D3DCompile(codeStream.str().c_str(), codeStream.str().size(), SHADER_NAME, nullptr, nullptr,
			"main", pixelShader ? "ps_3_0" : "vs_3_0", 0, 0, &codeBuf, &errBuf);
#endif
		if (errBuf)
		{
			fprintf(stderr, "D3D9 shader compilation failed, errors:\n%.*s\n",
				int(errBuf->GetBufferSize()), (const char*) errBuf->GetBufferPointer());
			errBuf->Release();
		}
		if (!codeBuf)
		{
			fprintf(stderr, "D3D9 shader compilation failed, no output generated\n");
			exit(1);
		}
		if (pixelShader)
			CHK(dev->CreatePixelShader((const DWORD*) codeBuf->GetBufferPointer(), &ps));
		else
			CHK(dev->CreateVertexShader((const DWORD*) codeBuf->GetBufferPointer(), &vs));
		codeBuf->Release();
	}

	void Init()
	{
		printf("[%f] D3D9: init\n", GetTime());
		apiWin = CreateWindowA("SubWindowClass", "Direct3D 9", WS_CHILD | WS_VISIBLE,
			0, 16, PART_WIDTH, PART_HEIGHT, g_MainWindow, nullptr, g_HInstance, nullptr);

		d3d = Direct3DCreate9(D3D_SDK_VERSION);
		D3DPRESENT_PARAMETERS pp;
		memset(&pp, 0, sizeof(pp));
		pp.Windowed = TRUE;
		pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		pp.hDeviceWindow = apiWin;
		pp.BackBufferFormat = D3DFMT_X8R8G8B8;
		pp.BackBufferWidth = PART_WIDTH;
		pp.BackBufferHeight = PART_HEIGHT;
		CHK(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, apiWin,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &dev));

		CHK(dev->CreateCubeTexture(4, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &cmtex, nullptr));
		for (int i = 0; i < 6; ++i)
		{
			D3DLOCKED_RECT lr;
			cmtex->LockRect((D3DCUBEMAP_FACES) i, 0, &lr, nullptr, D3DLOCK_DISCARD);
			char* p = (char*) lr.pBits;
			for (int j = 0; j < 4; ++j)
			{
				memcpy(p, &CUBE_MAP_FACE[j * 4], 4);
				p += lr.Pitch;
			}
			cmtex->UnlockRect((D3DCUBEMAP_FACES) i, 0);
		}

		printf("[%f] D3D9: compiling shaders\n", GetTime());
		CompileShader(false);
		CompileShader(true);
		printf("[%f] D3D9: init done\n", GetTime());
	}

	void Free()
	{
		printf("[%f] D3D9: cleanup\n", GetTime());
		cmtex->Release();
		ps->Release();
		vs->Release();
		dev->Release();
		d3d->Release();
		DestroyWindow(apiWin);
		printf("[%f] D3D9: cleanup done\n", GetTime());
	}

	void Render(float mtx[16])
	{
		dev->SetRenderState(D3DRS_LIGHTING, FALSE);
		dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(10, 20, 140), 1.0f, 0);
		dev->BeginScene();
		dev->SetVertexShader(vs);
		dev->SetPixelShader(ps);
		float reso[4] = { PART_WIDTH, PART_HEIGHT, 0, 0 };
	//	float mtx[16] = {
	//		-0.7991666718346001, 0.22324646365622025, -0.5581161591405507, -6.011094996993548, 0.6011094996993547, 0.2968030507723102, -0.7420076269307755, -7.991666718346001, 0, 0.9284766908852593, 0.3713906763541037, 4, 0, 0, 0, 1
	//	};
		dev->SetVertexShaderConstantF(0, reso, 1);
		dev->SetPixelShaderConstantF(0, mtx, 4);
		dev->SetPixelShaderConstantF(3, reso, 1);
		dev->SetTexture(0, cmtex);
		dev->SetFVF(D3DFVF_XYZ);
		dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, FULLSCREEN_TRIANGLE_VERTICES, sizeof(float) * 3);
		dev->EndScene();
		dev->Present(nullptr, nullptr, nullptr, nullptr);
	}
}


struct CBufData
{
	float resX, resY, pad[2];
	float viewMtx[16];
};


namespace D3D11
{
	HWND                     apiWin      = nullptr;
	ID3D11Device*            dev         = nullptr;
	ID3D11DeviceContext*     ctx         = nullptr;
	IDXGISwapChain*          swapChain   = nullptr;
	ID3D11RenderTargetView*  backBufRTV  = nullptr;
	ID3D11RasterizerState*   rasterState = nullptr;
	ID3D11BlendState*        blendState  = nullptr;
	ID3D11DepthStencilState* dsState     = nullptr;
	ID3D11Buffer*            cbuf        = nullptr;
	ID3D11Texture2D*         cmtex       = nullptr;
	ID3D11ShaderResourceView*cmsrv       = nullptr;
	ID3D11SamplerState*      cmsmp       = nullptr;
	ID3D11VertexShader*      vs          = nullptr;
	ID3D11PixelShader*       ps          = nullptr;

	void CompileShader(bool pixelShader)
	{
		Compiler compiler;
		FILEStream errStream(stderr);
		StringStream codeStream;
		compiler.errorOutputStream = &errStream;
		compiler.codeOutputStream = &codeStream;
		compiler.outputFmt = OSF_HLSL_SM4;
		compiler.stage = pixelShader ? ShaderStage_Pixel : ShaderStage_Vertex;
		ShaderMacro macros[] = { { pixelShader ? "PS" : "VS", "1" }, { "D3D11", "1" }, { nullptr, nullptr } };
		compiler.defines = macros;
		String inCode = GetFileContents(SHADER_NAME, true);
		if (!compiler.CompileFile(SHADER_NAME, inCode.c_str()))
		{
			fprintf(stderr, "compilation failed, no output generated\n");
			exit(1);
		}
		ID3DBlob* codeBuf = nullptr;
		ID3DBlob* errBuf = nullptr;
		D3DCompile(codeStream.str().c_str(), codeStream.str().size(), SHADER_NAME, nullptr, nullptr,
			"main", pixelShader ? "ps_4_0" : "vs_4_0", 0, 0, &codeBuf, &errBuf);
		if (errBuf)
		{
			fprintf(stderr, "D3D11 shader compilation failed, errors:\n%.*s\n",
				int(errBuf->GetBufferSize()), (const char*) errBuf->GetBufferPointer());
			errBuf->Release();
		}
		if (!codeBuf)
		{
			fprintf(stderr, "D3D11 shader compilation failed, no output generated\n");
			exit(1);
		}
		if (pixelShader)
			CHK(dev->CreatePixelShader((const DWORD*) codeBuf->GetBufferPointer(),
				codeBuf->GetBufferSize(), nullptr, &ps));
		else
			CHK(dev->CreateVertexShader((const DWORD*) codeBuf->GetBufferPointer(),
				codeBuf->GetBufferSize(), nullptr, &vs));
		codeBuf->Release();
	}

	void Init()
	{
		printf("[%f] D3D11: init\n", GetTime());
		apiWin = CreateWindowA("SubWindowClass", "Direct3D 11", WS_CHILD | WS_VISIBLE,
			PART_WIDTH, 16, PART_WIDTH, PART_HEIGHT, g_MainWindow, nullptr, g_HInstance, nullptr);

		DXGI_SWAP_CHAIN_DESC scd;
		memset(&scd, 0, sizeof(scd));
		scd.BufferDesc.Width = PART_WIDTH;
		scd.BufferDesc.Height = PART_HEIGHT;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.RefreshRate.Numerator = 60;
		scd.BufferDesc.RefreshRate.Denominator = 1;
		scd.SampleDesc.Count = 1;
		scd.SampleDesc.Quality = 0;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 2;
		scd.OutputWindow = apiWin;
		scd.Windowed = TRUE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_0 };
		CHK(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_DEBUG, featureLevels, 1, D3D11_SDK_VERSION, &scd, &swapChain, &dev, nullptr, &ctx));

		ID3D11Texture2D* backBuf;
		D3D11_RENDER_TARGET_VIEW_DESC rtvd;
		memset(&rtvd, 0, sizeof(rtvd));
		rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		CHK(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &backBuf));
		CHK(dev->CreateRenderTargetView(backBuf, &rtvd, &backBufRTV));
		backBuf->Release();

		D3D11_RASTERIZER_DESC rd;
		memset(&rd, 0, sizeof(rd));
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		CHK(dev->CreateRasterizerState(&rd, &rasterState));

		D3D11_DEPTH_STENCIL_DESC dsd;
		memset(&dsd, 0, sizeof(dsd));
		dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dsd.DepthFunc = D3D11_COMPARISON_ALWAYS;
		CHK(dev->CreateDepthStencilState(&dsd, &dsState));

		D3D11_BLEND_DESC bd;
		memset(&bd, 0, sizeof(bd));
		bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		CHK(dev->CreateBlendState(&bd, &blendState));

		D3D11_BUFFER_DESC cbd;
		cbd.ByteWidth = sizeof(CBufData);
		cbd.Usage = D3D11_USAGE_DYNAMIC;
		cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbd.MiscFlags = 0;
		CHK(dev->CreateBuffer(&cbd, NULL, &cbuf));

		D3D11_TEXTURE2D_DESC cmt2d;
		memset(&cmt2d, 0, sizeof(cmt2d));
		cmt2d.Width = 4;
		cmt2d.Height = 4;
		cmt2d.MipLevels = 1;
		cmt2d.ArraySize = 6;
		cmt2d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		cmt2d.SampleDesc.Count = 1;
		cmt2d.Usage = D3D11_USAGE_IMMUTABLE;
		cmt2d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		cmt2d.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		D3D11_SUBRESOURCE_DATA cmsrd[6];
		uint32_t convdata[16];
		for (int i = 0; i < 16; ++i)
		{
			convdata[i] = (0xff << 24) | (CUBE_MAP_FACE[i] << 16)
				| (CUBE_MAP_FACE[i] << 8) | (CUBE_MAP_FACE[i]);
		}
		for (int i = 0; i < 6; ++i)
		{
			cmsrd[i].pSysMem = convdata;
			cmsrd[i].SysMemPitch = 16;
			cmsrd[i].SysMemSlicePitch = 0;
		}
		CHK(dev->CreateTexture2D(&cmt2d, cmsrd, &cmtex));
		CHK(dev->CreateShaderResourceView(cmtex, nullptr, &cmsrv));

		D3D11_SAMPLER_DESC cmsmd;
		memset(&cmsmd, 0, sizeof(cmsmd));
		cmsmd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		cmsmd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		cmsmd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		cmsmd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		cmsmd.ComparisonFunc = D3D11_COMPARISON_NEVER;
		CHK(dev->CreateSamplerState(&cmsmd, &cmsmp));

		printf("[%f] D3D11: compiling shaders\n", GetTime());
		CompileShader(false);
		CompileShader(true);
		printf("[%f] D3D11: init done\n", GetTime());
	}

	void Free()
	{
		printf("[%f] D3D11: cleanup\n", GetTime());
		cmsmp->Release();
		cmsrv->Release();
		cmtex->Release();
		ps->Release();
		vs->Release();
		cbuf->Release();
		blendState->Release();
		dsState->Release();
		rasterState->Release();
		backBufRTV->Release();
		swapChain->Release();
		ctx->Release();
		dev->Release();
		DestroyWindow(apiWin);
		printf("[%f] D3D11: cleanup done\n", GetTime());
	}

	void Render(float mtx[16])
	{
		float col[4] = { 10/255.0f, 20/255.0f, 180/255.0f, 1.0f };
		ctx->ClearRenderTargetView(backBufRTV, col);

		CBufData bufData;
		D3D11_MAPPED_SUBRESOURCE mapRsrc;
		CHK(ctx->Map(cbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapRsrc));
		bufData.resX = PART_WIDTH;
		bufData.resY = PART_HEIGHT;
		memcpy(bufData.viewMtx, mtx, sizeof(bufData.viewMtx));
		memcpy(mapRsrc.pData, &bufData, sizeof(bufData));
		ctx->Unmap(cbuf, 0);

		D3D11_VIEWPORT vp;
		memset(&vp, 0, sizeof(vp));
		vp.Width = PART_WIDTH;
		vp.Height = PART_HEIGHT;
		vp.MaxDepth = 1;
		ctx->RSSetViewports(1, &vp);
		ctx->VSSetShader(vs, nullptr, 0);
		ctx->PSSetShader(ps, nullptr, 0);
		ctx->VSSetConstantBuffers(0, 1, &cbuf);
		ctx->PSSetConstantBuffers(0, 1, &cbuf);
		ctx->PSSetShaderResources(0, 1, &cmsrv);
		ctx->PSSetSamplers(0, 1, &cmsmp);
		ctx->IASetInputLayout(nullptr);
		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx->RSSetState(rasterState);
		ctx->OMSetRenderTargets(1, &backBufRTV, nullptr);
		ctx->OMSetDepthStencilState(dsState, 0);
		ctx->OMSetBlendState(blendState, nullptr, 0xffffffff);
		ctx->Draw(3, 0);

		swapChain->Present(1, 0);
	}
}


// imported {
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_FLAGS_ARB             0x2094

typedef const char *(WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#define GL_TEXTURE_CUBE_MAP               0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X    0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X    0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y    0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y    0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z    0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z    0x851A
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_INVALID_INDEX                  0xFFFFFFFFu
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRYP PFNGLBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);

typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLVALIDATEPROGRAMPROC) (GLuint program);

typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);

typedef GLuint (APIENTRYP PFNGLGETUNIFORMBLOCKINDEXPROC) (GLuint program, const GLchar *uniformBlockName);
typedef void (APIENTRYP PFNGLUNIFORMBLOCKBINDINGPROC) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
typedef void (APIENTRYP PFNGLBINDBUFFERBASEPROC) (GLenum target, GLuint index, GLuint buffer);
// } imported

GLenum g_CubeMapBindPoints[6] =
{
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

void _GLCheck_Succeeded(const char* code, int line)
{
	auto err = glGetError();
	if (err != GL_NO_ERROR)
	{
		fprintf(stderr, "GL call failed (error=%X, line %d): %s\n", err, line, code);
		exit(1);
	}
}
#define GLCHK(x) x;_GLCheck_Succeeded(#x, __LINE__)


namespace GL20
{
	HWND   apiWin = nullptr;
	HDC    dc     = nullptr;
	HGLRC  glrc   = nullptr;
	GLuint vbo    = 0;
	GLuint cmtex  = 0;
	GLuint vs     = 0;
	GLuint ps     = 0;
	GLuint prog   = 0;

	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	PFNGLBINDBUFFERPROC glBindBuffer;
	PFNGLDELETEBUFFERSPROC glDeleteBuffers;
	PFNGLGENBUFFERSPROC glGenBuffers;
	PFNGLBUFFERDATAPROC glBufferData;
	PFNGLBUFFERSUBDATAPROC glBufferSubData;

	PFNGLATTACHSHADERPROC glAttachShader;
	PFNGLCOMPILESHADERPROC glCompileShader;
	PFNGLCREATEPROGRAMPROC glCreateProgram;
	PFNGLCREATESHADERPROC glCreateShader;
	PFNGLDELETEPROGRAMPROC glDeleteProgram;
	PFNGLDELETESHADERPROC glDeleteShader;
	PFNGLDETACHSHADERPROC glDetachShader;
	PFNGLGETPROGRAMIVPROC glGetProgramiv;
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	PFNGLGETSHADERIVPROC glGetShaderiv;
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	PFNGLLINKPROGRAMPROC glLinkProgram;
	PFNGLSHADERSOURCEPROC glShaderSource;
	PFNGLUSEPROGRAMPROC glUseProgram;
	PFNGLUNIFORM2FPROC glUniform2f;
	PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
	PFNGLVALIDATEPROGRAMPROC glValidateProgram;

	void LoadExtensions()
	{
		*(void**)&glVertexAttribPointer = wglGetProcAddress("glVertexAttribPointer");
		*(void**)&glEnableVertexAttribArray = wglGetProcAddress("glEnableVertexAttribArray");
		*(void**)&glBindBuffer = wglGetProcAddress("glBindBuffer");
		*(void**)&glDeleteBuffers = wglGetProcAddress("glDeleteBuffers");
		*(void**)&glGenBuffers = wglGetProcAddress("glGenBuffers");
		*(void**)&glBufferData = wglGetProcAddress("glBufferData");
		*(void**)&glBufferSubData = wglGetProcAddress("glBufferSubData");

		*(void**)&glAttachShader = wglGetProcAddress("glAttachShader");
		*(void**)&glCompileShader = wglGetProcAddress("glCompileShader");
		*(void**)&glCreateProgram = wglGetProcAddress("glCreateProgram");
		*(void**)&glCreateShader = wglGetProcAddress("glCreateShader");
		*(void**)&glDeleteProgram = wglGetProcAddress("glDeleteProgram");
		*(void**)&glDeleteShader = wglGetProcAddress("glDeleteShader");
		*(void**)&glDetachShader = wglGetProcAddress("glDetachShader");
		*(void**)&glGetProgramiv = wglGetProcAddress("glGetProgramiv");
		*(void**)&glGetProgramInfoLog = wglGetProcAddress("glGetProgramInfoLog");
		*(void**)&glGetShaderiv = wglGetProcAddress("glGetShaderiv");
		*(void**)&glGetShaderInfoLog = wglGetProcAddress("glGetShaderInfoLog");
		*(void**)&glGetUniformLocation = wglGetProcAddress("glGetUniformLocation");
		*(void**)&glLinkProgram = wglGetProcAddress("glLinkProgram");
		*(void**)&glShaderSource = wglGetProcAddress("glShaderSource");
		*(void**)&glUseProgram = wglGetProcAddress("glUseProgram");
		*(void**)&glUniform2f = wglGetProcAddress("glUniform2f");
		*(void**)&glUniformMatrix4fv = wglGetProcAddress("glUniformMatrix4fv");
		*(void**)&glValidateProgram = wglGetProcAddress("glValidateProgram");
	}

	void CompileShader(bool pixelShader)
	{
		Compiler compiler;
		FILEStream errStream(stderr);
		StringStream codeStream;
		compiler.errorOutputStream = &errStream;
		compiler.codeOutputStream = &codeStream;
		compiler.outputFmt = OSF_GLSL_ES_100;
		compiler.stage = pixelShader ? ShaderStage_Pixel : ShaderStage_Vertex;
		ShaderMacro macros[] = { { pixelShader ? "PS" : "VS", "1" }, { "GL20", "1" }, { nullptr, nullptr } };
		compiler.defines = macros;
		String inCode = GetFileContents(SHADER_NAME, true);
		if (!compiler.CompileFile(SHADER_NAME, inCode.c_str()))
		{
			fprintf(stderr, "compilation failed, no output generated\n");
			exit(1);
		}
	//	printf("%s ---------------\n%s\n----------------\n\n",
	//		pixelShader ? "PIXEL" : "VERTEX", codeStream.str().c_str());

		GLuint shader = GLCHK(glCreateShader(pixelShader ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER));
		const char* shaderSources[] = { codeStream.str().c_str() };
		GLCHK(glShaderSource(shader, 1, shaderSources, nullptr));
		GLCHK(glCompileShader(shader));
		GLint compileSuccessful = GL_FALSE;
		GLCHK(glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccessful));
		if (!compileSuccessful)
		{
			GLint infoLogLength;
			GLCHK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength));
			String log;
			log.resize(infoLogLength);
			GLCHK(glGetShaderInfoLog(shader, infoLogLength, nullptr, &log[0]));
			fprintf(stderr, "GLSL compilation failed:\n%s\n", log.c_str());
			exit(1);
		}

		if (pixelShader)
			ps = shader;
		else
			vs = shader;
	}

	void Init()
	{
		printf("[%f] GL2.0: init\n", GetTime());
		apiWin = CreateWindowA("SubWindowClass", "OpenGL 3.1", WS_CHILD | WS_VISIBLE,
			0, 16+PART_HEIGHT+16, PART_WIDTH, PART_HEIGHT, g_MainWindow, nullptr, g_HInstance, nullptr);

		dc = GetDC(apiWin);
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 32;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int pixelFmt = ChoosePixelFormat(dc, &pfd);
		if (pixelFmt == 0)
		{
			fprintf(stderr, "GL2.0 - could not choose a pixel format\n");
			exit(1);
		}
		if (!SetPixelFormat(dc, pixelFmt, &pfd))
		{
			fprintf(stderr, "GL2.0 - could not set the pixel format\n");
			exit(1);
		}
		glrc = wglCreateContext(dc);
		WINCHK(wglMakeCurrent(dc, glrc));

		LoadExtensions();

		GLCHK(glGenBuffers(1, &vbo));
		GLCHK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
		GLCHK(glBufferData(GL_ARRAY_BUFFER, sizeof(FULLSCREEN_TRIANGLE_VERTICES),
			FULLSCREEN_TRIANGLE_VERTICES, GL_STATIC_DRAW));
		GLCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));
		GLCHK(glEnableVertexAttribArray(0));

		GLCHK(glGenTextures(1, &cmtex));
		GLCHK(glBindTexture(GL_TEXTURE_CUBE_MAP, cmtex));
		GLCHK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCHK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		for (int i = 0; i < 6; ++i)
		{
			GLCHK(glTexImage2D(g_CubeMapBindPoints[i], 0, GL_LUMINANCE,
				4, 4, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, CUBE_MAP_FACE));
		}

		printf("[%f] GL2.0: compiling shaders\n", GetTime());
		CompileShader(false);
		CompileShader(true);
		printf("[%f] GL2.0: linking shaders\n", GetTime());
		prog = GLCHK(glCreateProgram());
		assert(prog != 0 && vs != 0 && ps != 0);
		GLCHK(glAttachShader(prog, vs));
		GLCHK(glAttachShader(prog, ps));
		GLCHK(glLinkProgram(prog));
		GLint linkSuccessful = GL_FALSE;
		GLCHK(glGetProgramiv(prog, GL_LINK_STATUS, &linkSuccessful));
		if (!linkSuccessful)
		{
			GLint infoLogLength;
			GLCHK(glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLength));
			String log;
			log.resize(infoLogLength);
			GLCHK(glGetProgramInfoLog(prog, infoLogLength, nullptr, &log[0]));
			fprintf(stderr, "GLSL program linking failed:\n%s\n", log.c_str());
			exit(1);
		}
		GLCHK(glValidateProgram(prog));
		GLint validationSuccessful = GL_FALSE;
		GLCHK(glGetProgramiv(prog, GL_VALIDATE_STATUS, &validationSuccessful));
		if (!validationSuccessful)
		{
			GLint infoLogLength;
			GLCHK(glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLength));
			String log;
			log.resize(infoLogLength);
			GLCHK(glGetProgramInfoLog(prog, infoLogLength, nullptr, &log[0]));
			fprintf(stderr, "GLSL program validation failed:\n%s\n", log.c_str());
			exit(1);
		}
		printf("[%f] GL2.0: init done\n", GetTime());
	}

	void Free()
	{
		printf("[%f] GL2.0: cleanup\n", GetTime());
		WINCHK(wglMakeCurrent(dc, glrc));
		GLCHK(glDeleteBuffers(1, &vbo));
		GLCHK(glDeleteTextures(1, &cmtex));
		GLCHK(glDetachShader(prog, vs));
		GLCHK(glDetachShader(prog, ps));
		GLCHK(glDeleteProgram(prog));
		GLCHK(glDeleteShader(vs));
		GLCHK(glDeleteShader(ps));
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(glrc);
		ReleaseDC(apiWin, dc);
		DestroyWindow(apiWin);
		printf("[%f] GL2.0: cleanup done\n", GetTime());
	}

	void Render(float mtx[16])
	{
		WINCHK(wglMakeCurrent(dc, glrc));
		GLCHK(glViewport(0, 0, PART_WIDTH, PART_HEIGHT));
		GLCHK(glClearColor(0.4f, 0.2f, 0.1f, 1.0f));
		GLCHK(glClearDepth(1.0f));
		GLCHK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		GLCHK(glUseProgram(prog));
		{
			GLint loc = GLCHK(glGetUniformLocation(prog, "iResolution"));
			assert(loc >= 0);
			GLCHK(glUniform2f(loc, PART_WIDTH, PART_HEIGHT));
		}
		{
			GLint loc = GLCHK(glGetUniformLocation(prog, "viewMatrix"));
			assert(loc >= 0);
			GLCHK(glUniformMatrix4fv(loc, 1, GL_FALSE, mtx));
		}
		GLCHK(glDrawArrays(GL_TRIANGLES, 0, 3));

		SwapBuffers(dc);
	}
}


namespace GL31
{
	HWND   apiWin = nullptr;
	HDC    dc     = nullptr;
	HGLRC  glrc   = nullptr;
	GLuint vao    = 0;
	GLuint vbo    = 0;
	GLuint ubo    = 0;
	GLuint cmtex  = 0;
	GLuint vs     = 0;
	GLuint ps     = 0;
	GLuint prog   = 0;

	bool GLIsExtSupported(const char* name, bool wgl = false)
	{
		const char* exts;
		if (wgl)
		{
			PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
			*(void**)&wglGetExtensionsStringARB = wglGetProcAddress("wglGetExtensionsStringARB");
			exts = wglGetExtensionsStringARB(dc);
		}
		else
		{
			exts = (const char*) glGetString(GL_EXTENSIONS);
		}
		auto* start = exts;
		size_t len = strlen(name);
		while (start)
		{
			auto* which = strstr(start, name);
			if (!which)
				break;
			if ((which == start || which[-1] == ' ') &&
				(which[len] == '\0' || which[len] == ' '))
				return true;
			start = which + 1;
		}
		return false;
	}

	PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
	PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
	PFNGLBINDBUFFERPROC glBindBuffer;
	PFNGLDELETEBUFFERSPROC glDeleteBuffers;
	PFNGLGENBUFFERSPROC glGenBuffers;
	PFNGLBUFFERDATAPROC glBufferData;
	PFNGLBUFFERSUBDATAPROC glBufferSubData;

	PFNGLATTACHSHADERPROC glAttachShader;
	PFNGLCOMPILESHADERPROC glCompileShader;
	PFNGLCREATEPROGRAMPROC glCreateProgram;
	PFNGLCREATESHADERPROC glCreateShader;
	PFNGLDELETEPROGRAMPROC glDeleteProgram;
	PFNGLDELETESHADERPROC glDeleteShader;
	PFNGLDETACHSHADERPROC glDetachShader;
	PFNGLGETPROGRAMIVPROC glGetProgramiv;
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	PFNGLGETSHADERIVPROC glGetShaderiv;
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	PFNGLLINKPROGRAMPROC glLinkProgram;
	PFNGLSHADERSOURCEPROC glShaderSource;
	PFNGLUSEPROGRAMPROC glUseProgram;
	PFNGLVALIDATEPROGRAMPROC glValidateProgram;

	PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
	PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
	PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;

	PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
	PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
	PFNGLBINDBUFFERBASEPROC glBindBufferBase;

	void LoadExtensions()
	{
		*(void**)&glVertexAttribPointer = wglGetProcAddress("glVertexAttribPointer");
		*(void**)&glEnableVertexAttribArray = wglGetProcAddress("glEnableVertexAttribArray");
		*(void**)&glBindBuffer = wglGetProcAddress("glBindBuffer");
		*(void**)&glDeleteBuffers = wglGetProcAddress("glDeleteBuffers");
		*(void**)&glGenBuffers = wglGetProcAddress("glGenBuffers");
		*(void**)&glBufferData = wglGetProcAddress("glBufferData");
		*(void**)&glBufferSubData = wglGetProcAddress("glBufferSubData");

		*(void**)&glAttachShader = wglGetProcAddress("glAttachShader");
		*(void**)&glCompileShader = wglGetProcAddress("glCompileShader");
		*(void**)&glCreateProgram = wglGetProcAddress("glCreateProgram");
		*(void**)&glCreateShader = wglGetProcAddress("glCreateShader");
		*(void**)&glDeleteProgram = wglGetProcAddress("glDeleteProgram");
		*(void**)&glDeleteShader = wglGetProcAddress("glDeleteShader");
		*(void**)&glDetachShader = wglGetProcAddress("glDetachShader");
		*(void**)&glGetProgramiv = wglGetProcAddress("glGetProgramiv");
		*(void**)&glGetProgramInfoLog = wglGetProcAddress("glGetProgramInfoLog");
		*(void**)&glGetShaderiv = wglGetProcAddress("glGetShaderiv");
		*(void**)&glGetShaderInfoLog = wglGetProcAddress("glGetShaderInfoLog");
		*(void**)&glLinkProgram = wglGetProcAddress("glLinkProgram");
		*(void**)&glShaderSource = wglGetProcAddress("glShaderSource");
		*(void**)&glUseProgram = wglGetProcAddress("glUseProgram");
		*(void**)&glValidateProgram = wglGetProcAddress("glValidateProgram");

		*(void**)&glBindVertexArray = wglGetProcAddress("glBindVertexArray");
		*(void**)&glDeleteVertexArrays = wglGetProcAddress("glDeleteVertexArrays");
		*(void**)&glGenVertexArrays = wglGetProcAddress("glGenVertexArrays");

		*(void**)&glGetUniformBlockIndex = wglGetProcAddress("glGetUniformBlockIndex");
		*(void**)&glUniformBlockBinding = wglGetProcAddress("glUniformBlockBinding");
		*(void**)&glBindBufferBase = wglGetProcAddress("glBindBufferBase");
	}

	void CompileShader(bool pixelShader)
	{
		Compiler compiler;
		FILEStream errStream(stderr);
		StringStream codeStream;
		compiler.errorOutputStream = &errStream;
		compiler.codeOutputStream = &codeStream;
		compiler.outputFmt = OSF_GLSL_140;
		compiler.stage = pixelShader ? ShaderStage_Pixel : ShaderStage_Vertex;
		ShaderMacro macros[] = { { pixelShader ? "PS" : "VS", "1" }, { "GL31", "1" }, { nullptr, nullptr } };
		compiler.defines = macros;
		String inCode = GetFileContents(SHADER_NAME, true);
		if (!compiler.CompileFile(SHADER_NAME, inCode.c_str()))
		{
			fprintf(stderr, "compilation failed, no output generated\n");
			exit(1);
		}
	//	printf("%s ---------------\n%s\n----------------\n\n",
	//		pixelShader ? "PIXEL" : "VERTEX", codeStream.str().c_str());

		GLuint shader = GLCHK(glCreateShader(pixelShader ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER));
		const char* shaderSources[] = { codeStream.str().c_str() };
		GLCHK(glShaderSource(shader, 1, shaderSources, nullptr));
		GLCHK(glCompileShader(shader));
		GLint compileSuccessful = GL_FALSE;
		GLCHK(glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccessful));
		if (!compileSuccessful)
		{
			GLint infoLogLength;
			GLCHK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength));
			String log;
			log.resize(infoLogLength);
			GLCHK(glGetShaderInfoLog(shader, infoLogLength, nullptr, &log[0]));
			fprintf(stderr, "GLSL compilation failed:\n%s\n", log.c_str());
			exit(1);
		}

		if (pixelShader)
			ps = shader;
		else
			vs = shader;
	}

	void Init()
	{
		printf("[%f] GL3.1: init\n", GetTime());
		apiWin = CreateWindowA("SubWindowClass", "OpenGL 3.1", WS_CHILD | WS_VISIBLE,
			PART_WIDTH, 16+PART_HEIGHT+16, PART_WIDTH, PART_HEIGHT, g_MainWindow, nullptr, g_HInstance, nullptr);

		dc = GetDC(apiWin);
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 32;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int pixelFmt = ChoosePixelFormat(dc, &pfd);
		if (pixelFmt == 0)
		{
			fprintf(stderr, "GL3.1 - could not choose a pixel format\n");
			exit(1);
		}
		if (!SetPixelFormat(dc, pixelFmt, &pfd))
		{
			fprintf(stderr, "GL3.1 - could not set the pixel format\n");
			exit(1);
		}
		HGLRC tempContext = wglCreateContext(dc);
		WINCHK(wglMakeCurrent(dc, tempContext));
		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 1,
			WGL_CONTEXT_FLAGS_ARB, 0,
			0
		};

		if (GLIsExtSupported("WGL_ARB_create_context", true))
		{
			PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
			*(void**)&wglCreateContextAttribsARB = wglGetProcAddress("wglCreateContextAttribsARB");

			glrc = wglCreateContextAttribsARB(dc, 0, attribs);
			WINCHK(wglMakeCurrent(dc, glrc));
			wglDeleteContext(tempContext);
		}
		else
		{
			fprintf(stderr, "GL3.1 - WGL_ARB_create_context not supported\n");
			exit(1);
		}

		LoadExtensions();

		GLCHK(glGenVertexArrays(1, &vao));
		GLCHK(glBindVertexArray(vao));
		GLCHK(glGenBuffers(1, &vbo));
		GLCHK(glBindBuffer(GL_ARRAY_BUFFER, vbo));
		GLCHK(glBufferData(GL_ARRAY_BUFFER, sizeof(FULLSCREEN_TRIANGLE_VERTICES),
			FULLSCREEN_TRIANGLE_VERTICES, GL_STATIC_DRAW));
		GLCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));
		GLCHK(glEnableVertexAttribArray(0));

		GLCHK(glGenBuffers(1, &ubo));
		GLCHK(glBindBuffer(GL_UNIFORM_BUFFER, ubo));
		GLCHK(glBufferData(GL_UNIFORM_BUFFER, sizeof(CBufData), nullptr, GL_DYNAMIC_DRAW));

		GLCHK(glGenTextures(1, &cmtex));
		GLCHK(glBindTexture(GL_TEXTURE_CUBE_MAP, cmtex));
		GLCHK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		GLCHK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		for (int i = 0; i < 6; ++i)
		{
			GLCHK(glTexImage2D(g_CubeMapBindPoints[i], 0, GL_LUMINANCE,
				4, 4, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, CUBE_MAP_FACE));
		}

		printf("[%f] GL3.1: compiling shaders\n", GetTime());
		CompileShader(false);
		CompileShader(true);
		printf("[%f] GL3.1: linking shaders\n", GetTime());
		prog = GLCHK(glCreateProgram());
		assert(prog != 0 && vs != 0 && ps != 0);
		GLCHK(glAttachShader(prog, vs));
		GLCHK(glAttachShader(prog, ps));
		GLCHK(glLinkProgram(prog));
		GLint linkSuccessful = GL_FALSE;
		GLCHK(glGetProgramiv(prog, GL_LINK_STATUS, &linkSuccessful));
		if (!linkSuccessful)
		{
			GLint infoLogLength;
			GLCHK(glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLength));
			String log;
			log.resize(infoLogLength);
			GLCHK(glGetProgramInfoLog(prog, infoLogLength, nullptr, &log[0]));
			fprintf(stderr, "GLSL program linking failed:\n%s\n", log.c_str());
			exit(1);
		}
		GLCHK(glValidateProgram(prog));
		GLint validationSuccessful = GL_FALSE;
		GLCHK(glGetProgramiv(prog, GL_VALIDATE_STATUS, &validationSuccessful));
		if (!validationSuccessful)
		{
			GLint infoLogLength;
			GLCHK(glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLength));
			String log;
			log.resize(infoLogLength);
			GLCHK(glGetProgramInfoLog(prog, infoLogLength, nullptr, &log[0]));
			fprintf(stderr, "GLSL program validation failed:\n%s\n", log.c_str());
			exit(1);
		}
		GLuint ubidx = GLCHK(glGetUniformBlockIndex(prog, "uniformData"));
		assert(ubidx != GL_INVALID_INDEX);
		GLCHK(glUniformBlockBinding(prog, ubidx, 0));
		GLCHK(glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo));
		printf("[%f] GL3.1: init done\n", GetTime());
	}

	void Free()
	{
		printf("[%f] GL3.1: cleanup\n", GetTime());
		WINCHK(wglMakeCurrent(dc, glrc));
		GLCHK(glDeleteBuffers(1, &vbo));
		GLCHK(glDeleteBuffers(1, &ubo));
		GLCHK(glDeleteVertexArrays(1, &vao));
		GLCHK(glDeleteTextures(1, &cmtex));
		GLCHK(glDetachShader(prog, vs));
		GLCHK(glDetachShader(prog, ps));
		GLCHK(glDeleteProgram(prog));
		GLCHK(glDeleteShader(vs));
		GLCHK(glDeleteShader(ps));
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(glrc);
		ReleaseDC(apiWin, dc);
		DestroyWindow(apiWin);
		printf("[%f] GL3.1: cleanup done\n", GetTime());
	}

	void Render(float mtx[16])
	{
		WINCHK(wglMakeCurrent(dc, glrc));
		GLCHK(glViewport(0, 0, PART_WIDTH, PART_HEIGHT));
		GLCHK(glClearColor(0.4f, 0.2f, 0.1f, 1.0f));
		GLCHK(glClearDepth(1.0f));
		GLCHK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		GLCHK(glUseProgram(prog));
		CBufData bufData;
		bufData.resX = PART_WIDTH;
		bufData.resY = PART_HEIGHT;
		memcpy(bufData.viewMtx, mtx, sizeof(bufData.viewMtx));
		GLCHK(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(bufData), &bufData));
		GLCHK(glDrawArrays(GL_TRIANGLES, 0, 3));

		SwapBuffers(dc);
	}
}


void EmptyMessageQueue()
{
	MSG msg;
	while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

int main()
{
	g_HInstance = GetModuleHandle(nullptr);

	WNDCLASSEXA wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(DKGRAY_BRUSH);
	wc.lpszClassName = "MainWindowClass";
	RegisterClassEx(&wc);

	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = SubWindowProc;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = "SubWindowClass";
	RegisterClassEx(&wc);

	// initialize font
	LOGFONT lf;
	memset(&lf, 0, sizeof(lf));
	lf.lfQuality = CLEARTYPE_QUALITY;
	lf.lfHeight = 16;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	strcpy(lf.lfFaceName, "Tahoma");
	g_Font = CreateFontIndirect(&lf);

	RECT winRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	AdjustWindowRect(&winRect, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE, FALSE);
	g_MainWindow = CreateWindowA("MainWindowClass", "HLSL Optimizing Converter - Four API test",
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, winRect.right - winRect.left, winRect.bottom - winRect.top,
		nullptr, nullptr, g_HInstance, nullptr);

	D3D9::Init();
	EmptyMessageQueue();
	D3D11::Init();
	EmptyMessageQueue();
	GL20::Init();
	EmptyMessageQueue();
	GL31::Init();

	float t = 0;
	double curTime = GetTime();
	MSG msg;
	for(;;)
	{
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
			break;

		double newTime = GetTime();
		float delta = newTime - curTime;
		curTime = newTime;
		if (delta > 1.0f/30.0f)
			delta = 1.0f/30.0f;
		t += delta;
		float mtx[16];
		LookAtMatrixInv(V3(sin(t) * 10, cos(t) * 10, 4), V3(0,0,0), V3(0,0,1), mtx);
		D3D9::Render(mtx);
		D3D11::Render(mtx);
		GL20::Render(mtx);
		GL31::Render(mtx);
	}

	D3D9::Free();
	D3D11::Free();

	DestroyWindow(g_MainWindow);
	return 0;
}

void ExitImminentFreeOpenGL()
{
	// a workaround for wglMakeCurrent
	// https://www.khronos.org/opengl/wiki/Platform_specifics:_Windows#When_do_I_destroy_the_GL_context.3F
	GL20::Free();
	GL31::Free();
}
