#pragma once

struct ImVec2;
struct ImVec4;
struct ID3D11Texture2D;
struct Channel;
struct CustomizableBuffer;
struct ID3D11BlendState;

struct RENDERDOC_API_1_3_0;
typedef RENDERDOC_API_1_3_0 RENDERDOC_API_1_1_2;

typedef int ImGuiWindowFlags;
typedef unsigned long DWORD;

#include <vector>
#include <string.h>

#include "Buffer.h"
#include "type/Resource.h"

namespace ImGuiEnum
{
	enum DefaultEditor
	{
		E_DEFAULT,
		E_NOTEPAD,
		E_NOTEPADPP,
		E_OTHER
	};

	enum AspectRatio
	{
		E_NONE,
		E_4_3,
		E_16_9,
		E_16_10,
		E_32_9
	};

	enum Resolution
	{
		E_CUSTOM,
		E_800x600,
		E_1024x600,
		E_1024x768,
		E_1152x864,
		E_1280x720,
		E_1280x768,
		E_1280x800,
		E_1280x1024,
		E_1360x768,
		E_1366x768,
		E_1440x900,
		E_1536x864,
		E_1600x900,
		E_1680x1050,
		E_1920x1080,
		E_1920x1200,
		E_2560x1080,
		E_2560x1440,
		E_3440x1440,
		E_3840x2160
	};
}


void ControlWindow(
	ID3D11Texture2D*			pRenderTargetTexture,
	Resource*					pResource,
	ImVec4&						clearColour,
	ImVec4&						clearColourFade,
	Buffer*						pBuffer,
	TextureLib*					pTextureLib,
	ImGuiEnum::DefaultEditor&	defaultEditor,
	ImGuiEnum::AspectRatio&		aspectRatio,
	ImGuiEnum::Resolution&		resolution,
	DirectX::XMFLOAT4			vMouse,
	std::string& strProj,
	bool& bPause, bool& bAutoReload,
	float& fGameT, float& playSpeed,
	float fDeltaT, float fScaling,
	int iFrame, int iLocation, int& buttonPress,
	bool& bResChanged, bool& bNewProj,
	bool& bDefaultEditorSet, bool bIsFullwindow,
	bool& bGoFullscreenChange, bool& bVsync,
	bool& bGrabbing, bool& bRenderdoc,
	ImVec4&						vWindowInfo,
	ImVec4&						vMainWindowInfo,
	std::vector<CustomizableBuffer>& vCustomizableBuffer,
	RENDERDOC_API_1_1_2**		rdoc_api
);

void MainImageWindow(
	ID3D11ShaderResourceView*	pShaderResourceView,
	Resource*					pResource,
	Buffer*						pBuffer,
	ImVec2&						vWindowSize,
	ImVec2&						vCurrentWindowSize,
	ImVec2&						vMouse,
	ImVec2						vPadding,
	ImGuiWindowFlags&			windowFlags,
	const char*					strProj,
	std::string					strDefaultEditor,
	ImGuiEnum::DefaultEditor	defaultEditor,
	int*						piIsUsed,
	int&						iLocation, 
	int							iHovered,
	float						fScaling,
	bool&						bTrackMouse,
	bool&						bExpandedImage,
	bool						bResChanged,
	ImVec4&						vWindowInfo
);

void ResourceWindow(
	ID3D11Device*				pDevice,
	ID3D11DeviceContext*		pContext,
	ID3D11VertexShader*			pVertexShader,
	Resource*					pResource,
	Buffer*						pBuffer,
	Channel*					pChannel,
	TextureLib*					pTextureLib,
	DWORD						dwShaderFlags,
	const char*					strProj,
	float						scaling,
	int*						piBufferUsed,
	int& iLocation, int& iPressIdentifier,
	int&						iHovered,
	bool bResChanged, bool& bProjChange,
	ImVec2						vSize,
	ImVec4&						vWindowInfo
);

void ShaderErrorWindow(
	Buffer*						pBuffer,
	std::vector<std::string>	mainErrors,
	std::string					strProj,
	std::string					strDefaultEditor,
	ImGuiEnum::DefaultEditor	defaultEditor,
	bool						bResChanged,
	ImVec4&						vWindowInfo
);

void DefaultEditorSelector(
	ImVec2						pos,
	ImGuiEnum::DefaultEditor&	defaultEditor,
	std::string&				strDefaultEditor,
	bool&						defaultEditorSelected
);

void Barrier(ImVec2);

bool MenuBar(
	ID3D11DeviceContext*		pContext,
	ID3D11BlendState*			pBlendStateEnabled,
	ID3D11BlendState*			pBlendStateDisabled,
	DWORD&						dwShaderFlags
);

float GetImGuiDeltaTime();