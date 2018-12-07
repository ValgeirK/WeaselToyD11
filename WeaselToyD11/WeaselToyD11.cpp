#include <ctime>
#include <chrono>

#include "lib/imgui.h"
#include "lib/imgui_impl_dx11.h"
#include "lib/imgui_impl_win32.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <string.h>
#include <vector>

#include "common/Textures.h"
#include "common/Loader.h"
#include "common/Shader.h"
#include "common/Buffer.h"
#include "common/HelperFunction.h"
#include "common/TextureLib.h"
#include "common/ImGuiWindows.h"
#include "common/RegisterHelper.h"
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
ID3D11Buffer*               g_pCBCustomizable = nullptr;

// Shader reflection
ID3D11ShaderReflection*		g_pPixelShaderReflection = nullptr;

// Demo
TextureLib*					g_pTexturelib;
Channel						g_Channels[4];
DirectX::XMFLOAT4           g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);
DirectX::XMFLOAT4			g_vMouse(0.0f, 0.0f, 0.0f, 0.0f);
RECT						g_vAppSize = { 0, 0, 0, 0 };
ImVec2						g_vCurrentWindowSize;
ImVec2						g_vWindowSize;
ImVec2						g_vPadding = ImVec2(16.0f, 65.0f);
ImGuiWindowFlags			g_windowFlags;
float				        g_fLastT = 0.0f;
float						g_fDeltaT = 0.0f;
float						g_fGameT = 0.0f;
float						g_fPauseT = 0;
float						g_fDPIscale = 1.0f;
int						    g_iFrame = 0;
bool			            g_bPause = false;
ImVec4						g_vClearColor = ImVec4(0.08f, 0.12f, 0.14f, 1.00f);
int							g_iPressIdentifier = 0;
int							g_iPadding = 0;
bool					    g_bTrackMouse = false;
bool						g_bFullscreen = false;
bool						g_bResChanged = false;
bool						g_bFullWindow = false;
bool						g_bCtrl = false;
bool						g_bAlt = false;
bool						g_bAutoReload = false;
bool						g_bNewProjLoad = false;
bool						g_bDefaultEditorSelected = false;
float						g_fPlaySpeed = 1.0f;
time_t						g_tTimeCreated;
std::vector<std::string>	g_vShaderErrorList;
std::string					g_strProject = "NewProject";
std::string					g_strDefaultEditor = "";

ImGuiEnum::DefaultEditor	g_eDefaultEditor = ImGuiEnum::DefaultEditor::E_NOTEPAD;
ImGuiEnum::AspectRatio		g_eAspectRatio = ImGuiEnum::AspectRatio::E_NONE;
ImGuiEnum::Resolution		g_eResolution = ImGuiEnum::Resolution::E_CUSTOM;

ImVec4						g_vResourceWindow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
ImVec4						g_vMainImageWindow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
ImVec4						g_vControlWindow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
ImVec4						g_vShaderErrorWindow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

std::vector<CustomizableBuffer> g_vCustomizableBuffer;
std::chrono::high_resolution_clock::time_point	g_tLastClick;



//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT				InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT				InitDevice();
HRESULT				InitResources(bool);
HRESULT				LoadProject();
void				CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void				Render();
void				ReloadShaders();
void				Resize(const float, const float);

void				InitImGui();
void				ImGuiSetup(HINSTANCE);
void				ImGuiRender();


//--------------------------------------------------------------------------------------
// Populate initial variables
//--------------------------------------------------------------------------------------
void Initialize()
{
	int enumEditor = 1;
	int exists = ReadIni(g_strProject, g_strDefaultEditor, g_vClearColor, g_vAppSize, enumEditor, g_bAutoReload);
	g_eDefaultEditor = static_cast<ImGuiEnum::DefaultEditor>(enumEditor);

	const char* pathToCheck = "../../ShaderToyLibrary/";
	DWORD dwAttrib = GetFileAttributes(pathToCheck);

	if (dwAttrib == INVALID_FILE_ATTRIBUTES)
	{
		// If this folder doesn't exist then we create it and create a new project 
		const char* pathToCreate = "..\\..\\ShaderToyLibrary\\NewProject";
		system((std::string("md ") + pathToCreate + std::string("\\channels")).c_str());
		system((std::string("md ") + pathToCreate + std::string("\\shaders")).c_str());
		system((std::string("xcopy ") + std::string(".\\channels ") + pathToCreate + std::string("\\channels") + std::string(" /i /E")).c_str());
		system((std::string("xcopy ") + std::string(".\\shaders ") + pathToCreate + std::string("\\shaders") + std::string(" /i /E")).c_str());
	}

	if (g_strDefaultEditor != "" || exists == 1)
		g_bDefaultEditorSelected = true;
	else
	{
		std::wstring strKeyDefaultValue;
		GetStringRegKey(
			static_cast<int>(g_eDefaultEditor),
			L"",
			strKeyDefaultValue,
			L"bad"
		);

		if (strKeyDefaultValue.length() > 0)
			std::copy(g_strDefaultEditor.begin(), g_strDefaultEditor.end(), strKeyDefaultValue.begin());
	}
}

