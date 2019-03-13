#include <ctime>
#include <chrono>

#include "lib/imgui.h"
#include "lib/imgui_impl_dx11.h"
#include "lib/imgui_impl_win32.h"

#include <d3d9.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <pix3.h>
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
#include "common/Reflection.h"
#include "common/type/ConstantBuffer.h"
#include "common/type/Channel.h"
#include "common/type/Resource.h"
#include "common/type/HashDefines.h"

#include "renderdoc_app.h"

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
SimpleVertex g_Vertices[QUAD_VERTICE_NUMB] =
{
	{ DirectX::XMFLOAT3(-1.0f,  1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },
	{ DirectX::XMFLOAT3( 1.0f, -1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
	{ DirectX::XMFLOAT3(-1.0f, -1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
	{ DirectX::XMFLOAT3( 1.0f,  1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) }
};

WORD g_Indicies[QUAD_INDICE_NUMB] =
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
ID3D11BlendState*			g_pAlphaEnableBlendingState = nullptr;
ID3D11BlendState*			g_pAlphaDisableBlendingState = nullptr;

// Shaders
ID3D11VertexShader*         g_pVertexShader = nullptr;
ID3D11PixelShader*          g_pPixelShader = nullptr;

// Resource
Resource					g_Resource[MAX_RESORCESCHANNELS];

// Buffers
Buffer						g_Buffers[MAX_RESORCESCHANNELS];

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
Channel						g_Channels[MAX_RESORCESCHANNELS];
DirectX::XMFLOAT4			g_vMouse(0.0f, 0.0f, 0.0f, 0.0f);
RECT						g_vAppSize = { 0, 0, 0, 0 };
RECT						g_vDesktop;

float				        g_fLastT = 0.0f;
float						g_fDeltaT = 0.0f;
float						g_fGameT = 0.0f;
float						g_fPauseT = 0;
float						g_fDPIscale = 1.0f;
float						g_fPlaySpeed = 1.0f;
int							g_piBufferUsed[MAX_RESORCESCHANNELS];
int						    g_iFrame = 0;
int							g_iPressIdentifier = 0;
int							g_iLocation = 0;
bool			            g_bPause = false;
bool					    g_bTrackMouse = false;
bool						g_bMouseMoved = false;
bool						g_bExpandedImage = false;
bool						g_bResChanged = false;
bool						g_bFullWindow = false;
bool						g_bCtrl = false;
bool						g_bAlt = false;
bool						g_bAutoReload = false;
bool						g_bNewProjLoaded = false;
bool						g_bDefaultEditorSelected = false;
bool						g_bVsync = true;
bool						g_bNeedsResize = true;
bool						g_bGrabbingFrame = false;
bool						g_bUseRenderdoc = false;
time_t						g_tTimeCreated;
std::vector<std::string>	g_vShaderErrorList;
std::string					g_strProject = DEFAULT_PROJECT_NAME;
std::string					g_strDefaultEditor = "";

ImGuiEnum::DefaultEditor	g_eDefaultEditor = ImGuiEnum::DefaultEditor::E_NOTEPAD;
ImGuiEnum::AspectRatio		g_eAspectRatio = ImGuiEnum::AspectRatio::E_NONE;
ImGuiEnum::Resolution		g_eResolution = ImGuiEnum::Resolution::E_CUSTOM;

ImVec2						g_vCurrentWindowSize;
ImVec2						g_vWindowSize;
ImVec2						g_vPadding = ImVec2(16.0f, 65.0f);
ImVec4						g_vClearColour = ImVec4(0.08f, 0.12f, 0.14f, 1.00f);
ImVec4						g_vClearColourFade;
ImVec2						g_vImageOffset = ImVec2(0.0f, 0.0f);
ImVec4						g_vResourceWindow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
ImVec4						g_vMainImageWindow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
ImVec4						g_vControlWindow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
ImVec4						g_vShaderErrorWindow = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
ImGuiWindowFlags			g_windowFlags;

DWORD						g_dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

RENDERDOC_API_1_1_2*		g_rdoc_api = nullptr;


std::vector<CustomizableBuffer>					g_vCustomizableBuffer;
UINT											g_iCustomizableBufferSize = 0;
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
void				ReloadTextures();
void				Resize(const float, const float);

void				InitImGui();
void				ImGuiSetup(HINSTANCE);
void				ImGuiRender();


//--------------------------------------------------------------------------------------
// Populate initial variables
//--------------------------------------------------------------------------------------
void Initialize()
{
	g_vClearColourFade = g_vClearColour;

	int enumEditor = 1;
	int exists = ReadIni(g_strProject, g_strDefaultEditor, g_vClearColour, g_vAppSize, g_dwShaderFlags, enumEditor, g_bAutoReload, g_bUseRenderdoc);
	g_eDefaultEditor = static_cast<ImGuiEnum::DefaultEditor>(enumEditor);

	const char* pathToCheck = PROJECT_PATH;
	DWORD dwAttrib = GetFileAttributes(pathToCheck);

	if (dwAttrib == INVALID_FILE_ATTRIBUTES)
	{
		// If this folder doesn't exist then we create it and create a new project 
		std::string pathToCreate = PROJECT_PATH_DOUBLE_SLASH + std::string(DEFAULT_PROJECT_NAME);
		system((std::string("md \"") + pathToCreate + std::string("\\channels\"")).c_str());
		system((std::string("md \"") + pathToCreate + std::string("\\shaders\"")).c_str());
		system((std::string("xcopy ") + std::string(".\\channels \"") + pathToCreate + std::string("\\channels") + std::string("\" /i /E")).c_str());
		system((std::string("xcopy ") + std::string(".\\shaders \"") + pathToCreate + std::string("\\shaders") + std::string("\" /i /E")).c_str());
	}

	std::string projectPathToCheck = PROJECT_PATH + std::string(g_strProject);
	dwAttrib = GetFileAttributes(projectPathToCheck.c_str());

	if (dwAttrib == INVALID_FILE_ATTRIBUTES || g_strProject == "")
	{
		// If this project doesn't exist then we fall back to the default one
		g_strProject = DEFAULT_PROJECT_NAME;
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

	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &g_vDesktop);

	if (dwAttrib == INVALID_FILE_ATTRIBUTES)
	{
		// if the imgui.ini file doesn't exist then the imgui windows will
		// most likely be in odd shapes and sizes, so we need to fix it

		float width = (float)(g_vAppSize.right - g_vAppSize.left);
		float height = (float)(g_vAppSize.bottom - g_vAppSize.top);

		switch (g_vDesktop.right)
		{
		case 3840:
			// 4K
			g_vControlWindow = ImVec4(
				1.0f,
				23.0f,
				width * 0.18f,
				height * 0.5f
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
			break;
		default:
			// 1080p
			g_vControlWindow = ImVec4(
				1.0f,
				23.0f,
				width * 0.24f,
				height * 0.65f
			);
			g_vMainImageWindow = ImVec4(
				width * 0.24f,
				1.0f,
				width * 0.76f,
				height * 0.85f
			);
			g_vResourceWindow = ImVec4(
				1.0f,
				height * 0.85f,
				width * 0.30f,
				height * 0.15f
			);
			g_vShaderErrorWindow = ImVec4(
				width * 0.30f,
				height * 0.85f,
				width * 0.70f,
				height * 0.15f
			);
			break;
		}

		g_bResChanged = true;
		
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

	//Initialize RenderDoc handle
	std::wstring wPath;

	if (g_bUseRenderdoc && GetRenderDocLoc(wPath))
	{
		std::string path(wPath.begin(), wPath.end());
		
		if (RenderdocCheck(path))
		{
			HINSTANCE hinstLib;
			hinstLib = LoadLibraryW(wPath.c_str());

			if (hinstLib != nullptr)
			{
				HMODULE mod = GetModuleHandleW(wPath.c_str());
				if (mod)
				{
					pRENDERDOC_GetAPI RENDERDOC_GetAPI =
						(pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
					int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&g_rdoc_api);
					assert(ret == 1);

					g_rdoc_api->SetCaptureFilePathTemplate((std::string(PROJECT_PATH) + g_strProject + std::string(WEASEL_CAPTURE)).c_str());
				}
			}
		}
	}

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	//MessageBox(nullptr,
	//	(LPCSTR)"Start Pix.", (LPCSTR)"Error", MB_OK);

	PIXBeginEvent(0, "Initializing Device");
	{
		if (FAILED(InitDevice()))
		{
			CleanupDevice();
			return 0;
		}
	}
	PIXEndEvent(); // Initializing Device

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

			PIXBeginEvent(PIX_COLOR_INDEX(col++), "MainLoop");
			{
				// Possibly change project
				if (g_bNewProjLoaded)
				{
					for (int j = 0; j < MAX_RESORCESCHANNELS; j++)
						g_piBufferUsed[j] = false;

					LoadProject();
					g_bNewProjLoaded = false;
					ReloadShaders();
				}

				// Render the ShaderToy image
				PIXBeginEvent(PIX_COLOR_INDEX(col++), "Render");
				{
					Render();
				}
				PIXEndEvent(); // RENDER

				if (!g_bExpandedImage)
				{
					PIXBeginEvent(PIX_COLOR_INDEX(col++), "ImGuiSetup");
					{
						D3DPERF_BeginEvent(0x0000ff, L"Main - ImGui Setup");
						ImGuiSetup(hInstance);
						D3DPERF_EndEvent();
					}
					PIXEndEvent(); // IMGUISETUP


					PIXBeginEvent(PIX_COLOR_INDEX(col++), "ImGuiRender");
					{
						D3DPERF_BeginEvent(0x0000ff, L"Main - ImGui Render");
						ImGuiRender();
						D3DPERF_EndEvent();
					}
					PIXEndEvent();
				}

				PIXBeginEvent(PIX_COLOR_INDEX(col++), "Present");
				{
					//D3D_VERIFY(g_pSwapChain->Present(1, 0));
					HRESULT hr = S_OK;
					hr = g_pSwapChain->Present(g_bVsync, 0);
					if (hr != S_OK)
					{
						_RPTF2(_CRT_WARN, "Present error %i.\n", ((hr) & 0x0000FFFF));
					}
				}
				PIXEndEvent(); // PRESENT
			}
			PIXEndEvent(); // MAIN LOOP
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

	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, g_pDepthStencilView);

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

	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, g_pDepthStencilView);

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
				g_bExpandedImage = !g_bExpandedImage;
				g_bNeedsResize = true;
				break;
			case VK_F5:
				// Reload Shaders
				if (g_bCtrl)
				{
					g_fGameT = 0;

					ReloadTextures();
				}
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
				if (g_bExpandedImage)
					g_bExpandedImage = false;

				g_bNeedsResize = true;
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
		g_bMouseMoved = true;

		if (g_bTrackMouse)
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
			if (g_bExpandedImage)
				g_bExpandedImage = false;
			else if(g_bTrackMouse)
				g_bExpandedImage = true;

			g_bNeedsResize = true;
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

			g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, g_pDepthStencilView);
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

	// Create an alpha enabled blend state description.
	blendState.RenderTarget[0].BlendEnable = TRUE;
	//blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendState.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendState.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendState.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	hr = g_pd3dDevice->CreateBlendState(&blendState, &g_pAlphaEnableBlendingState);
	if (FAILED(hr))
		return hr;

	// Modify the description to create an alpha disabled blend state description.
	blendState.RenderTarget[0].BlendEnable = FALSE;

	// Create the blend state using the description.
	hr = g_pd3dDevice->CreateBlendState(&blendState, &g_pAlphaDisableBlendingState);
	if (FAILED(hr))
		return hr;

	FLOAT BlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	UINT SampleMask = 0xffffffff;

	g_pImmediateContext->OMSetBlendState(g_pAlphaDisableBlendingState, BlendFactor, SampleMask);

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
	hr = CompileShaderFromFile(((std::wstring(PROJECT_PATH_W) + g_strProjectW + std::wstring(L"./shaders/VertexShader.hlsl")).c_str()), "main", "vs_5_0", &pVSBlob, g_dwShaderFlags, g_vShaderErrorList);
	if (FAILED(hr))
	{
		SetForegroundWindow(g_hWnd);
		MessageBox(g_hWnd,
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
	hr = CompileShaderFromFile(((std::wstring(PROJECT_PATH_W) + g_strProjectW + std::wstring(L"/shaders/PixelShader.hlsl"))).c_str(), "mainImage", "ps_5_0", &pPSBlob, g_dwShaderFlags, g_vShaderErrorList);
	if (FAILED(hr))
	{
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			std::string msg = "Error Pixel Shader \"shaders/PixelShader.hlsl\" not found!";

			SetForegroundWindow(g_hWnd);
			MessageBox(g_hWnd,
				(LPCSTR)msg.c_str(), (LPCSTR)"Error", MB_OK);
		}

		SetForegroundWindow(g_hWnd);

		// Trying without a pop-up box
		//MessageBox(g_hWnd,
		//	(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(g_pPixelShader, "PixelShader");

	hr = Reflection::D3D11ReflectionSetup(pPSBlob, &g_pPixelShaderReflection);

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
	
	InitResources(true);

	ScanShaderForCustomizable(g_strProject.c_str(), g_vCustomizableBuffer);

	Reflection::D3D11ShaderReflectionAndPopulation(g_pPixelShaderReflection, g_vCustomizableBuffer, g_iCustomizableBufferSize, std::vector<CustomizableBuffer>());

	Reflection::D3D11ShaderReflection(g_pPixelShaderReflection, g_Resource, g_dwShaderFlags);

	bd.ByteWidth = (g_iCustomizableBufferSize / 16 + 1) * 16;
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBCustomizable);
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(g_pCBNeverChanges, "ConstantBuffer_NeverChanges");
	SetDebugObjectName(g_pCBChangesEveryFrame, "ConstantBuffer_ChangesEveryFrame");

	// Set constant buffers
	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4((float)width, (float)height, 1.0f / (float)width, 1.0f / (float)height);
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

	switch (g_vDesktop.right)
	{
	case 3840:
		// 4K
		g_fDPIscale = 1.1f;
		io.FontGlobalScale = 1.2f * g_fDPIscale;
		break;
	default:
		io.FontGlobalScale = 1.0f;
		break;
	}

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

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
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

	if (!LoadChannels((std::string(PROJECT_PATH) + g_strProject + std::string("/channels/channels.txt")).c_str(), g_Channels, size))
		return S_FALSE;

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		// Initialize all buffers
		RECT rc;
		GetClientRect(g_hWnd, &rc);
		UINT width = rc.right - rc.left;
		UINT height = rc.bottom - rc.top;

		// Initialize buffer
		g_Buffers[i].InitBuffer(g_pd3dDevice, g_pImmediateContext, g_pVertexShader, g_pTexturelib, g_dwShaderFlags, g_strProject.c_str(), g_piBufferUsed, width, height, i);


		if (g_Channels[i].m_Type == Channels::ChannelType::E_Texture)
		{
			g_Resource[i].m_Type = Channels::ChannelType::E_Texture;

			strcpy(g_Resource[i].m_strTexture, g_Channels[i].m_strTexture);

			// Get texture from texture lib
			g_pTexturelib->GetTexture(g_Channels[i].m_strTexture, &g_Resource[i].m_pShaderResource, g_Resource[i].m_vChannelRes);
		}
		else if (g_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			g_Resource[i].m_Type = Channels::ChannelType::E_Buffer;

			g_Resource[i].m_vChannelRes = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
			g_Resource[i].m_iBufferIndex = static_cast<int>(g_Channels[i].m_BufferId);

			g_piBufferUsed[g_Resource[i].m_iBufferIndex] += 1;
		}
		else
		{
			g_Resource[i].m_Type = Channels::ChannelType::E_None;
		}

		// Create Sampler
		CreateSampler(g_pd3dDevice, g_pImmediateContext, &g_Resource[i].m_pSampler, g_Channels[i], 0, i);
	}

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Resize the Texture
//--------------------------------------------------------------------------------------
void Resize(const float width, const float height)
{
	// Updating the texture size per buffer
	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (g_Buffers[i].m_bIsActive)
		{
			if (g_vPadding.x < width && g_vPadding.y < height && !g_bExpandedImage && !g_bFullWindow)
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

	if (g_vPadding.x < width && g_vPadding.y < height && !g_bExpandedImage && !g_bFullWindow)
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
	if (g_vPadding.x < width && g_vPadding.y < height && !g_bExpandedImage && !g_bFullWindow)
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

	std::string path = std::string(PROJECT_PATH) + g_strProject;

	for (int i = 0; i < MAX_RESORCESCHANNELS + 1; ++i)
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

void ClearColourFade()
{
	if ((g_vClearColour.x == g_vClearColourFade.x)
		&& (g_vClearColour.y == g_vClearColourFade.y)
		&& (g_vClearColour.z == g_vClearColourFade.z)
		&& (g_vClearColour.w == g_vClearColourFade.w))
		return;

	g_vClearColour.x -= ((g_vClearColour.x - g_vClearColourFade.x) * 2.0f * GetImGuiDeltaTime());
	g_vClearColour.y -= ((g_vClearColour.y - g_vClearColourFade.y) * 2.0f * GetImGuiDeltaTime());
	g_vClearColour.z -= ((g_vClearColour.z - g_vClearColourFade.z) * 2.0f * GetImGuiDeltaTime());
	g_vClearColour.w -= ((g_vClearColour.w - g_vClearColourFade.w) * 2.0f * GetImGuiDeltaTime());
}

void ClearShaderResource(ID3D11DeviceContext* pImmediateContext, ID3D11DepthStencilView* pDepthStencilView)
{
	D3DPERF_BeginEvent(0xff0000, L"Main - Clearing Shader Resource");
	// Clearing the Shader Resource so it's not bound on clear
	ID3D11ShaderResourceView* renderNull = nullptr;
	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
		pImmediateContext->PSSetShaderResources(i, 1, &renderNull);

	ID3D11RenderTargetView* viewNull = nullptr;
	pImmediateContext->OMSetRenderTargets(1, &viewNull, pDepthStencilView);

	if (renderNull) renderNull->Release();
	if (viewNull) viewNull->Release();

	D3DPERF_EndEvent();
}


//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
void Render()
{
	D3DPERF_BeginEvent(0x00ff00, L"Main - Begin Render");

	if (g_bAutoReload)
	{
		D3DPERF_BeginEvent(0xff0000, L"Main - Reloading");
		AutoReload();
		D3DPERF_EndEvent();
	}

	ClearColourFade();

	if (g_pTexturelib->m_bReload)
	{
		for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
		{
			if (g_Resource[i].m_Type == Channels::ChannelType::E_Texture)
			{
				// Get texture from texture lib
				g_pTexturelib->GetTexture(g_Resource[i].m_strTexture, &g_Resource[i].m_pShaderResource, g_Resource[i].m_vChannelRes);
			}

			g_Buffers->ReloadTexture(g_pTexturelib, i);
		}

		g_pTexturelib->m_bReload = false;
	}

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		// Update buffers
		if (g_piBufferUsed[i])
		{
			// Update the buffer frames
			D3DPERF_BeginEvent(0xff0000, L"Main - Render Buffer");
			g_Buffers[i].Render(g_Buffers, g_pImmediateContext, g_pDepthStencilView, g_pCBNeverChanges, ARRAYSIZE(g_Indicies), (UINT)g_vCurrentWindowSize.x, (UINT)g_vCurrentWindowSize.y, i);
			D3DPERF_EndEvent();
		}
	}

	D3DPERF_BeginEvent(0xff0000, L"Main - Setting Shader Resources");
	if (!g_bExpandedImage || g_iLocation == 0)
	{
		for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
		{
			if (g_Resource[i].m_iTextureSlot >= 0)
			{
				// Bind the correct textures and buffer resources
				if (g_Resource[i].m_Type == Channels::ChannelType::E_Texture)
					g_pImmediateContext->PSSetShaderResources(g_Resource[i].m_iTextureSlot, 1, &g_Resource[i].m_pShaderResource);
				else if (g_Resource[i].m_Type == Channels::ChannelType::E_Buffer)
					g_pImmediateContext->PSSetShaderResources(g_Resource[i].m_iTextureSlot, 1, &g_Buffers[(int)g_Resource[i].m_iBufferIndex].m_pShaderResourceView);
			}
		}
	}
	else
	{
		for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
		{
			if (g_Resource[i].m_iTextureSlot >= 0)
			{
				if (g_Buffers[g_iLocation - 1].m_Res[i].m_Type == Channels::ChannelType::E_Texture)
					g_pImmediateContext->PSSetShaderResources(g_Resource[i].m_iTextureSlot, 1, &g_Buffers[g_iLocation - 1].m_Res[i].m_pShaderResource);
				else if (g_Buffers[g_iLocation - 1].m_Res[i].m_Type == Channels::ChannelType::E_Buffer)
					g_pImmediateContext->PSSetShaderResources(g_Resource[i].m_iTextureSlot, 1, &g_Buffers[g_Buffers[g_iLocation - 1].m_Res[i].m_iBufferIndex].m_pShaderResourceView);
			}
		}
	}
	D3DPERF_EndEvent();

	// Set the shaders
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

	if (!g_bExpandedImage)
		g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
	else
	{
		RECT rcClient;
		GetClientRect(g_hWnd, &rcClient);

		if (g_bNeedsResize)
		{
			Resize((float)(rcClient.right - rcClient.left), (float)(rcClient.bottom - rcClient.top));
			g_bNeedsResize = false;
		}

		if(g_iLocation != 0)
			g_pImmediateContext->PSSetShader(g_Buffers[g_iLocation - 1].m_pPixelShader, nullptr, 0);
		
		g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, g_pDepthStencilView);
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
	if (!g_bExpandedImage)
		g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, DirectX::Colors::MidnightBlue);
	else
		g_pImmediateContext->ClearRenderTargetView(g_pBackBufferRenderTargetView, DirectX::Colors::MidnightBlue);

	//
	// Update variables that change once per frame
	//
	CBChangesEveryFrame cb;
	cb.m_iFrame = g_iFrame++;
	if (g_bTrackMouse)
	{
		if (g_bMouseMoved && !g_bExpandedImage)
		{
			g_vMouse.x -= g_vImageOffset.x;
			g_vMouse.y -= g_vImageOffset.y;

			if (g_vMouse.x < 0)
				g_vMouse.x = 0;
			if (g_vMouse.y < 0)
				g_vMouse.y = 0;

			if (g_vMouse.x > g_vCurrentWindowSize.x - g_vPadding.x)
				g_vMouse.x = g_vCurrentWindowSize.x - g_vPadding.x;
			if (g_vMouse.y > g_vCurrentWindowSize.y - g_vPadding.y)
				g_vMouse.y = g_vCurrentWindowSize.y - g_vPadding.y;

			g_bMouseMoved = false;
		}
		cb.m_vMouse = g_vMouse;
	}
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
	cbNC.Resolution = DirectX::XMFLOAT4((float)g_vCurrentWindowSize.x, (float)g_vCurrentWindowSize.y, 1.0f / (float)g_vCurrentWindowSize.x, 1.0f / (float)g_vCurrentWindowSize.y);
	cbNC.ChannelResolution[0] = g_Resource[0].m_vChannelRes;
	cbNC.ChannelResolution[1] = g_Resource[1].m_vChannelRes;
	cbNC.ChannelResolution[2] = g_Resource[2].m_vChannelRes;
	cbNC.ChannelResolution[3] = g_Resource[3].m_vChannelRes;

	g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	int allocSize = MEMORY_ALIGNINT(g_iCustomizableBufferSize, 4 * sizeof(float));
	void* customizableBufferData = malloc(allocSize);

	for (int i = 0; i < g_vCustomizableBuffer.size(); ++i)
	{
		if (g_vCustomizableBuffer[i].isDataSet)
		{
			if (g_vCustomizableBuffer[i].type == D3D_SVT_FLOAT)
			{
				for (int j = 0; j < g_vCustomizableBuffer[i].size / sizeof(float); ++j)
					((float*)customizableBufferData)[g_vCustomizableBuffer[i].offset / sizeof(float) + j] = ((float*)g_vCustomizableBuffer[i].data)[j];
			}
			else if (g_vCustomizableBuffer[i].type == D3D_SVT_INT)
			{
				for (int j = 0; j < g_vCustomizableBuffer[i].size / sizeof(int); ++j)
					((int*)customizableBufferData)[g_vCustomizableBuffer[i].offset / sizeof(int) + j] = ((int*)g_vCustomizableBuffer[i].data)[j];
			}
			else
			{
				// Currently not supporting other types
				assert(g_vCustomizableBuffer[i].type == D3D_SVT_FLOAT || g_vCustomizableBuffer[i].type == D3D_SVT_INT);
			}
		}
	}
	g_pImmediateContext->UpdateSubresource(g_pCBCustomizable, 0, nullptr, customizableBufferData, 0, 0);

	free(customizableBufferData);
	customizableBufferData = nullptr;

	// Set constant buffers
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCBCustomizable);
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pCBChangesEveryFrame);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pCBNeverChanges);

	PIXSetMarker(PIX_COLOR_INDEX((byte)256), "Drawing Triangles");

	// Draw th triangles that make up the window
	D3DPERF_BeginEvent(0xff0000, L"Main - Render Main");
	g_pImmediateContext->DrawIndexed(ARRAYSIZE(g_Indicies), 0, 0);
	D3DPERF_EndEvent();

	ClearShaderResource(g_pImmediateContext, nullptr);
	D3DPERF_EndEvent();
}

//--------------------------------------------------------------------------------------
// Render ImGui
//--------------------------------------------------------------------------------------
void ImGuiRender()
{
	// Rendering
	ImGui::Render();
	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRenderTargetView, NULL);
	g_pImmediateContext->ClearRenderTargetView(g_pBackBufferRenderTargetView, (float*)&g_vClearColour);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	// Save Imgui state
	ImGui::SaveIniSettingsToDisk("imgui.ini");
	WriteIni(g_strProject.c_str(), g_strDefaultEditor.c_str(), g_vClearColour, g_vAppSize, g_dwShaderFlags, static_cast<int>(g_eDefaultEditor), g_bAutoReload, g_bUseRenderdoc);
	ULONG refs = 0;
	// revert fullscreen before closing
	g_pSwapChain->SetFullscreenState(FALSE, NULL);

	if (g_rdoc_api != nullptr)
	{
		g_rdoc_api->Shutdown();

		// Extra ones created by initializing Renderdoc
		g_pAlphaEnableBlendingState->Release();
		g_pAlphaDisableBlendingState->Release();

		g_pDepthStencilState->Release();

		g_pImmediateContext->Release();
	}

	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
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
		{
			_RPTF2(_CRT_WARN, "Main Sampler %i still has %i references.\n", i, refs);
		}
	}

	delete g_pTexturelib;
	g_pTexturelib = nullptr;

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

	SAFE_RELEASE(g_pAlphaEnableBlendingState);
	SAFE_RELEASE(g_pAlphaDisableBlendingState);
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

	// We don't want the ImGui windows to go outside the app window
	KeepImGuiWindowsInsideApp(g_vAppSize, g_vControlWindow, g_bResChanged);

	// Controls Window
	int buttonPress = 0;
	ControlWindow(
		g_pRenderTargetTexture,
		g_Resource,
		g_vClearColour,
		g_vClearColourFade,
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
		g_bNewProjLoaded,
		g_bDefaultEditorSelected,
		g_bFullWindow,
		bGoFullChange,
		g_bVsync,
		g_bGrabbingFrame,
		g_bUseRenderdoc,
		g_vControlWindow,
		g_vMainImageWindow,
		g_vCustomizableBuffer,
		&g_rdoc_api
	);

	if (bGoFullChange)
	{
		// Changing between fullscreen and windowed
		if (!g_bFullWindow)
			GoFullscreen();
		else
			GoWindowed();
	}

	if (buttonPress & MASK_RELOAD_SHADERS)
		ReloadShaders();
	if (buttonPress & MASK_RELOAD_TEXTURES)
		ReloadTextures();

	int iHovered = -1;

	// We don't want the ImGui windows to go outside the app window
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
		g_dwShaderFlags,
		g_strProject.c_str(), 
		g_fDPIscale, 
		g_piBufferUsed,
		g_iLocation,
		g_iPressIdentifier, 
		iHovered,
		g_bResChanged, 
		g_bNewProjLoaded,
		g_vWindowSize,
		g_vResourceWindow
	);

	// Box for ShaderToy image to be displayed in
	if ((((int)g_vCurrentWindowSize.x != (int)g_vWindowSize.x && (int)g_vCurrentWindowSize.y != (int)g_vWindowSize.y) || g_bResChanged) && !g_bExpandedImage)
	{
		if (g_vWindowSize.x > 0.0f && g_vWindowSize.y > 0.0f && g_vMainImageWindow.z > 0.0f && g_vMainImageWindow.w > 0.0f)
		{
			if (g_bResChanged)
				Resize(g_vMainImageWindow.z, g_vMainImageWindow.w);
			else
				Resize(g_vWindowSize.x, g_vWindowSize.y);

		}
	}

	// We don't want the ImGui windows to go outside the app window
	KeepImGuiWindowsInsideApp(g_vAppSize, g_vMainImageWindow, g_bResChanged);

	MainImageWindow(
		g_pShaderResourceView,
		g_Resource,
		g_Buffers,
		g_vWindowSize,
		g_vCurrentWindowSize,
		g_vImageOffset,
		g_vPadding,
		g_windowFlags,
		g_strProject.c_str(),
		g_strDefaultEditor,
		g_eDefaultEditor,
		g_piBufferUsed,
		g_iLocation, iHovered,
		g_fDPIscale,
		g_bTrackMouse, g_bExpandedImage,
		g_bResChanged, g_vMainImageWindow);

	// We don't want the ImGui windows to go outside the app window
	KeepImGuiWindowsInsideApp(g_vAppSize, g_vShaderErrorWindow, g_bResChanged);

	// Shader Compiler Errors
	ShaderErrorWindow(
		g_Buffers,
		g_vShaderErrorList,
		g_strProject,
		g_strDefaultEditor,
		g_eDefaultEditor,
		g_bResChanged,
		g_vShaderErrorWindow
	);

	if (MenuBar(
		g_pImmediateContext,
		g_pAlphaEnableBlendingState,
		g_pAlphaDisableBlendingState,
		g_dwShaderFlags))
		ReloadShaders();

	// Selection of default editor
	if (!g_bDefaultEditorSelected)
	{	
		ImVec2 size = ImVec2((float)(g_vAppSize.right + g_vPadding.x), (float)(g_vAppSize.bottom + g_vPadding.y));
		ImGui::SetNextWindowFocus();
		Barrier(size);
		ImGui::SetNextWindowFocus();
		DefaultEditorSelector(ImVec2(size.x / 2, size.y / 2), g_eDefaultEditor, g_strDefaultEditor, g_bDefaultEditorSelected);
	}

	if (g_bExpandedImage)
	{
		g_bTrackMouse = true;
	}

	g_bResChanged = false;
}

