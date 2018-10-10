#include <ctime>

#include "lib/imgui.h"
#include "lib/imgui_impl_dx11.h"
#include "lib/imgui_impl_win32.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

#include <string>

#include "common/Textures.h"
#include "common/Loader.h"
#include "common/Shader.h"
#include "common/Buffer.h"
#include "common/HelperFunction.h"
#include "common/type/ConstantBuffer.h"
#include "common/type/Channel.h"
#include "common/type/Resource.h"

#include "ImGuiFileDialog.h"

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 UV;
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
ID3D11RenderTargetView*     g_pBackBufferRenderTargetView = nullptr;

// Depth stencil
ID3D11DepthStencilView*		g_pDepthStencilView = nullptr;
ID3D11DepthStencilState*	g_pDepthStencilState = nullptr;
ID3D11Texture2D*			g_pDepthStencilBuffer = nullptr;

// Blend
ID3D11BlendState*			g_pBlendState = nullptr;

// Shaders
ID3D11VertexShader*         g_pVertexShader = nullptr;
ID3D11PixelShader*          g_pPixelShader = nullptr;

// Resource
Resource					g_Resource[4];

// ShaderToy Image
ID3D11Texture2D*			g_mRenderTargetTexture = nullptr;
ID3D11RenderTargetView*     g_pRenderTargetView = nullptr;
ID3D11ShaderResourceView*   g_pShaderResourceView = nullptr;

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
ImGuiWindowFlags			g_window_flags = 0;
float				        g_vLastT = 0.0f;
ULONGLONG			        g_vPauseT = 0;
int						    g_vFrame = 0;
bool			            g_vPause = false;
ImVec4						g_clear_color = ImVec4(0.08f, 0.12f, 0.14f, 1.00f);
int							g_pressIdentifier = 0;
int							g_padding = 0;
bool					    g_trackMouse = false;



//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
HRESULT InitText();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();
void ReloadShaders();

void InitImGui();
void ImGuiSetup(HINSTANCE);
void ImGuiRender();


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

	InitImGui();

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
			// Render the ShaderToy image
			Render();

			ImGuiSetup(hInstance);

			ImGuiRender();

			// Present the information rendered to the back buffer to the front buffer (the screen)
			g_pSwapChain->Present(1, 0);
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
	RECT rc = { 0, 0, 2560, 1440 };
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
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

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
				// Reload Shaders
				ReloadShaders();
				break;
			case 'P':
			case VK_SPACE:
				// Pause
				g_vPause = !g_vPause;
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
		if ((wParam & MK_LBUTTON) && g_trackMouse)
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

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pBackBufferRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(g_pBackBufferRenderTargetView, "BackBufferRenderTargetView");

	Create2DTexture(g_pd3dDevice, &g_mRenderTargetTexture, &g_pRenderTargetView, &g_pShaderResourceView, width, height);

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the Depth/Stencil View
	g_pd3dDevice->CreateTexture2D(&depthStencilDesc, NULL, &g_pDepthStencilBuffer);
	g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, NULL, &g_pDepthStencilView);
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &g_pDepthStencilState);

	SetDebugObjectName(g_pDepthStencilBuffer, "DepthStencilBuffer");
	SetDebugObjectName(g_pDepthStencilView, "DepthStencilView");
	SetDebugObjectName(g_pDepthStencilState, "DepthStencilState");

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilState, NULL);


	D3D11_BLEND_DESC blendState;
	ZeroMemory(&blendState, sizeof(D3D11_BLEND_DESC));
	blendState.RenderTarget[0].BlendEnable = FALSE;
	blendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = g_pd3dDevice->CreateBlendState(&blendState, &g_pBlendState);

	FLOAT BlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	UINT SampleMask = 0xffffffff;

	g_pImmediateContext->OMSetBlendState(g_pBlendState, BlendFactor, SampleMask);

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

	SetDebugObjectName(g_pVertexShader, "VertexShader");

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

	SetDebugObjectName(g_d3dInputLayout, "InputLayout");

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

	SetDebugObjectName(g_pPixelShader, "PixelShader");

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

	SetDebugObjectName(g_pVertexBuffer, "VertexBuffer");

	InitData.pSysMem = g_Indicies;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
	if (FAILED(hr))
		return hr;

	// Set index buffer
	SetDebugObjectName(g_pIndexBuffer, "IndexBuffer");
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

	InitText();

	SetDebugObjectName(g_pCBNeverChanges, "ConstantBuffer_NeverChanges");
	SetDebugObjectName(g_pCBChangesEveryFrame, "ConstantBuffer_ChangesEveryFrame");

	// Set constant buffers
	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
	cbNC.ChannelResolution[0] = g_Resource[0].channelRes;
	cbNC.ChannelResolution[1] = g_Resource[1].channelRes;
	cbNC.ChannelResolution[2] = g_Resource[2].channelRes;
	cbNC.ChannelResolution[3] = g_Resource[3].channelRes;
	
	g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBNeverChanges);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Initialize ImGui