void DefaultImGuiWindows()
{
	const char* fileToCheck = "imgui.ini";
	DWORD dwAttrib = GetFileAttributes(fileToCheck);

	if (dwAttrib == INVALID_FILE_ATTRIBUTES)
	{
		// if the imgui.ini file doesn't exist then the imgui windows will
		// most likely be in odd shapes and sizes, so we need to fix it

		float width = g_vAppSize.right - g_vAppSize.left;
		float height = g_vAppSize.bottom - g_vAppSize.top;

		g_bResChanged = true;
		g_vControlWindow = ImVec4(
			1.0f,
			1.0f,
			width * 0.18f,
			height * 0.4f
		);
		g_vMainImageWindow = ImVec4(
			5.0f + width * 0.18f,
			height * 0.12f,
			width * 0.67f,
			height * 0.7f
		);
		g_vResourceWindow = ImVec4(
			width * 0.85f + 10.0f,
			height * 0.25f,
			width * 0.14f,
			height * 0.45f
		);
		g_vShaderErrorWindow = ImVec4(
			1.0f,
			height * 0.9f,
			width * 0.7f,
			height * 0.15f
		);
	}
}

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

	Initialize();

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	//MessageBox(nullptr,
	//	(LPCSTR)"Start Pix.", (LPCSTR)"Error", MB_OK);
	{
		if (FAILED(InitDevice()))
		{
			CleanupDevice();
			return 0;
		}
	}

	DefaultImGuiWindows();

	InitImGui();

	g_tLastClick = std::chrono::high_resolution_clock::now();

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

			{
				// Possibly change project
				if (g_bNewProjLoad)
				{
					LoadProject();
					g_bNewProjLoad = false;
				}

				{
					Render();
				}

				if (!g_bFullscreen)
				{
					{
						ImGuiSetup(hInstance);
					}

					{
						ImGuiRender();
					}
				}

				{
					//D3D_VERIFY(g_pSwapChain->Present(1, 0));
					HRESULT hr = S_OK;
					hr = g_pSwapChain->Present(1, 0);
					if(hr != S_OK)
						_RPTF2(_CRT_WARN, "Present error %i.\n", ((hr) & 0x0000FFFF));
				}
			}
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
	
	if (g_vAppSize.right == 0 || g_vAppSize.bottom == 0)
	{
		// Get desktop resolution
		const HWND hDesktop = GetDesktopWindow();
		RECT desktop;
		GetWindowRect(hDesktop, &desktop);
		g_vAppSize = { 0, 0, desktop.right * 2 / 3, desktop.bottom * 2 / 3 };
		g_vWindowSize = ImVec2((float)(desktop.right * 2 / 3), (float)(desktop.bottom * 2 / 3));
	}

	AdjustWindowRect(&g_vAppSize, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow((LPCSTR)"TutorialWindowClass", (LPCSTR)"WeaselToyD11",
		WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, g_vAppSize.right - g_vAppSize.left, g_vAppSize.bottom - g_vAppSize.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

void GoFullscreen()
{
	const HWND hDesktop = GetDesktopWindow();
	RECT desktop;
	GetWindowRect(hDesktop, &desktop);

	g_pImmediateContext->OMSetRenderTargets(0, 0, 0);

	// Release all outstanding references to the swap chain's buffers.
	g_pBackBufferRenderTargetView->Release();

	HRESULT hr;
	// Preserve the existing buffer count and format.
	// Automatically choose the width and height to match the client rect for HWNDs.
	hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	// Perform error handling here!

	// Get buffer and create a render-target-view.
	ID3D11Texture2D* pBuffer;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(void**)&pBuffer);
	// Perform error handling here!

	hr = g_pd3dDevice->CreateRenderTargetView(pBuffer, NULL,
		&g_pBackBufferRenderTargetView);
	// Perform error handling here!
	pBuffer->Release();

	g_vWindowSize.x = (float)desktop.right;
	g_vWindowSize.y = (float)desktop.bottom;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, NULL);

	Resize((float)desktop.right, (float)desktop.bottom);

	g_pSwapChain->SetFullscreenState(TRUE, NULL);
	g_bFullWindow = true;
}