//--------------------------------------------------------------------------------------
// Reload the Pixel Shaders
//--------------------------------------------------------------------------------------
void ReloadShaders()
{
	HRESULT hr = S_OK;

	g_vClearColourFade = g_vClearColour;

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (g_Buffers[i].m_bIsActive)
			g_Buffers[i].ReloadShader(g_pd3dDevice, g_pVertexShader, g_dwShaderFlags, g_strProject.c_str(), i);
	}

	// Make room for characters
	std::wstring wStrProj(g_strProject.length(), L' ');
	// Copy string to wstring.
	std::copy(g_strProject.begin(), g_strProject.end(), wStrProj.begin());

	std::wstring path = std::wstring(PROJECT_PATH_W) + wStrProj;

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	g_vShaderErrorList.clear();
	hr = CompileShaderFromFile((path + std::wstring(L"/shaders/PixelShader.hlsl")).c_str(), "mainImage", "ps_5_0", &pPSBlob, g_dwShaderFlags, g_vShaderErrorList);
	if (FAILED(hr))
	{
		SetForegroundWindow(g_hWnd);
		// Trying without a pop-up box
		/*MessageBox(g_hWnd,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);*/
	}


	g_pPixelShader->Release();
	g_pPixelShader = nullptr;

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	if (FAILED(hr))
	{
		SetForegroundWindow(g_hWnd);
		MessageBox(g_hWnd,
			(LPCSTR)"Error creating pixel shader.", (LPCSTR)"Error", MB_OK);
		return;
	}

	if (FAILED(Reflection::D3D11ReflectionSetup(pPSBlob, &g_pPixelShaderReflection)))
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

	Reflection::D3D11ShaderReflectionAndPopulation(g_pPixelShaderReflection, g_vCustomizableBuffer, g_iCustomizableBufferSize, customizableBufferCopy);

	Reflection::D3D11ShaderReflection(g_pPixelShaderReflection, g_Resource, g_dwShaderFlags);

	D3D11_BUFFER_DESC bd = {};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = (g_iCustomizableBufferSize / 16 + 1) * 16;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;

	SAFE_RELEASE(g_pCBCustomizable);
	hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBCustomizable);

	// Set the shaders
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

	g_vClearColour = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

