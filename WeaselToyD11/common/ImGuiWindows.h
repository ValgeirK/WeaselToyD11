#pragma once

//#include "../lib/imgui.h"
//#include "../lib/imgui_impl_dx11.h"
//#include "../lib/imgui_impl_win32.h"

struct ImVec2;
struct ImVec4;
typedef int ImGuiWindowFlags;
struct ID3D11Texture2D;
struct Channel;
struct CustomizableBuffer;

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
	ID3D11Texture2D*,
	Resource*,
	ImVec4&,
	Buffer*,
	TextureLib*,
	ImGuiEnum::DefaultEditor&,
	ImGuiEnum::AspectRatio&,
	ImGuiEnum::Resolution&,
	DirectX::XMFLOAT4,
	std::string&,
	bool&, bool&,
	float&, float&,
	float, float,
	int, int&,
	bool&, bool&, bool&, bool,
	bool&,
	ImVec4&,
	ImVec4&,
	std::vector<CustomizableBuffer>&
);

void MainImageWindow(
	ID3D11ShaderResourceView*,   
	Resource*,
	Buffer*,
	ImVec2&,
	ImVec2&,
	ImVec2,
	ImGuiWindowFlags&,
	const char*,
	std::string,
	ImGuiEnum::DefaultEditor,
	int&,
	int,
	float,
	bool&,
	bool&,
	bool,
	ImVec4&
);

void ResourceWindow(
	ID3D11Device*,
	ID3D11DeviceContext*,
	ID3D11VertexShader*,
	Resource*,
	Buffer*,
	Channel*,
	TextureLib*,
	const char*,
	float,
	int&, int&, int&, 
	bool, bool&,
	ImVec2,
	ImVec4&
);

void ShaderErrorWindow(
	Buffer*, 
	std::vector<std::string>,
	std::string,
	std::string,
	ImGuiEnum::DefaultEditor, 
	bool, 
	ImVec4&
);

void DefaultEditorSelector(ImVec2, ImGuiEnum::DefaultEditor&, std::string&, bool&);

void Barrier(ImVec2);