void GoWindowed()
{
	const HWND hDesktop = GetDesktopWindow();
	RECT desktop;
	GetWindowRect(hDesktop, &desktop);

	g_pImmediateContext->OMSetRenderTargets(0, 0, 0);

	// Release all outstanding references to the swap chain's buffers.
	g_pBackBufferRenderTargetView->Release();

	HRESULT hr;
	// Preserve the existing buffer count and format.
	// Automatically choose the width and height to match the client rect for HWNDs.
	hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	// Perform error handling here!

	// Get buffer and create a render-target-view.
	ID3D11Texture2D* pBuffer;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(void**)&pBuffer);
	// Perform error handling here!

	hr = g_pd3dDevice->CreateRenderTargetView(pBuffer, NULL,
		&g_pBackBufferRenderTargetView);
	// Perform error handling here!
	pBuffer->Release();

	g_vWindowSize.x = (float)desktop.right;
	g_vWindowSize.y = (float)desktop.bottom;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, NULL);

	Resize((float)desktop.right, (float)desktop.bottom);

	g_pSwapChain->SetFullscreenState(FALSE, NULL);
	g_bFullWindow = false;
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

	case WM_KEYDOWN:
		switch (wParam)
		{
			case VK_F11:
				g_bFullscreen = !g_bFullscreen;
				break;
			case VK_F5:
				// Reload Shaders
				ReloadShaders();
				break;
			case 'P':
			case VK_SPACE:
				// Pause
				g_bPause = !g_bPause;
				break;
			case VK_CONTROL:
				g_bCtrl = true;
				break;
			case VK_ESCAPE:
				if (g_bFullscreen)
					g_bFullscreen = false;
				break;
			case 'Q':
				// Quit
				if (g_bCtrl)
					DestroyWindow(hWnd);
				break;
		}
		break;
	case WM_SYSKEYDOWN:
		switch (wParam)
		{
		case VK_MENU:
			g_bAlt = true;
			break;
		}
		break;
	case WM_SYSKEYUP:
		switch (wParam)
		{
		case VK_MENU:
			g_bAlt = false;
			break;
		case VK_RETURN:
			if (g_bAlt)
			{
				if (!g_bFullWindow)
					GoFullscreen();
				else
					GoWindowed();
			}
			break;
		}
		break;
	case WM_KEYUP:
	{
		switch (wParam)
		{
		case VK_CONTROL:
			g_bCtrl = false;
			break;
		}
		break;
	}
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

		std::chrono::high_resolution_clock::time_point newClick;
		newClick = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(newClick - g_tLastClick);
		if (time_span.count() < 0.3)
		{
			if (g_bFullscreen)
				g_bFullscreen = false;
			else if(g_bTrackMouse)
				g_bFullscreen = true;
		}

		g_tLastClick = newClick;

		g_vMouse.z = 0.0f;

		break;
	}
	case WM_RBUTTONUP:
	{
		g_vMouse.w = 0.0f;
		break;
	}
	case WM_SIZE:
	{
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

		if (width == 0 || height == 0)
			break;
		if (g_pSwapChain)
		{
			g_pImmediateContext->OMSetRenderTargets(0, 0, 0);

			// Release all outstanding references to the swap chain's buffers.
			g_pBackBufferRenderTargetView->Release();

			HRESULT hr;
			// Preserve the existing buffer count and format.
			// Automatically choose the width and height to match the client rect for HWNDs.
			hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

			// Perform error handling here!

			// Get buffer and create a render-target-view.
			ID3D11Texture2D* pBuffer;
			hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
				(void**)&pBuffer);
			// Perform error handling here!

			hr = g_pd3dDevice->CreateRenderTargetView(pBuffer, NULL,
				&g_pBackBufferRenderTargetView);
			// Perform error handling here!
			pBuffer->Release();

			// Resize and relocate ImGui windows
			float xScale = (float)width / (g_vAppSize.right - g_vAppSize.left);
			float yScale = (float)height / (g_vAppSize.bottom - g_vAppSize.top);

			// Update where ImGui windows should be
			ImGuiScaleMove(g_vResourceWindow, xScale, yScale);
			ImGuiScaleMove(g_vMainImageWindow, xScale, yScale);
			ImGuiScaleMove(g_vControlWindow, xScale, yScale);
			ImGuiScaleMove(g_vShaderErrorWindow, xScale, yScale);

			g_bResChanged = true;

			g_vWindowSize.x = (float)width;
			g_vWindowSize.y = (float)height;

			g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, NULL);
		}

		g_vAppSize.right = width + g_vAppSize.left;
		g_vAppSize.bottom = height + g_vAppSize.top;

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
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

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
	std::wstring g_strProjectW(g_strProject.length(), L' '); // Make room for characters
	 // Copy string to wstring.
	std::copy(g_strProject.begin(), g_strProject.end(), g_strProjectW.begin());
	hr = CompileShaderFromFile(((std::wstring(L"./shaders/VertexShader.hlsl")).c_str()), "main", "vs_4_0", &pVSBlob, g_vShaderErrorList);
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
	hr = CompileShaderFromFile(((std::wstring(L"../../ShaderToyLibrary/") + g_strProjectW + std::wstring(L"/shaders/PixelShader.hlsl"))).c_str(), "main", "ps_4_0", &pPSBlob, g_vShaderErrorList);
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
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(g_pPixelShader, "PixelShader");

	hr = D3DReflect(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&g_pPixelShaderReflection);
	if (FAILED(hr))
	{
		return hr;
	}
	pPSBlob->Release();

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

	bd.ByteWidth = 16;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBCustomizable);
	if (FAILED(hr))
		return hr;
	
	InitResources(true);

	D3D11_SHADER_DESC desc;
	g_pPixelShaderReflection->GetDesc(&desc);

	ScanShaderForCustomizable(g_strProject.c_str(), g_vCustomizableBuffer);

	UINT sizeofBuffer = 0;

	for (unsigned int iConstant = 0; iConstant < desc.ConstantBuffers; ++iConstant)
	{
		ID3D11ShaderReflectionConstantBuffer* pConstantBuffer = g_pPixelShaderReflection->GetConstantBufferByIndex(iConstant);

		// bufferDesc holds the name of the buffer and how many variables it holds
		D3D11_SHADER_BUFFER_DESC bufferDesc;
		pConstantBuffer->GetDesc(&bufferDesc);

		if (strcmp(bufferDesc.Name, "cbCustomizable") == 0)
		{
			for (unsigned int iVariables = 0; iVariables < bufferDesc.Variables; ++iVariables)
			{
				ID3D11ShaderReflectionVariable* pVar = pConstantBuffer->GetVariableByIndex(iVariables);

				D3D11_SHADER_VARIABLE_DESC varDesc;
				pVar->GetDesc(&varDesc);

				ID3D11ShaderReflectionType* newType = pVar->GetType();

				D3D11_SHADER_TYPE_DESC typeDesc;
				newType->GetDesc(&typeDesc);

				for (int i = 0; i < g_vCustomizableBuffer.size(); ++i)
				{
					if (strcmp(varDesc.Name, g_vCustomizableBuffer[i].strVariable) == 0)
					{
						sizeofBuffer += varDesc.Size;
						g_vCustomizableBuffer[iVariables].offset = varDesc.StartOffset;

						if (typeDesc.Type == D3D_SVT_FLOAT)
						{
							g_vCustomizableBuffer[iVariables].data = new float[varDesc.Size / 4];
							g_vCustomizableBuffer[iVariables].isDataSet = true;

							for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
								((float*)g_vCustomizableBuffer[iVariables].data)[i] = ((float*)varDesc.DefaultValue)[i];
						}
						else if (typeDesc.Type == D3D_SVT_INT)
						{
							g_vCustomizableBuffer[iVariables].data = new int[varDesc.Size / 4];
							g_vCustomizableBuffer[iVariables].isDataSet = true;

							for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
								((int*)g_vCustomizableBuffer[iVariables].data)[i] = ((int*)varDesc.DefaultValue)[i];
						}

						g_vCustomizableBuffer[iVariables].type = static_cast<int>(typeDesc.Type);
						g_vCustomizableBuffer[iVariables].size = varDesc.Size;
					}
				}
			}
		}
	}

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

	const HWND hDesktop = GetDesktopWindow();
	RECT desktop;
	GetWindowRect(hDesktop, &desktop);

	switch (desktop.right)
	{
	case 3840:
		// 4K
		g_fDPIscale = 1.1f;
		break;
	}

	io.FontGlobalScale = 1.2f * g_fDPIscale;

	ImGui_ImplWin32_Init(g_hWnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pImmediateContext);

	// Setup style
	ImGui::StyleColorsDark();
}