//--------------------------------------------------------------------------------------
// Reload the Texture Library
//--------------------------------------------------------------------------------------
void ReloadTextures()
{
	delete g_pTexturelib;
	g_pTexturelib = nullptr;

	g_pTexturelib = new TextureLib();
	g_pTexturelib->ParallelLoadDDSTextures(g_pd3dDevice, "textures/*");
	g_pTexturelib->m_bReload = true;
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
	hr = CompileShaderFromFile(((std::wstring(PROJECT_PATH_W) + g_strProjectW + std::wstring(L"/shaders/PixelShader.hlsl"))).c_str(), "mainImage", "ps_5_0", &pPSBlob, g_dwShaderFlags, g_vShaderErrorList);
	if (FAILED(hr))
	{
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			std::string msg = "Error Pixel Shader \"shaders/PixelShader.hlsl\" not found!";

			SetForegroundWindow(g_hWnd);
			MessageBox(g_hWnd,
				(LPCSTR)msg.c_str(), (LPCSTR)"Error", MB_OK);
		}

		SetForegroundWindow(g_hWnd);
		// Trying without a pop-up box
		/*MessageBox(g_hWnd,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);*/
	}

	// Create the pixel shader
	g_pPixelShader->Release();
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (g_Buffers[i].m_bIsActive)
			g_Buffers[i].Release(i);

		if (g_Resource[i].m_pSampler)
			g_Resource[i].m_pSampler->Release();
	}

	// Change capture location
	if (g_rdoc_api != nullptr)
		g_rdoc_api->SetCaptureFilePathTemplate((std::string(PROJECT_PATH) + g_strProject + std::string(WEASEL_CAPTURE)).c_str());

	InitResources(false);
	Resize(g_vWindowSize.x, g_vWindowSize.y);
	return hr;
}