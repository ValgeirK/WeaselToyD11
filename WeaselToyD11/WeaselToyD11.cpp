#include <stdlib.h>
#include <stddef.h>
#include <ctime>

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "common/DDSTextureLoader.h"


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 UV;
};

struct CBNeverChanges
{
	DirectX::XMFLOAT4 Resolution;
};

struct CBChangesEveryFrame
{
	DirectX::XMFLOAT4 Mouse;
	DirectX::XMFLOAT4 Date;
	float             Time;
	float             TimeDelta;
	int			      Frame;
	int			      Padding;
};


//--------------------------------------------------------------------------------------
// Raw Data
//--------------------------------------------------------------------------------------
SimpleVertex g_Vertices[4] =
{
	{ DirectX::XMFLOAT3(-1.0f,  1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },
	{ DirectX::XMFLOAT3( 1.0f, -1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
	{ DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
	{ DirectX::XMFLOAT3( 1.0f,  1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }
};

WORD g_Indicies[6] =
{
	0, 1, 2,
	0, 3, 1
};

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                   g_hInst = nullptr;
HWND                        g_hWnd = nullptr;
D3D_DRIVER_TYPE             g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL           g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*               g_pd3dDevice = nullptr;
ID3D11DeviceContext*        g_pImmediateContext = nullptr;
IDXGISwapChain*             g_pSwapChain = nullptr;
ID3D11RenderTargetView*     g_pRenderTargetView = nullptr;

// Shaders
ID3D11VertexShader*         g_pVertexShader = nullptr;
ID3D11PixelShader*          g_pPixelShader = nullptr;

// Sampler
ID3D11SamplerState*         g_pSamplerLinear = nullptr;

// Texture
ID3D11ShaderResourceView*   g_pTextureRV = nullptr;

// Vertex buffer data
ID3D11InputLayout*		    g_d3dInputLayout = nullptr;
ID3D11Buffer*			    g_pVertexBuffer = nullptr;
ID3D11Buffer*			    g_pIndexBuffer = nullptr;

// Constant buffers
ID3D11Buffer*               g_pCBNeverChanges = nullptr;
ID3D11Buffer*               g_pCBChangesEveryFrame = nullptr;

// Demo
DirectX::XMFLOAT4           g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);
DirectX::XMFLOAT4			g_vMouse(0.0f, 0.0f, 0.0f, 0.0f);
int						    g_vFrame = 0;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();
void ReloadShaders();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = (LPCSTR)"TutorialWindowClass";
	wcex.hIconSm = nullptr;
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 800, 600 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow((LPCSTR)"TutorialWindowClass", (LPCSTR)"WeaselToyD11",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		// Note that this tutorial does not handle resizing (WM_SIZE) requests,
		// so we created the window without the resize border.

	case WM_KEYDOWN:
		switch (wParam)
		{
			case 'R':
			case VK_F5:
				ReloadShaders();
				break;
			case VK_ESCAPE:
				DestroyWindow(hWnd);
				break;
		}
		break;
	case WM_LBUTTONDOWN:
	{
		// Capture mouse input
		SetCapture(hWnd);

		g_vMouse.z = 1.0f;

		// Retrieve the screen coordinates of the client area, 
        // and convert them into client coordinates.
		RECT rcClient;
		POINT ptClientUL;
		POINT ptClientLR;
		GetClientRect(hWnd, &rcClient);

		ptClientUL.x = rcClient.left;
		ptClientUL.y = rcClient.top;

		// Add one to the right and bottom sides, because the 
		// coordinates retrieved by GetClientRect do not 
		// include the far left and lowermost pixels. 
		ptClientLR.x = rcClient.right + 1;
		ptClientLR.y = rcClient.bottom + 1;

		ClientToScreen(hWnd, &ptClientUL);
		ClientToScreen(hWnd, &ptClientLR);

		// Copy the client coordinates of the client area 
		// to the rcClient structure. Confine the mouse cursor 
		// to the client area by passing the rcClient structure 
		// to the ClipCursor function. 
		SetRect(&rcClient, ptClientUL.x, ptClientUL.y,
			ptClientLR.x, ptClientLR.y);
		ClipCursor(&rcClient);

		break;
	}
	case WM_RBUTTONDOWN:
	{
		g_vMouse.w = 1.0f;
		break;
	}
	case WM_MOUSEMOVE:
	{
		if (wParam & MK_LBUTTON)
		{
			POINTS pts = MAKEPOINTS(lParam);
			g_vMouse.x = pts.x;
			g_vMouse.y = pts.y;
		}
		break;
	}
	case WM_LBUTTONUP:
	{
		// Release mouse
		ReleaseCapture();

		ClipCursor(NULL);

		g_vMouse.z = 0.0f;

		break;
	}
	case WM_RBUTTONUP:
	{
		g_vMouse.w = 0.0f;
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;

	hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	IDXGIFactory1* dxgiFactory = nullptr;
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}
	}
	if (FAILED(hr))
		return hr;

	// Create swap chain
	// DirectX 11.0 systems
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);

	// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
	dxgiFactory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER);

	dxgiFactory->Release();

	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"shaders/VertexShader.hlsl", "main", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error with the vertex shader.", (LPCSTR)"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	if (FAILED(hr))
	{
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(SimpleVertex,Pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV",   0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SimpleVertex,UV), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_d3dInputLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_d3dInputLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shaders/PixelShader.hlsl", "main", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * ARRAYSIZE(g_Vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = g_Vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * ARRAYSIZE(g_Indicies);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	InitData.pSysMem = g_Indicies;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
	if (FAILED(hr))
		return hr;

	// Set index buffer
	g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

	// Set primitive topology
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffers
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(CBNeverChanges);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBNeverChanges);
	if (FAILED(hr))
		return hr;
	
	bd.ByteWidth = sizeof(CBChangesEveryFrame);
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangesEveryFrame);
	if (FAILED(hr))
		return hr;

	// Load Texture
	   // Load the Texture
	hr = DirectX::CreateDDSTextureFromFile(g_pd3dDevice, L"seafloor.dds", nullptr, &g_pTextureRV);
	if (FAILED(hr))
		return hr;

	// Create the sample state
	D3D11_SAMPLER_DESC samplDesc = {};
	samplDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplDesc.MinLOD = 0;
	samplDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&samplDesc, &g_pSamplerLinear);
	if (FAILED(hr))
		return hr;

	// Set constant buffers
	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4(width, height, 1.0f, 1.0f);
	g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
	// Get time
	static time_t first = time(0);
	time_t now = time(0);
	struct tm ltm;
	localtime_s(&ltm, &now);
	int hour = 1 + ltm.tm_hour;
	int min = 1 + ltm.tm_min;
	int sec = 1 + ltm.tm_sec;

	// Update our time
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)DirectX::XM_PI * 0.0125f;
	}
	else
	{
		static ULONGLONG timeStart = 0;
		ULONGLONG timeCur = GetTickCount64();
		if (timeStart == 0)
			timeStart = timeCur;
		t = (timeCur - timeStart) / 1000.0f;
	}

	// Clear the back buffer 
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);

	//
	// Update variables that change once per frame
	//
	CBChangesEveryFrame cb;
	cb.Frame = g_vFrame++;
	cb.TimeDelta = (float)t;
	cb.Mouse = g_vMouse;
	cb.Time = now - first;

	cb.Date.x = (float)(1970 + ltm.tm_year);
	cb.Date.y = (float)(1 + ltm.tm_mon);
	cb.Date.z = (float)(ltm.tm_mday);
	cb.Date.w = (float)(((hour * 60 + min) * 60) + sec);

	g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

	// Render a triangle
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

	// Set constant buffers
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBNeverChanges);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBNeverChanges);

	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBChangesEveryFrame);

	// Texture
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);

	// Sampler
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

	g_pImmediateContext->DrawIndexed(ARRAYSIZE(g_Indicies), 0, 0);

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present(0, 0);
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	if (g_pTextureRV) g_pTextureRV->Release();
	if (g_pCBNeverChanges) g_pCBNeverChanges->Release();
	if (g_pCBChangesEveryFrame) g_pCBChangesEveryFrame->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_d3dInputLayout) g_d3dInputLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}

void ReloadShaders()
{
	HRESULT hr = S_OK;

	// Compile the vertex shader
	ID3DBlob* pVSBlob = nullptr;
	hr = CompileShaderFromFile(L"shaders/VertexShader.hlsl", "main", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error with the vertex shader.", (LPCSTR)"Error", MB_OK);
		return;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
	pVSBlob->Release();
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error creating vertex shader.", (LPCSTR)"Error", MB_OK);
		return;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shaders/PixelShader.hlsl", "main", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
		return;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error creating pixel shader.", (LPCSTR)"Error", MB_OK);
		return;
	}
}