//--------------------------------------------------------------------------------------
// Load in textures and initialize the samplers
//--------------------------------------------------------------------------------------
HRESULT InitResources(bool bLoadTextures)
{
	HRESULT hr = S_OK;
	int size = 0;

	for (int i = 0; i < 4; ++i)
	{
		// Initializing all the buffers but they have the active flag set to false
		g_Buffers[i] = Buffer();
	}

	if (bLoadTextures)
	{
		// Populate the texture library
		g_pTexturelib = new TextureLib();
		g_pTexturelib->ParallelLoadDDSTextures(g_pd3dDevice, "textures/*");
	}

	if (!LoadChannels((std::string("../../ShaderToyLibrary/") + g_strProject + std::string("/channels/channels.txt")).c_str(), g_Channels, size))
		return S_FALSE;

	for (int i = 0; i < size; ++i)
	{
		if (g_Channels[i].m_Type == Channels::ChannelType::E_Texture)
		{
			g_Resource[i].m_Type = Channels::ChannelType::E_Texture;

			strcpy(g_Resource[i].m_strTexture, g_Channels[i].m_strTexture);

			// Get texture from texture lib
			g_pTexturelib->GetTexture(g_Channels[i].m_strTexture, &g_Resource[i].m_pShaderResource, g_Resource[i].m_vChannelRes);
		}
		else if (g_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			RECT rc;
			GetClientRect(g_hWnd, &rc);
			UINT width = rc.right - rc.left;
			UINT height = rc.bottom - rc.top;

			g_Resource[i].m_Type = Channels::ChannelType::E_Buffer;

			// Initialize buffer
			g_Buffers[static_cast<int>(g_Channels[i].m_BufferId)].InitBuffer(g_pd3dDevice, g_pImmediateContext, g_pVertexShader, g_pTexturelib, g_Buffers, g_strProject.c_str(), width, height, g_Channels[i].m_BufferId);

			g_Resource[i].m_vChannelRes = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
			g_Resource[i].m_iBufferIndex = static_cast<int>(g_Channels[i].m_BufferId);

			g_Buffers[static_cast<int>(g_Channels[i].m_BufferId)].SetShaderResource(g_pImmediateContext, i);
		}
		else
		{
			g_Resource[i].m_Type = Channels::ChannelType::E_None;
		}

		// Create Sampler
		CreateSampler(g_pd3dDevice, g_pImmediateContext, &g_Resource[i].m_pSampler, g_Channels[i], -1, i);
	}

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Resize the Texture
//--------------------------------------------------------------------------------------
void Resize(const float width, const float height)
{
	// Updating the texture size per buffer
	for (int i = 0; i < 4; ++i)
	{
		if (g_Buffers[i].m_bIsActive)
		{
			if (g_vPadding.x < width && g_vPadding.y < height && !g_bFullscreen && !g_bFullWindow)
			{
				g_Buffers[i].ResizeTexture(g_pd3dDevice, g_pImmediateContext, (UINT)(width - g_vPadding.x), (UINT)(height - g_vPadding.y));

				g_Buffers[i].m_BufferResolution = DirectX::XMFLOAT4(width - g_vPadding.x, height - g_vPadding.y, 0.0f, 0.0f);
			}
			else
			{
				g_Buffers[i].ResizeTexture(g_pd3dDevice, g_pImmediateContext, (UINT)(width), (UINT)(height));

				g_Buffers[i].m_BufferResolution = DirectX::XMFLOAT4(width, height, 0.0f, 0.0f);
			}
		}
	}

	// Releasing objects before creating new ones
	g_pRenderTargetTexture->Release();
	g_pRenderTargetView->Release();
	g_pShaderResourceView->Release();

	g_pDepthStencilView->Release();
	g_pDepthStencilState->Release();
	g_pDepthStencilBuffer->Release();

	if (g_vPadding.x < width && g_vPadding.y < height && !g_bFullscreen && !g_bFullWindow)
	{
		Create2DTexture(g_pd3dDevice, &g_pRenderTargetTexture, &g_pRenderTargetView, &g_pShaderResourceView, (UINT)(width - g_vPadding.x), (UINT)(height - g_vPadding.y));
		CreateDepthStencilView(g_pd3dDevice, &g_pDepthStencilView, &g_pDepthStencilState, &g_pDepthStencilBuffer, (UINT)(width - g_vPadding.x), (UINT)(height - g_vPadding.y));
	}
	else
	{
		Create2DTexture(g_pd3dDevice, &g_pRenderTargetTexture, &g_pRenderTargetView, &g_pShaderResourceView, (UINT)(width), (UINT)(height));
		CreateDepthStencilView(g_pd3dDevice, &g_pDepthStencilView, &g_pDepthStencilState, &g_pDepthStencilBuffer, (UINT)(width), (UINT)(height));
	}

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	g_pImmediateContext->OMSetDepthStencilState(g_pDepthStencilState, NULL);

	// Resizing the viewport
	D3D11_VIEWPORT vp;
	if (g_vPadding.x < width && g_vPadding.y < height && !g_bFullscreen && !g_bFullWindow)
	{
		vp.Width = (width - g_vPadding.x);
		vp.Height = (height - g_vPadding.y);
	}
	else
	{
		vp.Width = width;
		vp.Height = height;
	}
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	g_pImmediateContext->RSSetViewports(1, &vp);

	g_vCurrentWindowSize = ImVec2(width, height);
}


//--------------------------------------------------------------------------------------
// Auto Reload
//--------------------------------------------------------------------------------------
void AutoReload()
{
	const char* filename[5] = { 
		"/shaders/PixelShader.hlsl",
		"/shaders/PixelShaderBufferA.hlsl",
		"/shaders/PixelShaderBufferB.hlsl",
		"/shaders/PixelShaderBufferC.hlsl",
		"/shaders/PixelShaderBufferD.hlsl",
	};

	struct _stat result;

	std::string path = std::string("../../ShaderToyLibrary/") + g_strProject;

	for (int i = 0; i < 5; ++i)
	{
		if (_stat((path + std::string(filename[i])).c_str(), &result) == 0)
		{
			// Time when file was last modified
			if  (g_tTimeCreated == 0)
				g_tTimeCreated = result.st_mtime;

			if (result.st_mtime > g_tTimeCreated)
			{
				// file has been modified
				g_tTimeCreated = result.st_mtime;
				ReloadShaders();

				// if one needs a reload, then we reload them all
				return;
			}
		}
	}
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
	if(g_bAutoReload)
		AutoReload();

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

	if (!g_bFullscreen)
		g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	else
	{
		RECT window;
		GetWindowRect(g_hWnd, &window);

		Resize((float)(window.right - window.left), (float)(window.bottom - window.top));

		if (g_iPadding != 0)
			g_pImmediateContext->PSSetShader(g_Buffers[g_iPadding - 1].m_pPixelShader, nullptr, 0);

		g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, nullptr);
	}

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
	if (!g_bFullscreen)
		g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);
	else
	{	
		if(g_iPadding == 0)
			g_pImmediateContext->ClearRenderTargetView(g_pBackBufferRenderTargetView, DirectX::Colors::MidnightBlue);
		else
			g_pImmediateContext->ClearRenderTargetView(g_Buffers[g_iPadding - 1].m_pRenderTargetView, DirectX::Colors::Cyan);
	}

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

	for (int i = 0; i < g_vCustomizableBuffer.size(); ++i)
	{
		if(g_vCustomizableBuffer[i].isDataSet)
			g_pImmediateContext->UpdateSubresource(g_pCBCustomizable, g_vCustomizableBuffer[i].offset, nullptr, g_vCustomizableBuffer[i].data, 0, 0);
	}

	// Set constant buffers
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCBCustomizable);
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBChangesEveryFrame);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBNeverChanges);

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
	// Save Imgui state
	ImGui::SaveIniSettingsToDisk("imgui.ini");
	WriteIni(g_strProject.c_str(), g_strDefaultEditor.c_str(), g_vClearColor, g_vAppSize, static_cast<int>(g_eDefaultEditor), g_bAutoReload);
	ULONG refs = 0;
	// revert fullscreen before closing
	g_pSwapChain->SetFullscreenState(FALSE, NULL);

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
	SAFE_RELEASE(g_pCBCustomizable);
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

	// In case we aren't cleaning up correctly
	/*ID3D11Debug *d3dDebug = nullptr;
	g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
	d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL);*/

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

	bool bGoFullChange = false;

	KeepImGuiWindowsInsideApp(g_vAppSize, g_vControlWindow, g_bResChanged);

	// Controls Window
	int buttonPress = 0;
	ControlWindow(
		g_pRenderTargetTexture,
		g_Resource,
		g_vClearColor,
		g_Buffers,
		g_pTexturelib,
		g_eDefaultEditor,
		g_eAspectRatio,
		g_eResolution,
		g_vMouse,
		g_strProject,
		g_bPause, g_bAutoReload,
		g_fGameT, g_fPlaySpeed, 
		g_fDeltaT, g_fDPIscale,
		g_iFrame, buttonPress,
		g_bResChanged,
		g_bNewProjLoad,
		g_bDefaultEditorSelected,
		g_bFullWindow,
		bGoFullChange,
		g_vControlWindow,
		g_vMainImageWindow,
		g_vCustomizableBuffer
	);

	if (bGoFullChange)
	{
		if (!g_bFullWindow)
			GoFullscreen();
		else
			GoWindowed();
	}

	if (buttonPress & 0x0001)
		ReloadShaders();

	int iHovered = -1;

	KeepImGuiWindowsInsideApp(g_vAppSize, g_vResourceWindow, g_bResChanged);

	// Box for the resources
	ResourceWindow(
		g_pd3dDevice,
		g_pImmediateContext,
		g_pVertexShader,
		g_Resource, 
		g_Buffers, 
		g_Channels, 
		g_pTexturelib, 
		g_strProject.c_str(), 
		g_fDPIscale, 
		g_iPadding, 
		g_iPressIdentifier, 
		iHovered, 
		g_bResChanged, 
		g_bNewProjLoad,
		g_vWindowSize,
		g_vResourceWindow
	);

	// Box for ShaderToy image to be displayed in
	if ((((int)g_vCurrentWindowSize.x != (int)g_vWindowSize.x && (int)g_vCurrentWindowSize.y != (int)g_vWindowSize.y) || g_bResChanged) && !g_bFullscreen)
	{
		if (g_vWindowSize.x > 0.0f && g_vWindowSize.y > 0.0f && g_vMainImageWindow.z > 0.0f && g_vMainImageWindow.w > 0.0f)
		{
			if (g_bResChanged)
				Resize(g_vMainImageWindow.z, g_vMainImageWindow.w);
			else
				Resize(g_vWindowSize.x, g_vWindowSize.y);

		}
	}

	KeepImGuiWindowsInsideApp(g_vAppSize, g_vMainImageWindow, g_bResChanged);

	MainImageWindow(
		g_pShaderResourceView,
		g_Resource,
		g_Buffers,
		g_vWindowSize,
		g_vCurrentWindowSize,
		g_vPadding,
		g_windowFlags,
		g_strProject.c_str(),
		g_strDefaultEditor,
		g_eDefaultEditor,
		g_iPadding, iHovered,
		g_fDPIscale,
		g_bTrackMouse, g_bFullscreen,
		g_bResChanged, g_vMainImageWindow);

	KeepImGuiWindowsInsideApp(g_vAppSize, g_vShaderErrorWindow, g_bResChanged);

	// Shader Compiler Errors
	if (g_vShaderErrorList.size() > 0)
	{
		ShaderErrorWindow(
			g_Buffers,
			g_vShaderErrorList,
			g_strProject,
			g_strDefaultEditor,
			g_eDefaultEditor,
			g_bResChanged,
			g_vShaderErrorWindow
		);
	}

	// Selection of default editor
	if (!g_bDefaultEditorSelected)
	{	
		ImVec2 size = ImVec2((float)(g_vAppSize.right + g_vPadding.x), (float)(g_vAppSize.bottom + g_vPadding.y));
		ImGui::SetNextWindowFocus();
		Barrier(size);
		ImGui::SetNextWindowFocus();
		DefaultEditorSelector(ImVec2(size.x / 2, size.y / 2), g_eDefaultEditor, g_strDefaultEditor, g_bDefaultEditorSelected);
	}

	g_bResChanged = false;
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
			g_Buffers[i].ReloadShader(g_pd3dDevice, g_pVertexShader, g_strProject.c_str(), i);
	}

	// Make room for characters
	std::wstring wStrProj(g_strProject.length(), L' ');
	// Copy string to wstring.
	std::copy(g_strProject.begin(), g_strProject.end(), wStrProj.begin());

	std::wstring path = std::wstring(L"../../ShaderToyLibrary/") + wStrProj;

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	g_vShaderErrorList.clear();
	hr = CompileShaderFromFile((path + std::wstring(L"/shaders/PixelShader.hlsl")).c_str(), "main", "ps_4_0", &pPSBlob, g_vShaderErrorList);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
	}

	g_pPixelShader->Release();
	g_pPixelShader = nullptr;

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error creating pixel shader.", (LPCSTR)"Error", MB_OK);
		return;
	}

	if (FAILED(D3DReflect(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&g_pPixelShaderReflection)))
	{
		return;
	}
	pPSBlob->Release();

	D3D11_SHADER_DESC desc;
	g_pPixelShaderReflection->GetDesc(&desc);

	std::vector<CustomizableBuffer> customizableBufferCopy;
	for (int i = 0; i < g_vCustomizableBuffer.size(); ++i)
	{
		// Copy the customizable variables so we can maintain the potentially 
		// modified values
		CustomizableBuffer cb;
		cb = g_vCustomizableBuffer[i];
		customizableBufferCopy.push_back(cb);
	}

	// Read the shader
	ScanShaderForCustomizable(g_strProject.c_str(), g_vCustomizableBuffer);

	UINT sizeofBuffer = 0;

	for (unsigned int iConstant = 0; iConstant < desc.ConstantBuffers; ++iConstant)
	{
		ID3D11ShaderReflectionConstantBuffer* pConstantBuffer = g_pPixelShaderReflection->GetConstantBufferByIndex(iConstant);

		// bufferDesc holds the name of the buffer and how many variables it holds
		D3D11_SHADER_BUFFER_DESC bufferDesc;
		pConstantBuffer->GetDesc(&bufferDesc);

		if (strcmp(bufferDesc.Name, "cbCustomizable") == 0)
		{
			for (unsigned int iVariables = 0; iVariables < bufferDesc.Variables; ++iVariables)
			{
				ID3D11ShaderReflectionVariable* pVar = pConstantBuffer->GetVariableByIndex(iVariables);

				D3D11_SHADER_VARIABLE_DESC varDesc;
				pVar->GetDesc(&varDesc);

				ID3D11ShaderReflectionType* newType = pVar->GetType();

				D3D11_SHADER_TYPE_DESC typeDesc;
				newType->GetDesc(&typeDesc);

				for (int i = 0; i < g_vCustomizableBuffer.size(); ++i)
				{
					if (strcmp(varDesc.Name, g_vCustomizableBuffer[i].strVariable) == 0)
					{
						sizeofBuffer += varDesc.Size;
						g_vCustomizableBuffer[iVariables].offset = varDesc.StartOffset;

						int copyIndex = -1;

						for (int j = 0; j < customizableBufferCopy.size(); ++j)
						{
							if (strcmp(customizableBufferCopy[j].strVariable, varDesc.Name) == 0)
							{
								// we want to keep the values that have possibly been altered
								customizableBufferCopy[j].isDataSet = true;
								copyIndex = j;
							}
						}

						if (typeDesc.Type == D3D_SVT_FLOAT)
						{
							g_vCustomizableBuffer[iVariables].data = new float[varDesc.Size / 4];

							for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
							{
								if (customizableBufferCopy[iVariables].isDataSet && sizeof(float) * (i + 1) <= customizableBufferCopy[iVariables].size)
									((float*)g_vCustomizableBuffer[iVariables].data)[i] = ((float*)(customizableBufferCopy[copyIndex]).data)[i];
								else
									((float*)g_vCustomizableBuffer[iVariables].data)[i] = ((float*)varDesc.DefaultValue)[i];
							}

							g_vCustomizableBuffer[iVariables].isDataSet = true;
						}
						else if (typeDesc.Type == D3D_SVT_INT)
						{
							g_vCustomizableBuffer[iVariables].data = new int[varDesc.Size / 4];

							for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
							{
								if (customizableBufferCopy[iVariables].isDataSet && sizeof(int) * (i + 1) <= customizableBufferCopy[iVariables].size)
									((int*)g_vCustomizableBuffer[iVariables].data)[i] = ((int*)(customizableBufferCopy[copyIndex]).data)[i];
								else
									((int*)g_vCustomizableBuffer[iVariables].data)[i] = ((int*)varDesc.DefaultValue)[i];
							}

							g_vCustomizableBuffer[iVariables].isDataSet = true;
						}

						g_vCustomizableBuffer[iVariables].type = static_cast<int>(typeDesc.Type);
						g_vCustomizableBuffer[iVariables].size = varDesc.Size;
					}
				}
			}
		}
	}

	D3D11_BUFFER_DESC bd = {};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = (sizeofBuffer / 16 + 1) * 16;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;

	SAFE_RELEASE(g_pCBCustomizable);
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBCustomizable);

	// Set the shaders
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
}

//--------------------------------------------------------------------------------------
// Reload the Pixel Shaders
//--------------------------------------------------------------------------------------
HRESULT LoadProject()
{
	HRESULT hr = S_OK;

	std::wstring g_strProjectW(g_strProject.length(), L' '); // Make room for characters
	 // Copy string to wstring.
	std::copy(g_strProject.begin(), g_strProject.end(), g_strProjectW.begin());

	// Load new pixel shader
	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = CompileShaderFromFile(((std::wstring(L"../../ShaderToyLibrary/") + g_strProjectW + std::wstring(L"/shaders/PixelShader.hlsl"))).c_str(), "main", "ps_4_0", &pPSBlob, g_vShaderErrorList);
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
	}

	// Create the pixel shader
	g_pPixelShader->Release();
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	for (int i = 0; i < 4; ++i)
	{
		if (g_Buffers[i].m_bIsActive)
		{
			g_Buffers[i].Release(i);
		}

		if (g_Resource[i].m_pSampler)
			g_Resource[i].m_pSampler->Release();
	}

	InitResources(false);
	Resize(g_vWindowSize.x, g_vWindowSize.y);
	return hr;
}