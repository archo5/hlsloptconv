
#include <windows.h>
#include <d3d9.h>
#if USE_D3DXCOMPILESHADER
#  include <d3dx9.h>
#  pragma comment(lib, "d3dx9.lib")
#endif
#include <d3d11.h>
#include <d3dcompiler.h>

#include "../compiler.hpp"

#define WINDOW_WIDTH (640+640)
#define WINDOW_HEIGHT (16+360+16+360)
#define PART_WIDTH 640
#define PART_HEIGHT 360

const char* SHADER_NAME = "runtests/html5-shader.hlsl";

HINSTANCE g_HInstance = nullptr;
HWND g_MainWindow = nullptr;
HFONT g_Font = nullptr;


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


LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(window, &ps);
		SelectObject(hdc, g_Font);
		SetBkMode(hdc, TRANSPARENT);
		SetBkColor(hdc, RGB(0,0,0));
		SetTextColor(hdc, RGB(200,200,200));
		TextOut(hdc, 0, 0, STRLIT_SIZE("Direct3D 9"));
		TextOut(hdc, PART_WIDTH, 0, STRLIT_SIZE("Direct3D 11"));
		EndPaint(window, &ps);
		return 0L;
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


namespace D3D9
{
	HWND                    apiWin = nullptr;
	IDirect3D9*             d3d    = nullptr;
	IDirect3DDevice9*       dev    = nullptr;
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
		std::string inCode = GetFileContents(SHADER_NAME, true);
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

		printf("[%f] D3D9: compiling shaders\n", GetTime());
		CompileShader(false);
		CompileShader(true);
		printf("[%f] D3D9: init done\n", GetTime());
	}

	void Free()
	{
		printf("[%f] D3D9: cleanup\n", GetTime());
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
		dev->SetFVF(D3DFVF_XYZ);
		float verts[] =
		{
			-1, -1, 0.5f,
			1, -1, 0.5f,
			1, 1, 0.5f,
			-1, 1, 0.5f,
		};
		dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(float) * 3);
		dev->EndScene();
		dev->Present(nullptr, nullptr, nullptr, nullptr);
	}
}

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
		std::string inCode = GetFileContents(SHADER_NAME, true);
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

		printf("[%f] D3D11: compiling shaders\n", GetTime());
		CompileShader(false);
		CompileShader(true);
		printf("[%f] D3D11: init done\n", GetTime());
	}

	void Free()
	{
		printf("[%f] D3D11: cleanup\n", GetTime());
		ps->Release();
		vs->Release();
		backBufRTV->Release();
		swapChain->Release();
		ctx->Release();
		dev->Release();
		printf("[%f] D3D11: cleanup done\n", GetTime());
	}

	void Render(float mtx[16])
	{
		float col[4] = { 10/255.0f, 20/255.0f, 180/255.0f, 1.0f };
		ctx->ClearRenderTargetView(backBufRTV, col);

		D3D11_VIEWPORT vp;
		memset(&vp, 0, sizeof(vp));
		vp.Width = PART_WIDTH;
		vp.Height = PART_HEIGHT;
		vp.MaxDepth = 1;
		ctx->RSSetViewports(1, &vp);
		ctx->VSSetShader(vs, nullptr, 0);
		ctx->PSSetShader(ps, nullptr, 0);
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
	g_MainWindow = CreateWindowA("MainWindowClass", "Four APIs",
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, winRect.right - winRect.left, winRect.bottom - winRect.top,
		nullptr, nullptr, g_HInstance, nullptr);

	D3D9::Init();
	D3D11::Init();

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
	}

	D3D9::Free();
	D3D11::Free();

	DestroyWindow(g_MainWindow);
	return 0;
}
