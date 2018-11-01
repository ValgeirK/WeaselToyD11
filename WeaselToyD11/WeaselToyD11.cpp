#include <ctime>

#include "lib/imgui.h"
#include "lib/imgui_impl_dx11.h"
#include "lib/imgui_impl_win32.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
//#include <pix3.h>
#include <string.h>
#include <vector>

#include "common/Textures.h"
#include "common/Loader.h"
#include "common/Shader.h"
#include "common/Buffer.h"
#include "common/HelperFunction.h"
#include "common/TextureLib.h"
#include "common/ImGuiWindows.h"
#include "common/type/ConstantBuffer.h"
#include "common/type/Channel.h"
#include "common/type/Resource.h"


//--------------------------------------------------------------------------------------
// Defines
//--------------------------------------------------------------------------------------
#define D3D_VERIFY(func)					\
	HRESULT hr = func;						\
	assert(hr == S_OK);						


#define SAFE_RELEASE(ptr)					\
	if (ptr)								\
	{										\
		UINT RefCount = ptr->Release();		\
		assert(RefCount == 0);				\
		ptr = nullptr;						\
	}

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

// Buffers
Buffer						g_Buffers[4];

// ShaderToy Image
ID3D11Texture2D*			g_pRenderTargetTexture = nullptr;
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
TextureLib*					g_pTexturelib;
DirectX::XMFLOAT4           g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);
DirectX::XMFLOAT4			g_vMouse(0.0f, 0.0f, 0.0f, 0.0f);
ImVec2						g_vCurrentWindowSize;
ImVec2						g_vWindowSize;
ImGuiWindowFlags			g_windowFlags;
float				        g_fLastT = 0.0f;
float						g_fDeltaT = 0.0f;
float						g_fGameT = 0.0f;
float						g_fPauseT = 0;
int						    g_iFrame = 0;
bool			            g_bPause = false;
ImVec4						g_vClearColor = ImVec4(0.08f, 0.12f, 0.14f, 1.00f);
int							g_iPressIdentifier = 0;
int							g_iPadding = 0;
bool					    g_bTrackMouse = false;
float						g_fPlaySpeed = 1.0f;
std::vector<std::string>	g_vShaderErrorList;
ImGuiEnum::DefaultEditor	g_eDefaultEditor = ImGuiEnum::DefaultEditor::E_NOTEPADPP;



//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT				InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT				InitDevice();
HRESULT				InitTextures();
void				CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void				Render();
void				ReloadShaders();