//--------------------------------------------------------------------------------------
void InitImGui()
{
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	io.FontGlobalScale = 1.2f;

	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);

	// Setup style
	ImGui::StyleColorsDark();
}

//--------------------------------------------------------------------------------------
// Load in textures and initialize the samplers
//--------------------------------------------------------------------------------------
HRESULT InitText()
{
	HRESULT hr = S_OK;
	Channel channels[4];
	int size = 0;

	if(!LoadChannels("channels/channels.txt", channels, size))
		return S_FALSE;

	for (int i = 0; i < size; ++i)
	{
		if (channels[i].type == ChannelType::texture)
		{
			g_Resource[i].type = ChannelType::texture;

			// Load texture
			LoadTexture(g_pd3dDevice, &g_Resource[i].pShaderResource, channels[i].texture, g_Resource[i].channelRes);

			// Texture
			g_pImmediateContext->PSSetShaderResources(i, 1, &g_Resource[i].pShaderResource);
		}
		else if (channels[i].type == ChannelType::buffer)
		{
			RECT rc;
			GetClientRect(g_hWnd, &rc);
			UINT width = rc.right - rc.left;
			UINT height = rc.bottom - rc.top;

			g_Resource[i].type = ChannelType::buffer;

			// Initialize buffer
			g_Resource[i].buffers.InitBuffer(g_pd3dDevice, g_pImmediateContext, g_pVertexShader, width, height, i);

			g_Resource[i].channelRes = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);

			g_Resource[i].buffers.SetShaderResource(g_pImmediateContext, i);
		}

		// Create Sampler
		CreateSampler(g_pd3dDevice, g_pImmediateContext, &g_Resource[i].pSampler, channels[i], TextLoc::main, i);
	}

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	for (int i = 0; i < 4; ++i)
	{
		if (g_Resource[i].buffers.isActive)
		{
			g_Resource[i].buffers.ClearShaderResource(g_pImmediateContext, i);
			g_Resource[i].buffers.Render(g_pImmediateContext, g_pDepthStencilView, g_pCBNeverChanges, ARRAYSIZE(g_Indicies), width, height, i);
			g_Resource[i].buffers.SetShaderResource(g_pImmediateContext, i);
		}
		//else
			//g_pImmediateContext->PSSetShaderResources()
	}

	// Set the shaders
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Get time
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

		if (g_vPause)
			g_vPauseT = timeCur - (ULONGLONG)(g_vLastT * 1000.0f) - timeStart ;

		if (timeStart == 0)
			timeStart = timeCur;
		t = (timeCur - timeStart - g_vPauseT) / 1000.0f;
	}

	// Clear the back buffer 
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);

	//
	// Update variables that change once per frame
	//
	CBChangesEveryFrame cb;
	cb.Frame = g_vFrame++;
	cb.TimeDelta = (float)t;
	if(g_trackMouse)
		cb.Mouse = g_vMouse;
	else
		cb.Mouse = DirectX::XMFLOAT4(g_vMouse.x, g_vMouse.y, 0.0f, 0.0f);
	cb.Time = !g_vPause ? t : g_vLastT;

	if (!g_vPause)
		g_vLastT = t;

	cb.Date.x = (float)(1970 + ltm.tm_year);
	cb.Date.y = (float)(1 + ltm.tm_mon);
	cb.Date.z = (float)(ltm.tm_mday);
	cb.Date.w = (float)(((hour * 60 + min) * 60) + sec);

	g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

	// Buffer that only changes with the channel resolution
	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
	cbNC.ChannelResolution[0] = g_Resource[0].channelRes;
	cbNC.ChannelResolution[1] = g_Resource[1].channelRes;
	cbNC.ChannelResolution[2] = g_Resource[2].channelRes;
	cbNC.ChannelResolution[3] = g_Resource[3].channelRes;

	g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	// Set constant buffers
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBChangesEveryFrame);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBNeverChanges);

	g_pImmediateContext->DrawIndexed(ARRAYSIZE(g_Indicies), 0, 0);
}

