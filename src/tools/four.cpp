
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

#include "../compiler.hpp"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 960
#define PART_WIDTH 640
#define PART_HEIGHT 480

HINSTANCE g_HInstance = nullptr;
HWND g_MainWindow = nullptr;
HFONT g_Font = nullptr;

LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(window, msg, wp, lp);
}

LRESULT CALLBACK SubWindowProc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
	PAINTSTRUCT ps;
	HDC hdc;
	char text[32];

	switch (msg)
	{
	case WM_PAINT:
		hdc = BeginPaint(window, &ps);
		SelectObject(hdc, g_Font);
		SetBkMode(hdc, TRANSPARENT);
		SetBkColor(hdc, RGB(0,0,0));
		SetTextColor(hdc, RGB(200,200,200));
		TextOut(hdc, 0, 0, text, GetWindowText(window, text, 32));
		EndPaint(window, &ps);
		return 0L;
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
	HWND apiWin = nullptr;
	IDirect3D9* d3d = nullptr;
	IDirect3DDevice9* dev = nullptr;
	IDirect3DVertexShader9* vs = nullptr;
	IDirect3DPixelShader9* ps = nullptr;

	void CompileShader(bool pixelShader)
	{
		Compiler compiler;
		FILEStream errStream(stderr);
		StringStream codeStream;
		compiler.errorOutputStream = &errStream;
		compiler.codeOutputStream = &codeStream;
		compiler.outputFmt = OSF_HLSL_SM3;
		compiler.stage = ShaderStage_Vertex;
		ShaderMacro macros[] = { { pixelShader ? "PS" : "VS", "1" }, { "D3D9", "1" }, { nullptr, nullptr } };
		compiler.defines = macros;
		std::string inCode = GetFileContents("runtests/html5-shader.hlsl", true);
		if (!compiler.CompileFile("runtests/html5-shader.hlsl", inCode.c_str()))
		{
			fprintf(stderr, "compilation failed, no output generated\n");
			exit(1);
		}
		ID3DXBuffer* codeBuf = nullptr;
		ID3DXBuffer* errBuf = nullptr;
	//	puts(codeStream.str().c_str());
		D3DXCompileShader(codeStream.str().c_str(), codeStream.str().size(), nullptr, nullptr,
	//	D3DXMACRO xmacros[] = { { pixelShader ? "PS" : "VS", "1" }, { "D3D9", "1" }, { nullptr, nullptr } };
	//	D3DXCompileShader(inCode.c_str(), inCode.size(), xmacros, nullptr,
			"main", pixelShader ? "ps_3_0" : "vs_3_0", D3DXSHADER_SKIPOPTIMIZATION, &codeBuf, &errBuf, nullptr);
		if (errBuf)
		{
			fprintf(stderr, "D3DXCompileShader failed, errors:\n%.*s\n",
				int(errBuf->GetBufferSize()), (const char*) errBuf->GetBufferPointer());
			errBuf->Release();
		}
		if (!codeBuf)
		{
			fprintf(stderr, "D3DXCompileShader failed, no output generated\n");
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
		apiWin = CreateWindowA("SubWindowClass", "Direct3D 9", WS_CHILD | WS_VISIBLE,
			0, 0, PART_WIDTH, PART_HEIGHT, g_MainWindow, nullptr, g_HInstance, nullptr);

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

		CompileShader(false);
		CompileShader(true);
	}

	void Free()
	{
		ps->Release();
		vs->Release();
		dev->Release();
		d3d->Release();
		DestroyWindow(apiWin);
	}

	void Render()
	{
		dev->SetRenderState(D3DRS_LIGHTING, FALSE);
		dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		dev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(10, 20, 140), 1.0f, 0);
		dev->BeginScene();
		dev->SetVertexShader(vs);
		dev->SetPixelShader(ps);
		float reso[4] = { PART_WIDTH, PART_HEIGHT, 0, 0 };
		float mtx[16] = {
		//	1, 0, 0, -10,
		//	0, 1, 0, -10,
		//	0, 0, 1, 10,
		//	0, 0, 0, 1,
			-0.7991666718346001, 0.22324646365622025, -0.5581161591405507, -6.011094996993548, 0.6011094996993547, 0.2968030507723102, -0.7420076269307755, -7.991666718346001, 0, 0.9284766908852593, 0.3713906763541037, 4, 0, 0, 0, 1
		};
	//	for(int i = 0; i < 3; ++i) for(int j = i; j < 3; ++j){
	//		float t = mtx[i*4+j]; mtx[i*4+j] = mtx[j*4+i]; mtx[j*4+i] = t; }
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

	g_MainWindow = CreateWindowA("MainWindowClass", "Four APIs", WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, g_HInstance, nullptr);

	D3D9::Init();

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

		D3D9::Render();
	}

	D3D9::Free();

	DestroyWindow(g_MainWindow);
	return 0;
}