void				InitImGui();
void				ImGuiSetup(HINSTANCE);
void				ImGuiRender();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
#ifdef DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // DEBUG

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	//MessageBox(nullptr,
	//	(LPCSTR)"Start Pix.", (LPCSTR)"Error", MB_OK);

	//PIXBeginEvent(0, "Initializing Device");
	{
		if (FAILED(InitDevice()))
		{
			CleanupDevice();
			return 0;
		}
	}
	//PIXEndEvent(); // Initializing Device

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
			byte col = 0;

			//PIXBeginEvent(PIX_COLOR_INDEX(col++), "MainLoop");
			{
				// Render the ShaderToy image
				//PIXBeginEvent(PIX_COLOR_INDEX(col++), "Render");
				{
					Render();
				}
				//PIXEndEvent(); // RENDER

				//PIXBeginEvent(PIX_COLOR_INDEX(col++), "ImGuiSetup");
				{
					ImGuiSetup(hInstance);
				}
				//PIXEndEvent(); // IMGUISETUP


				//PIXBeginEvent(PIX_COLOR_INDEX(col++), "ImGuiRender");
				{
					ImGuiRender();
				}
				//PIXEndEvent();

				//PIXBeginEvent(PIX_COLOR_INDEX(col++), "Present");
				{
					D3D_VERIFY(g_pSwapChain->Present(1, 0));
				}
				//PIXEndEvent(); // PRESENT
			}
			//PIXEndEvent(); // MAIN LOOP
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
	
	// Get desktop resolution
	const HWND hDesktop = GetDesktopWindow();
	RECT desktop;
	GetWindowRect(hDesktop, &desktop);

	RECT rc = { 0, 0, desktop.right * 2 / 3, desktop.bottom * 2 / 3 };
	g_vWindowSize = ImVec2((float)(desktop.right * 2 / 3), (float)(desktop.bottom * 2 / 3));
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
				g_bPause = !g_bPause;
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
		if ((wParam & MK_LBUTTON) && g_bTrackMouse)
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

	Create2DTexture(g_pd3dDevice, &g_pRenderTargetTexture, &g_pRenderTargetView, &g_pShaderResourceView, width, height);

	CreateDepthStencilView(g_pd3dDevice, &g_pDepthStencilView, &g_pDepthStencilState, &g_pDepthStencilBuffer, width, height);

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
	hr = CompileShaderFromFile(L"shaders/VertexShader.hlsl", "main", "vs_4_0", &pVSBlob, g_vShaderErrorList);
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
	hr = CompileShaderFromFile(L"shaders/PixelShader.hlsl", "main", "ps_4_0", &pPSBlob, g_vShaderErrorList);
	if (FAILED(hr))
	{
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			std::string msg = "Error Pixel Shader \"shaders/PixelShader.hlsl\" not found!";

			MessageBox(nullptr,
				(LPCSTR)msg.c_str(), (LPCSTR)"Error", MB_OK);
		}

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

	
	InitTextures();

	SetDebugObjectName(g_pCBNeverChanges, "ConstantBuffer_NeverChanges");
	SetDebugObjectName(g_pCBChangesEveryFrame, "ConstantBuffer_ChangesEveryFrame");

	// Set constant buffers
	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
	cbNC.ChannelResolution[0] = g_Resource[0].m_vChannelRes;
	cbNC.ChannelResolution[1] = g_Resource[1].m_vChannelRes;
	cbNC.ChannelResolution[2] = g_Resource[2].m_vChannelRes;
	cbNC.ChannelResolution[3] = g_Resource[3].m_vChannelRes;
	
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

	io.FontGlobalScale = 1.2f;

	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);

	// Setup style
	ImGui::StyleColorsDark();
}