//--------------------------------------------------------------------------------------
// Render ImGui
//--------------------------------------------------------------------------------------
void ImGuiRender()
{
	// Rendering
	ImGui::Render();
	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, NULL);
	g_pImmediateContext->ClearRenderTargetView(g_pBackBufferRenderTargetView, (float*)&g_clear_color);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	for (int i = 0; i < 4; ++i)
	{
		if (g_Resource[i].buffers.isActive)
		{
			g_Resource[i].buffers.Release(i);
		}

		if (g_Resource[i].pSampler) g_Resource[i].pSampler->Release();
		if (g_Resource[i].pShaderResource) g_Resource[i].pShaderResource->Release();
	}

	if (g_pCBNeverChanges) g_pCBNeverChanges->Release();
	if (g_pCBChangesEveryFrame) g_pCBChangesEveryFrame->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_d3dInputLayout) g_d3dInputLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();

	if (g_pDepthStencilBuffer) g_pDepthStencilBuffer->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();

	if (g_mRenderTargetTexture) g_mRenderTargetTexture->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pShaderResourceView) g_pShaderResourceView->Release();

	if (g_pBackBufferRenderTargetView) g_pBackBufferRenderTargetView->Release();
	if (g_pDepthStencilState) g_pDepthStencilState->Release();
	if (g_pBlendState) g_pBlendState->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();

	if (g_pd3dDevice) g_pd3dDevice->Release();


	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

//--------------------------------------------------------------------------------------
// Setup ImGui windows
//--------------------------------------------------------------------------------------
void ImGuiSetup(HINSTANCE hInstance)
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGui::IsItemHovered())
		g_trackMouse = false;

	// Constant buffer Data
	{
		ImGui::Begin("Constant Buffer Info258");
		static float resol2[2] = { 1920, 1080 };
		ImGui::DragFloat2("", resol2);

		ImGui::Separator();
		ImGui::Separator();

		ImGui::Text("Never Changes");

		ImGui::Separator();

		ImGui::Text("Resolution:");
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 150.0f);
		ImGui::PushItemWidth(150.0f);
		//ImGui::DragInt2("", resol);
		ImGui::PopItemWidth();

		ImGui::Spacing();
		ImGui::Text("ChanelResolution");

		ImGui::Text("BufferA:");
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 150.0f);
		static int resBufferA[2] = { (int)g_Resource[0].channelRes.x, (int)g_Resource[0].channelRes.y };
		ImGui::PushItemWidth(150.0f);
		static const char * plable = "a";
		ImGui::InputInt2(plable, resBufferA);
		ImGui::PopItemWidth();

