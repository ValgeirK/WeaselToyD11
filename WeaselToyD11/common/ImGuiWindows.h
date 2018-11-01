#pragma once

//#include "../lib/imgui.h"
//#include "../lib/imgui_impl_dx11.h"
//#include "../lib/imgui_impl_win32.h"

struct ImVec2;
struct ImVec4;
typedef int ImGuiWindowFlags;
struct ID3D11Texture2D;

#include "Buffer.h"
#include "type/Resource.h"

namespace ImGuiEnum
{
	enum DefaultEditor
	{
		E_NOTEPAD,
		E_NOTEPADPP
	};
}


void ControlWindow(bool&, float&, float, ImVec4, int&, ImGuiEnum::DefaultEditor&);

void ConstantBufferInfoWindow(
	ID3D11Texture2D*, 
	Resource*,
	Buffer*,
	int , bool,
	float , float,
	DirectX::XMFLOAT4
);

void MainImageWindow(
	ID3D11ShaderResourceView*,   
	Resource*,
	Buffer*,
	ImVec2&,
	ImVec2&,
	ImGuiWindowFlags&,
	int,
	bool&
);

void ViewToggleWindow(int&, Buffer*, int);

void ResourceWindow(
	ID3D11DeviceContext*,
	Resource*,
	Buffer*,
	TextureLib*,
	int&, int&, int&
);

void ShaderErrorWindow(Buffer*, ImGuiEnum::DefaultEditor);