//--------------------------------------------------------------------------------------
// Load in textures and initialize the samplers
//--------------------------------------------------------------------------------------
HRESULT InitTextures()
{
	HRESULT hr = S_OK;
	Channel channels[4];
	int size = 0;

	for (int i = 0; i < 4; ++i)
	{
		// Initializing all the buffers but they have the active flag set to false
		g_Buffers[i] = Buffer();
	}

	// Populate the texture library
	g_pTexturelib = new TextureLib();
	g_pTexturelib->ParallelLoadDDSTextures(g_pd3dDevice, "textures/*");

	if(!LoadChannels("channels/channels.txt", channels, size))
		return S_FALSE;

	for (int i = 0; i < size; ++i)
	{
		if (channels[i].m_Type == Channels::ChannelType::E_Texture)
		{
			g_Resource[i].m_Type = Channels::ChannelType::E_Texture;

			strcpy(g_Resource[i].m_strTexture,channels[i].m_strTexture);

			// Get texture from texture lib
			g_pTexturelib->GetTexture(channels[i].m_strTexture, &g_Resource[i].m_pShaderResource, g_Resource[i].m_vChannelRes);
		}
		else if (channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			RECT rc;
			GetClientRect(g_hWnd, &rc);
			UINT width = rc.right - rc.left;
			UINT height = rc.bottom - rc.top;

			g_Resource[i].m_Type = Channels::ChannelType::E_Buffer;

			// Initialize buffer
			g_Buffers[static_cast<int>(channels[i].m_BufferId)].InitBuffer(g_pd3dDevice, g_pImmediateContext, g_pVertexShader, g_pTexturelib, g_Buffers, width, height, channels[i].m_BufferId);

			g_Resource[i].m_vChannelRes = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
			g_Resource[i].m_iBufferIndex = static_cast<int>(channels[i].m_BufferId);

			g_Buffers[static_cast<int>(channels[i].m_BufferId)].SetShaderResource(g_pImmediateContext, i);
		}

		// Create Sampler
		CreateSampler(g_pd3dDevice, g_pImmediateContext, &g_Resource[i].m_pSampler, channels[i], -1, i);
	}

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
	if (g_pTexturelib->m_bReload)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (g_Resource[i].m_Type == Channels::ChannelType::E_Texture)
			{
				// Get texture from texture lib
				g_pTexturelib->GetTexture(g_Resource[i].m_strTexture, &g_Resource[i].m_pShaderResource, g_Resource[i].m_vChannelRes);

				// Texture
				g_pImmediateContext->PSSetShaderResources(i, 1, &g_Resource[i].m_pShaderResource);
			}
		}

		g_pTexturelib->m_bReload = false;
	}

	for (int i = 0; i < 4; ++i)
	{
		// Update buffers
		if (g_Buffers[i].m_bIsActive)
		{
			// Update the buffer frames
			g_Buffers[i].Render(g_pImmediateContext, g_pDepthStencilView, g_pCBNeverChanges, ARRAYSIZE(g_Indicies), (UINT)g_vCurrentWindowSize.x, (UINT)g_vCurrentWindowSize.y, i);
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		// Bind the correct textures and buffer resources
		if (g_Resource[i].m_Type == Channels::ChannelType::E_Texture)
			g_pImmediateContext->PSSetShaderResources(i, 1, &g_Resource[i].m_pShaderResource);
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

		if (g_bPause)
			g_fPauseT = (float)(timeCur - (ULONGLONG)(g_fLastT * 1000.0f) - timeStart);

		if (timeStart == 0)
			timeStart = timeCur;
		t = (timeCur - timeStart - g_fPauseT) / 1000.0f;
	}

	// Clear the back buffer 
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);

	//
	// Update variables that change once per frame
	//
	CBChangesEveryFrame cb;
	cb.m_iFrame = g_iFrame++;
	if(g_bTrackMouse)
		cb.m_vMouse = g_vMouse;
	else
		cb.m_vMouse = DirectX::XMFLOAT4(g_vMouse.x, g_vMouse.y, 0.0f, 0.0f);

	g_fDeltaT = !g_bPause ? t - g_fLastT : 0.0f;

	if (!g_bPause)
		g_fLastT = t;

	g_fDeltaT *= g_fPlaySpeed;

	g_fGameT += !g_bPause ? g_fDeltaT : 0.0f;
	cb.m_fTimeDelta = g_fDeltaT;
	cb.m_fTime = g_fGameT;

	cb.m_vDate.x = (float)(1900 + ltm.tm_year);
	cb.m_vDate.y = (float)(1 + ltm.tm_mon);
	cb.m_vDate.z = (float)(ltm.tm_mday);
	cb.m_vDate.w = (float)(((hour * 60 + min) * 60) + sec);

	g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

	// Buffer that only changes with the channel resolution
	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4((float)g_vCurrentWindowSize.x, (float)g_vCurrentWindowSize.y, 0.0f, 0.0f);
	cbNC.ChannelResolution[0] = g_Resource[0].m_vChannelRes;
	cbNC.ChannelResolution[1] = g_Resource[1].m_vChannelRes;
	cbNC.ChannelResolution[2] = g_Resource[2].m_vChannelRes;
	cbNC.ChannelResolution[3] = g_Resource[3].m_vChannelRes;

	g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	// Set constant buffers
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBChangesEveryFrame);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBNeverChanges);

	//PIXSetMarker(PIX_COLOR_INDEX((byte)256), "Drawing Triangles");

	// Draw th triangles that make up the window
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
	g_pImmediateContext->ClearRenderTargetView(g_pBackBufferRenderTargetView, (float*)&g_vClearColor);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	ULONG refs = 0;

	if (g_pImmediateContext) g_pImmediateContext->ClearState();


	for (int i = 0; i < 4; ++i)
	{
		if (g_Buffers[i].m_bIsActive)
		{
			g_Buffers[i].Release(i);
		}

		if (g_Resource[i].m_pSampler)
			refs = g_Resource[i].m_pSampler->Release();
		else
			refs = 0;
		if (refs > 0)
			_RPTF2(_CRT_WARN, "Main Sampler %i still has %i references.\n", i, refs);
	}

	g_pTexturelib->Release();

	SAFE_RELEASE(g_pCBNeverChanges);

	SAFE_RELEASE(g_pCBNeverChanges);
	SAFE_RELEASE(g_pCBChangesEveryFrame);
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pIndexBuffer);
	SAFE_RELEASE(g_d3dInputLayout);
	SAFE_RELEASE(g_pVertexShader);
	SAFE_RELEASE(g_pPixelShader);

	SAFE_RELEASE(g_pDepthStencilBuffer);
	SAFE_RELEASE(g_pDepthStencilView);

	SAFE_RELEASE(g_pRenderTargetTexture);
	SAFE_RELEASE(g_pRenderTargetView);
	SAFE_RELEASE(g_pShaderResourceView);

	SAFE_RELEASE(g_pBackBufferRenderTargetView);
	SAFE_RELEASE(g_pDepthStencilState);
	SAFE_RELEASE(g_pBlendState);
	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pImmediateContext);

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	SAFE_RELEASE(g_pd3dDevice);
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
		g_bTrackMouse = false;

	// Controls Window
	int buttonPress = 0;
	ControlWindow(g_bPause, g_fGameT, g_fPlaySpeed, g_vClearColor, buttonPress, g_eDefaultEditor);

	if (buttonPress & 0x0001)
		ReloadShaders();

	// Constant buffer Data
	ConstantBufferInfoWindow(g_pRenderTargetTexture, g_Resource, g_Buffers, g_iFrame, g_bPause, g_fGameT, g_fDeltaT, g_vMouse);

	// Box for ShaderToy image to be displayed in
	if (g_vCurrentWindowSize.x != g_vWindowSize.x && g_vCurrentWindowSize.y != g_vWindowSize.y)
	{
		// Updating the texture size per buffer
		for (int i = 0; i < 4; ++i)
		{
			if (g_Buffers[i].m_bIsActive)
			{
				g_Buffers[i].ResizeTexture(g_pd3dDevice, g_pImmediateContext, (UINT)g_vWindowSize.x, (UINT)g_vWindowSize.y);
				
				g_Buffers[i].m_BufferResolution = DirectX::XMFLOAT4(g_vWindowSize.x, g_vWindowSize.y, 0.0f, 0.0f);
			}
		}

		// Releasing objects before creating new ones
		g_pRenderTargetTexture->Release();
		g_pRenderTargetView->Release();
		g_pShaderResourceView->Release();

		g_pDepthStencilView->Release();
		g_pDepthStencilState->Release();
		g_pDepthStencilBuffer->Release();

		Create2DTexture(g_pd3dDevice, &g_pRenderTargetTexture, &g_pRenderTargetView, &g_pShaderResourceView, (UINT)g_vWindowSize.x, (UINT)g_vWindowSize.y);
		CreateDepthStencilView(g_pd3dDevice, &g_pDepthStencilView, &g_pDepthStencilState, &g_pDepthStencilBuffer, (UINT)g_vWindowSize.x, (UINT)g_vWindowSize.y);

		g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
		g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilState, NULL);

		// Resizing the viewport
		D3D11_VIEWPORT vp;
		vp.Width = (FLOAT)g_vWindowSize.x;
		vp.Height = (FLOAT)g_vWindowSize.y;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;

		g_pImmediateContext->RSSetViewports(1, &vp);

		g_vCurrentWindowSize = g_vWindowSize;
	}

	MainImageWindow(
		g_pShaderResourceView,
		g_Resource,
		g_Buffers,
		g_vWindowSize,
		g_vCurrentWindowSize,
		g_windowFlags, g_iPadding, g_bTrackMouse);

	int iHovered = -1;

	// Box for the resources
	ResourceWindow(g_pImmediateContext, g_Resource, g_Buffers, g_pTexturelib, g_iPadding, g_iPressIdentifier, iHovered);

	// Toggle view
	ViewToggleWindow(g_iPadding, g_Buffers, iHovered);

	// Shader Compiler Errors
	ShaderErrorWindow(g_Buffers, g_eDefaultEditor);
}

//--------------------------------------------------------------------------------------
// Reload the Pixel Shaders
//--------------------------------------------------------------------------------------
void ReloadShaders()
{
	HRESULT hr = S_OK;

	for (int i = 0; i < 4; ++i)
	{
		if (g_Buffers[i].m_bIsActive)
			g_Buffers[i].ReloadShader(g_pd3dDevice, g_pVertexShader, i);
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(L"shaders/PixelShader.hlsl", "main", "ps_4_0", &pPSBlob, g_vShaderErrorList);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
		return;
	}

	g_pPixelShader->Release();
	g_pPixelShader = nullptr;

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