/*		ImGui::Text("BufferB:");
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 150.0f);
		static int resBufferB[2] = { (int)g_Resource[1].channelRes.x, (int)g_Resource[1].channelRes.y };
		ImGui::PushItemWidth(150.0f);
		ImGui::InputInt2("", resBufferB);
		ImGui::PopItemWidth();

		ImGui::Text("BufferC:");
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 150.0f);
		static int resBufferC[2] = { (int)g_Resource[2].channelRes.x, (int)g_Resource[2].channelRes.y };
		ImGui::PushItemWidth(150.0f);
		ImGui::InputInt2("", resBufferC);
		ImGui::PopItemWidth();

		ImGui::Text("BufferD:");
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 150.0f);
		static int resBufferD[2] = { (int)g_Resource[3].channelRes.x, (int)g_Resource[3].channelRes.y };
		ImGui::PushItemWidth(150.0f);
		ImGui::InputInt2("", resBufferD);
		ImGui::PopItemWidth();

		ImGui::Separator();
		ImGui::Separator();
*/
		ImGui::End();
	}

	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
		ImGui::ColorEdit3("clear color", (float*)&g_clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		static float resol[2] = { 1920, 1080 };
		ImGui::DragFloat2("", resol);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// Box for ShaderToy image to be displayed in
	{
		ImGui::Begin("ShaderToy", 0, g_window_flags);

		g_window_flags = ImGuiWindowFlags_NoMove;

		g_trackMouse = false;

		if (ImGui::IsWindowHovered())
			g_trackMouse = true;

		if (ImGui::IsItemHovered())
		{
			g_window_flags = 0;
			g_trackMouse = false;
		}

		ImVec2 windowSize = ImGui::GetWindowSize();

		switch (g_padding)
		{
		case 0:
			ImGui::Image(g_pShaderResourceView, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			break;
		case 1:
			if (g_Resource[0].buffers.isActive)
				ImGui::Image(g_Resource[0].buffers.mShaderResourceView, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			else
				ImGui::Image(g_Resource[0].pShaderResource, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			break;
		case 2:
			if (g_Resource[1].buffers.isActive)
				ImGui::Image(g_Resource[1].buffers.mShaderResourceView, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			else
				ImGui::Image(g_Resource[1].pShaderResource, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			break;
		case 3:
			if (g_Resource[2].buffers.isActive)
				ImGui::Image(g_Resource[2].buffers.mShaderResourceView, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			else
				ImGui::Image(g_Resource[2].pShaderResource, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			break;
		case 4:
			if (g_Resource[3].buffers.isActive)
				ImGui::Image(g_Resource[3].buffers.mShaderResourceView, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			else
				ImGui::Image(g_Resource[3].pShaderResource, ImVec2(windowSize.x - 17, windowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
			break;
		}

		ImGui::End();
	}

	// Toggle view
	{
		ImGui::Begin("View Toggle");
		
		const float ItemSpacing = ImGui::GetStyle().ItemSpacing.x;
		ImVec2 windowSize = ImGui::GetWindowSize();

		float bufferDButtonWidth = (windowSize.x) / 5.0f - 10.0f;
		float pos = bufferDButtonWidth + ItemSpacing;
		ImGui::SameLine(windowSize.x - pos);
		if (ImGui::Button("Buffer D", ImVec2(bufferDButtonWidth, windowSize.y - 42.0f)))
			g_padding = 4;
		bufferDButtonWidth = ImGui::GetItemRectSize().x;

		float bufferCButtonWidth = (windowSize.x) / 5.0f - 10.0f;
		pos += bufferCButtonWidth + ItemSpacing;
		ImGui::SameLine(windowSize.x - pos);
		if (ImGui::Button("Buffer C", ImVec2(bufferCButtonWidth, windowSize.y - 42.0f)))
			g_padding = 3;
		bufferCButtonWidth = ImGui::GetItemRectSize().x;

		float bufferBButtonWidth = (windowSize.x) / 5.0f - 10.0f;
		pos += bufferBButtonWidth + ItemSpacing;
		ImGui::SameLine(windowSize.x - pos);
		if (ImGui::Button("Buffer B", ImVec2(bufferBButtonWidth, windowSize.y - 42.0f)))
			g_padding = 2;
		bufferBButtonWidth = ImGui::GetItemRectSize().x;

		float bufferAButtonWidth = (windowSize.x) / 5.0f - 10.0f;
		pos += bufferAButtonWidth + ItemSpacing;
		ImGui::SameLine(windowSize.x - pos);
		if (ImGui::Button("Buffer A", ImVec2(bufferAButtonWidth, windowSize.y - 42.0f)))
			g_padding = 1;
		bufferAButtonWidth = ImGui::GetItemRectSize().x;

		float mainButtonWidth = (windowSize.x) / 5.0f - 10.0f;
		pos += mainButtonWidth + ItemSpacing;
		ImGui::SameLine(windowSize.x - pos);
		if (ImGui::Button("Main", ImVec2(mainButtonWidth, windowSize.y - 42.0f)))
			g_padding = 0;
		mainButtonWidth = ImGui::GetItemRectSize().x;

		ImGui::End();
	}

	// Box for the resources
	{
		ImGui::Begin("Resources");

		ImVec2 windowSize = ImGui::GetWindowSize();

		ImVec2 texButtonSize = ImVec2(125.0f, 125.0f);

		if (g_padding == 0)
		{
			// Main Image

			if (g_Resource[0].buffers.isActive)
			{
				if (ImGui::ImageButton(g_Resource[0].buffers.mShaderResourceView, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
				{
					//g_pressIdentifier = 1;
				}
			}
			else
			{
				if (ImGui::ImageButton(g_Resource[0].pShaderResource, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
				{
					g_pressIdentifier = 1;
				}
			}

			if (g_Resource[1].buffers.isActive)
			{
				if (ImGui::ImageButton(g_Resource[1].buffers.mShaderResourceView, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
				{
					//g_pressIdentifier = 2;
				}
			}
			else
			{
				if (ImGui::ImageButton(g_Resource[1].pShaderResource, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
				{
					g_pressIdentifier = 2;
				}
			}

			if (g_Resource[2].buffers.isActive)
			{
				if (ImGui::ImageButton(g_Resource[2].buffers.mShaderResourceView, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
				{
					//g_pressIdentifier = 3;
				}
			}
			else
			{
				if (ImGui::ImageButton(g_Resource[2].pShaderResource, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
				{
					g_pressIdentifier = 3;
				}
			}

			if (g_Resource[3].buffers.isActive)
			{
				if (ImGui::ImageButton(g_Resource[3].buffers.mShaderResourceView, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
				{
					//g_pressIdentifier = 4;
				}
			}
			else
			{
				if (ImGui::ImageButton(g_Resource[3].pShaderResource, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
				{
					g_pressIdentifier = 4;
				}
			}
		}
		else
		{
			// Buffers

			if (ImGui::ImageButton(g_Resource[g_padding - 1].buffers.mpTextureRV[0], ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
			{
				g_pressIdentifier = 1;
			}
			
			if (ImGui::ImageButton(g_Resource[g_padding - 1].buffers.mpTextureRV[1], ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
			{
				g_pressIdentifier = 2;
			}

			if (ImGui::ImageButton(g_Resource[g_padding - 1].buffers.mpTextureRV[2], ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
			{
				g_pressIdentifier = 3;
			}

			if (ImGui::ImageButton(g_Resource[g_padding - 1].buffers.mpTextureRV[3], ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f)))
			{
				g_pressIdentifier = 4;
			}
		}

		// Resource picker
		if (g_pressIdentifier)
		{
			if (ImGuiFileDialog::Instance(g_pd3dDevice)->FileDialog("Choose File", ".dds\0", ".", ""))
			{
				std::string filePathName = "";
				std::string path = "";
				std::string fileName = "";
				std::string filter = "";

				if (ImGuiFileDialog::Instance(g_pd3dDevice)->IsOk == true)
				{
					filePathName = ImGuiFileDialog::Instance(g_pd3dDevice)->GetFilepathName();
					path = ImGuiFileDialog::Instance(g_pd3dDevice)->GetCurrentPath();
					fileName = ImGuiFileDialog::Instance(g_pd3dDevice)->GetCurrentFileName();
					filter = ImGuiFileDialog::Instance(g_pd3dDevice)->GetCurrentFilter();
				}
				else
				{
					filePathName = "";
					path = "";
					fileName = "";
					filter = "";
				}

				Channel channel = Channel(std::string("textures/" + fileName).c_str());


				if (g_padding == 0)
				{
					// Main image

					// Load texture
					LoadTexture(g_pd3dDevice, &g_Resource[(g_pressIdentifier - 1) + g_padding * 4].pShaderResource, channel.texture, g_Resource[(g_pressIdentifier - 1) + g_padding * 4].channelRes);

					// Texture
					g_pImmediateContext->PSSetShaderResources((g_pressIdentifier - 1) + g_padding * 4, 1, &g_Resource[(g_pressIdentifier - 1) + g_padding * 4].pShaderResource);
				}
				else
				{
					// Buffers

					// Load texture
					LoadTexture(g_pd3dDevice, &g_Resource[g_padding - 1].buffers.mpTextureRV[g_pressIdentifier - 1], channel.texture, g_Resource[(g_padding - 1)].buffers.mvChannelRes[g_pressIdentifier - 1]);

					// Texture
					g_pImmediateContext->PSSetShaderResources((g_pressIdentifier - 1) + g_padding * 4, 1, &g_Resource[g_padding - 1].buffers.mpTextureRV[g_pressIdentifier - 1]);
				}

				g_pressIdentifier = 0;
				ImGuiFileDialog::Instance(g_pd3dDevice)->Clear();
			}
		}

		ImGui::End();
	}
}

//--------------------------------------------------------------------------------------
// Reload the Shaders
//--------------------------------------------------------------------------------------
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

	// Set the shaders
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
}