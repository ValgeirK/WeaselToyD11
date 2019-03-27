#include "ImGuiWindows.h"

#include "../lib/imgui.h"
#include "../lib/imgui_internal.h"
#include "../lib/imgui_impl_dx11.h"
#include "../lib/imgui_impl_win32.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <ctime>
#include <string.h>
#include <process.h>
#include <algorithm>
#include <math.h>

#include "Loader.h"
#include "Textures.h"
#include "HelperFunction.h"
#include "RegisterHelper.h"
#include "type/Resource.h"
#include "type/Channel.h"
#include "type/ConstantBuffer.h"
#include "type/HashDefines.h"

#include "../ImGuiFileDialog.h"

#include "../renderdoc_app.h"

//--------------------------------------------------------------------------------------
// Methods to decrease code
//--------------------------------------------------------------------------------------

void BufferSwitchLookup(std::string& strName, int index, std::string post = "")
{
	assert(index >= 0 && index < MAX_RESORCESCHANNELS);

	static std::string bufferText[]
	{
		"Buffer A",
		"Buffer B",
		"Buffer C",
		"Buffer D"
	};

	assert(sizeof(bufferText) / sizeof(std::string) == MAX_RESORCESCHANNELS);

	strName = bufferText[index];

	strName += post;
}

void TextureSwitchLookup(std::string& strName, int index, std::string post = "")
{
	assert(index >= 0 && index < MAX_RESORCESCHANNELS);

	static std::string textureText[]
	{
		"Texture A",
		"Texture B",
		"Texture C",
		"Texture D"
	};

	assert(sizeof(textureText) / sizeof(std::string) == MAX_RESORCESCHANNELS);

	strName = textureText[index];

	strName += post;
}

void ChannelPathSwitchLookup(std::string& path, const char* strProj, int index)
{
	assert(index >= 0 && index < MAX_RESORCESCHANNELS);

	std::string channelPath[]
	{
		path + std::string(strProj) + std::string("\\channels\\channelsA.txt"),
		path + std::string(strProj) + std::string("\\channels\\channelsB.txt"),
		path + std::string(strProj) + std::string("\\channels\\channelsC.txt"),
		path + std::string(strProj) + std::string("\\channels\\channelsD.txt")
	};

	assert(sizeof(channelPath) / sizeof(std::string) == MAX_RESORCESCHANNELS);

	path = channelPath[index];
}

void LineText(Resource* pResource, Buffer* pBuffer, int index, float scale)
{
	std::string strName = "";
	float x = 0.0f, y = 0.0f;

	switch (pResource[index].m_Type)
	{
	case Channels::ChannelType::E_Texture:
	{
		TextureSwitchLookup(strName, index, ":");

		x = pResource[index].m_vChannelRes.x;
		y = pResource[index].m_vChannelRes.y;

		break;
	}
	case Channels::ChannelType::E_Buffer:
	{
		int id = pResource[index].m_iBufferIndex;
		
		BufferSwitchLookup(strName, id, ":");

		x = pBuffer[pResource[index].m_iBufferIndex].m_BufferResolution.x;
		y = pBuffer[pResource[index].m_iBufferIndex].m_BufferResolution.y;

		break;
	}
	}

	ImGui::Text(strName.c_str());
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f  * scale);
	ImGui::PushItemWidth(100.0f * scale);
	ImGui::Text("%u x %u", (int)x, (int)y);
	ImGui::PopItemWidth();
}

void BufferLineText(Buffer* pBuffer, int iResourceIndex, int iChannelIndex, float scale)
{
	std::string strName = "";

	switch (pBuffer[iResourceIndex].m_Res[iChannelIndex].m_Type)
	{
	case Channels::ChannelType::E_Texture:
	{
		TextureSwitchLookup(strName, iChannelIndex, ":");
		break;
	}
	case Channels::ChannelType::E_Buffer:
	{
		int id = pBuffer[iResourceIndex].m_Res[iChannelIndex].m_iBufferIndex;

		BufferSwitchLookup(strName, id, ":");
		break;
	}
	default:
		return;
		break;
	}


	ImGui::Text(strName.c_str());
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 110.0f * scale);
	ImGui::PushItemWidth(110.0f * scale);
	ImGui::Text("%u x %u", (int)pBuffer[iResourceIndex].m_Res[iChannelIndex].m_vChannelRes.x, (int)pBuffer[iResourceIndex].m_Res[iChannelIndex].m_vChannelRes.y);
	ImGui::PopItemWidth();
}

void RecursiveLineText(const char* strName, Buffer* pBuffer, int index, float scale)
{
	if (pBuffer[index].m_bIsActive && ImGui::TreeNode(strName))
	{	
		for(int i = 0; i < 4; ++i)
			BufferLineText(pBuffer, index, i, scale);

		ImGui::TreePop();
	}
}

bool ButtonLine(const char* strName, ImVec2 vWindowSize, float fItemSpacing, float& fButtonWidth, float& fPos, float fScale, float fButtonCount)
{
	fButtonWidth = (vWindowSize.x) / fButtonCount - 10.0f;
	fPos += fButtonWidth + fItemSpacing;
	ImGui::SameLine(vWindowSize.x - fPos);

	bool res = ImGui::Button(strName, ImVec2(fButtonWidth, 20.0f * fScale));

	fButtonWidth = ImGui::GetItemRectSize().x;

	return res;
}

void ViewButtonLine(const char* strName, int& iLocation, ImVec2 vWindowSize, float fItemSpacing, float& fButtonWidth, float& fPos, int index, bool active, int hovered, float scale, float fButtonCount)
{
	bool useColour = false;
	ImVec4 colour = ImVec4();

	if (iLocation == index)
	{
		// Active tab
		useColour = true;
		colour = (ImVec4)ImColor(1.0f, 0.5f, 0.5f, 1.0f);
	}
	else
	if (!active)
	{
		// Unused tabs
		useColour = true;
		colour = (ImVec4)ImColor(0.5f, 0.5f, 0.5f, 1.0f);
	}
	else
	if (hovered == index)
	{
		// Hovered tab
		useColour = true;
		colour = (ImVec4)ImColor(0.5f, 0.8f, 0.5f, 1.0f);
	}

	if (useColour)
		ImGui::PushStyleColor(ImGuiCol_Button, colour);

	if (ButtonLine(strName, vWindowSize, fItemSpacing, fButtonWidth, fPos, scale, fButtonCount) && active)
		iLocation = index;

	if(useColour)
		ImGui::PopStyleColor();
}

void ErrorLine(std::string strName, std::string strProj, std::string strDefaultEditor, ImGuiEnum::DefaultEditor defaultEditor)
{
	if (ImGui::Selectable(strName.c_str()))
	{ 
		std::size_t indexStart = strName.find(".hlsl");
		std::size_t indexEnd = strName.find("):");
		std::string lineInfo = strName.substr(indexStart + 6, indexEnd - indexStart - 6);

		// First string will be the line number where the error is
		std::vector<std::string> ints;
		SplitString(lineInfo, ints, ',');

		// The path to the file containing the error
		std::string shaderName = strName.substr(0, indexStart + 5);

		if (defaultEditor == ImGuiEnum::DefaultEditor::E_DEFAULT)
			system((std::string("start ") + std::string(shaderName)).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_NOTEPAD)
			system((std::string("start notepad ") + std::string(shaderName)).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_NOTEPADPP)
			system((std::string("start notepad++ ") + std::string(shaderName) + std::string(" -n") + ints[0]).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_OTHER)
			system((std::string("start ") + strDefaultEditor + std::string(" ") + std::string(shaderName)).c_str());
	}
}

void OverlayLabel(const char* strName)
{
	ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(1.0f, 1.0f, 1.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(1.0f, 1.0f, 1.0f, 0.0f));
	ImGui::Button(strName);
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
}

void ConstantBufferInfoWindow(ID3D11Texture2D* pRenderTargetTexture,
	Resource* pResource,
	Buffer* pBuffer,
	float scaling,
	int iFrame, bool bPause,
	float fGameT, float fDeltaT,
	DirectX::XMFLOAT4 vMouse)
{
	ImGui::Separator();
	ImGui::Separator();

	ImGui::Text("Never Changes");

	ImGui::Separator();

	ImGui::Text("Resolution:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	D3D11_TEXTURE2D_DESC desc = {};
	pRenderTargetTexture->GetDesc(&desc);
	ImGui::PushItemWidth(100.0f * scaling);
	ImGui::Text("%u x %u", (int)desc.Width, (int)desc.Height);
	ImGui::PopItemWidth();

	ImGui::Spacing();
	ImGui::Text("ChanelResolution");

	for (int i = 0; i < 4; ++i)
		LineText(pResource, pBuffer, i, scaling);

	ImGui::Spacing();

	for (int i = 0; i < 4; ++i)
	{
		std::string label = "";
		BufferSwitchLookup(label, i, " Resource");
		RecursiveLineText(label.c_str(), pBuffer, i, scaling);
	}

	ImGui::Separator();
	ImGui::Separator();

	ImGui::Text("Changes Every Frame");

	ImGui::Separator();

	ImGui::Text("Frame:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::PushItemWidth(100.0f * scaling);
	ImGui::Text("%u", iFrame);
	ImGui::PopItemWidth();

	ImGui::Text("Time:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::PushItemWidth(100.0f * scaling);
	ImGui::Text("%.3f sec", !bPause ? fGameT + fDeltaT : fGameT);
	ImGui::PopItemWidth();

	ImGui::Text("Delta Time:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::PushItemWidth(100.0f * scaling);
	ImGui::Text("%.5f ms", fDeltaT * 1000.0f * scaling);
	ImGui::PopItemWidth();

	ImGui::Text("Mouse:");
	ImGui::PushItemWidth(100.0f * scaling);
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::Text("x:%.2f", vMouse.x);
	ImGui::NewLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::Text("y:%.2f", vMouse.y);
	ImGui::NewLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::Text("LC:%u", (int)vMouse.z);
	ImGui::NewLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::Text("RC:%u", (int)vMouse.w);
	ImGui::PopItemWidth();

	// Get time
	time_t now = time(0);
	struct tm ltm;
	localtime_s(&ltm, &now);
	int hour = ltm.tm_hour;
	int min = ltm.tm_min;
	int sec = ltm.tm_sec;

	ImGui::Text("Date:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::PushItemWidth(100.0f * scaling);
	ImGui::Text("%u/%u/%u", ltm.tm_mday, 1 + ltm.tm_mon, 1900 + ltm.tm_year);
	ImGui::NewLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f * scaling);
	ImGui::Text("%u:%u:%u", hour, min, sec);
	ImGui::PopItemWidth();

	ImGui::Separator();
	ImGui::Separator();
}

void MainResolution(ID3D11Texture2D* pRenderTargetTexture, ImGuiEnum::AspectRatio& aspectRatio, ImGuiEnum::Resolution& resolution, ImVec4& vMainWindowInfo, bool& resChanged)
{
	// Get resolution of main image
	D3D11_TEXTURE2D_DESC desc = {};
	pRenderTargetTexture->GetDesc(&desc);

	float itemWidth = 150.0f;
	ImVec2 size = ImGui::GetWindowSize();
	const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;

	ImGui::PushItemWidth(150.0f);
	ImGui::Text("Resolution:");
	ImGui::PopItemWidth();

	int xRes = (int)desc.Width;
	ImGui::SameLine(size.x / 2.0f - 5.0f);
	ImGui::PushItemWidth(size.x / 5.0f);
	ImGui::InputInt("x", &xRes, 0, 100, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::PopItemWidth();

	int yRes = desc.Height;
	ImGui::SameLine(size.x - size.x / 5.0f - 21.0f);
	ImGui::PushItemWidth(size.x / 4.0f);
	ImGui::InputInt("y", &yRes, 0, 100, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::PopItemWidth();

	int xPadding = (int)(vMainWindowInfo.z - desc.Width);
	int yPadding = (int)(vMainWindowInfo.w - desc.Height);

	if (xRes != (int)desc.Width || yRes != (int)desc.Height)
		resChanged = true;

	// Aspect ratio
	const char* aspectItems[] = { "Custom", "4:3", "16:9", "16:10", "32:9" };

	int item_current = 0;
	float aspect = roundf(((float)xRes / (float)yRes) * 100.0f) / 100.0f;
	if (aspect == roundf((4.0f / 3.0f) * 100.0f) / 100.0f)
		item_current = 1;
	if (aspect == roundf((16.0f / 9.0f) * 100.0f) / 100.0f)
		item_current = 2;
	if (aspect == roundf((16.0f / 10.0f) * 100.0f) / 100.0f)
		item_current = 3;
	if (aspect == roundf((32.0f / 9.0f) * 100.0f) / 100.0f)
		item_current = 4;

	ImGui::PushItemWidth(0.0f);
	ImGui::Text("Aspect Ratio:");
	ImGui::PopItemWidth();
	ImGui::SameLine(size.x / 2.0f - 5.0f);
	ImGui::PushItemWidth(size.x / 2.0f);
	if (ImGui::Combo("Aspect Ratio", &item_current, aspectItems, IM_ARRAYSIZE(aspectItems)))
	{
		if (item_current != 0)
		{
			switch (item_current)
			{
			case 1:
				xRes = (int)((float)yRes * 4.0f / 3.0f);
				break;
			case 2:
				xRes = (int)((float)yRes * 16.0f / 9.0f);
				break;
			case 3:
				xRes = (int)((float)yRes * 16.0f / 10.0f);
				break;
			case 4:
				xRes = (int)((float)yRes * 32.0f / 9.0f);
				break;
			}
			resChanged = true;
		}
	}
	aspectRatio = static_cast<ImGuiEnum::AspectRatio>(item_current);
	ImGui::PopItemWidth();

	// Popular resolutions
	const char* resolutionItems[] = { 
		"Custom",
		"800x600",
		"1024x600",
		"1024x768",
		"1152x864",
		"1280x720",
		"1280x768",
		"1280x800",
		"1280x1024",
		"1360x768",
		"1366x768",
		"1440x900",
		"1536x864",
		"1600x900",
		"1680x1050",
		"1920x1080",
		"1920x1200",
		"2560x1080",
		"2560x1440",
		"3440x1440",
		"3840x2160"
	};

	// Set the current selected combo
	item_current = 0; // Custom
	if (xRes == 800 && yRes == 600)
		item_current = 1; // 800x600
	else if (xRes == 1024 && yRes == 600)
		item_current = 2; // 1024x600
	else if (xRes == 1024 && yRes == 768)
		item_current = 3; // 1024x768
	else if (xRes == 1152&& yRes == 864)
		item_current = 4; // 1152x864
	else if (xRes == 1280&& yRes == 720)
		item_current = 5; // 1280x720
	else if (xRes == 1280 && yRes == 768)
		item_current = 6; // 1280x768
	else if (xRes == 1280 && yRes == 800)
		item_current = 7; // 1280x800
	else if (xRes == 1280 && yRes == 1024)
		item_current = 8; // 1080x1024
	else if (xRes == 1360 && yRes == 768)
		item_current = 9; // 1360x768
	else if (xRes == 1366 && yRes == 768)
		item_current = 10; // 1366x768
	else if (xRes == 1440 && yRes == 900)
		item_current = 11; // 1440x900
	else if (xRes == 1536 && yRes == 864)
		item_current = 12; // 1536x864
	else if (xRes == 1600 && yRes == 900)
		item_current = 13; // 1600x900
	else if (xRes == 1680 && yRes == 1050)
		item_current = 14; // 1680x1050
	else if (xRes == 1920 && yRes == 1080)
		item_current = 15; // 1920x1080
	else if (xRes == 1920 && yRes == 1200)
		item_current = 16; // 1920x1200
	else if (xRes == 2560 && yRes == 1080)
		item_current = 17; // 2560x1080
	else if (xRes == 2560 && yRes == 1440)
		item_current = 18; // 2560x1440
	else if (xRes == 3440 && yRes == 1440)
		item_current = 19; // 3440x1440
	else if (xRes == 3840 && yRes == 2160)
		item_current = 20; // 3840x2160



	ImGui::PushItemWidth(0.0f);
	ImGui::Text("Set Resolutions:");
	ImGui::PopItemWidth();
	ImGui::SameLine(size.x / 2.0f - 5.0f);
	ImGui::PushItemWidth(size.x / 2.0f);
	if (ImGui::Combo("Set Resolutions", &item_current, resolutionItems, IM_ARRAYSIZE(resolutionItems)))
	{
		if (item_current != 0)
		{
			switch (item_current)
			{
			case 1:
				xRes = 800;
				yRes = 600;
				break;
			case 2:
				xRes = 1024;
				yRes = 600;
				break;
			case 3:
				xRes = 1024;
				yRes = 768;
				break;
			case 4:
				xRes = 1152;
				yRes = 864;
				break;
			case 5:
				xRes = 1280;
				yRes = 720;
				break;
			case 6:
				xRes = 1280;
				yRes = 768;
				break;
			case 7:
				xRes = 1280;
				yRes = 800;
				break;
			case 8:
				xRes = 1280;
				yRes = 1024;
				break;
			case 9:
				xRes = 1360;
				yRes = 768;
				break;
			case 10:
				xRes = 1366;
				yRes = 768;
				break;
			case 11:
				xRes = 1440;
				yRes = 900;
				break;
			case 12:
				xRes = 1536;
				yRes = 864;
				break;
			case 13:
				xRes = 1600;
				yRes = 900;
				break;
			case 14:
				xRes = 1680;
				yRes = 1050;
				break;
			case 15:
				xRes = 1920;
				yRes = 1080;
				break;
			case 16:
				xRes = 1920;
				yRes = 1200;
				break;
			case 17:
				xRes = 2560;
				yRes = 1080;
				break;
			case 18:
				xRes = 2560;
				yRes = 1440;
				break;
			case 19:
				xRes = 3440;
				yRes = 1440;
				break;
			case 20:
				xRes = 3840;
				yRes = 2160;
				break;
			}

			resChanged = true;
		}
	}

	vMainWindowInfo.z = (float)(xRes + xPadding);
	vMainWindowInfo.w = (float)(yRes + yPadding);

	resolution = static_cast<ImGuiEnum::Resolution>(item_current);
	ImGui::PopItemWidth();
}

void CustomizableArea(
	std::vector<CustomizableBuffer>& vCustomizableBuffer
)
{
	for (int i = 0; i < vCustomizableBuffer.size(); ++i)
	{
		// We can't have a case below for both sizes
		assert(sizeof(int) == sizeof(float));

		switch (vCustomizableBuffer[i].m_iSize)
		{
			case sizeof(int) :
//			case sizeof(float) :
			if (vCustomizableBuffer[i].m_iType == 3) // FLOAT
			{
				if (strcmp(vCustomizableBuffer[i].m_strCommand, "slider") == 0)
					ImGui::SliderFloat(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData, vCustomizableBuffer[i].m_fMin, vCustomizableBuffer[i].m_fMax);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "input") == 0)
					ImGui::InputFloat(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData, vCustomizableBuffer[i].m_fStep);
			}
			else if (vCustomizableBuffer[i].m_iType == 2) // INT
			{
				if (strcmp(vCustomizableBuffer[i].m_strCommand, "slider") == 0)
					ImGui::SliderInt(vCustomizableBuffer[i].m_strVariable, (int*)vCustomizableBuffer[i].m_pData, (int)vCustomizableBuffer[i].m_fMin, (int)vCustomizableBuffer[i].m_fMax);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "input") == 0)
					ImGui::InputInt(vCustomizableBuffer[i].m_strVariable, (int*)vCustomizableBuffer[i].m_pData, (int)vCustomizableBuffer[i].m_fStep);
			}
			break;
		case 8:
			if (vCustomizableBuffer[i].m_iType == 3) // FLOAT
			{
				if (strcmp(vCustomizableBuffer[i].m_strCommand, "slider") == 0)
					ImGui::SliderFloat2(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData, vCustomizableBuffer[i].m_fMin, vCustomizableBuffer[i].m_fMax);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "input") == 0)
					ImGui::InputFloat2(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData, 3);
			}
			else if (vCustomizableBuffer[i].m_iType == 2) // INT
			{
				if (strcmp(vCustomizableBuffer[i].m_strCommand, "slider") == 0)
					ImGui::SliderInt2(vCustomizableBuffer[i].m_strVariable, (int*)vCustomizableBuffer[i].m_pData, (int)vCustomizableBuffer[i].m_fMin, (int)vCustomizableBuffer[i].m_fMax);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "input") == 0)
					ImGui::InputInt2(vCustomizableBuffer[i].m_strVariable, (int*)vCustomizableBuffer[i].m_pData, 3);
			}
			break;
		case 12:
			if (vCustomizableBuffer[i].m_iType == 3) // FLOAT
			{
				if (strcmp(vCustomizableBuffer[i].m_strCommand, "slider") == 0)
					ImGui::SliderFloat3(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData, vCustomizableBuffer[i].m_fMin, vCustomizableBuffer[i].m_fMax);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "input") == 0)
					ImGui::InputFloat3(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData, 3);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "colorEdit") == 0
					|| strcmp(vCustomizableBuffer[i].m_strCommand, "colourEdit") == 0)
					ImGui::ColorEdit3(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData);
			}
			else if (vCustomizableBuffer[i].m_iType == 2) // INT
			{
				if (strcmp(vCustomizableBuffer[i].m_strCommand, "slider") == 0)
					ImGui::SliderInt3(vCustomizableBuffer[i].m_strVariable, (int*)vCustomizableBuffer[i].m_pData, (int)vCustomizableBuffer[i].m_fMin, (int)vCustomizableBuffer[i].m_fMax);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "input") == 0)
					ImGui::InputInt3(vCustomizableBuffer[i].m_strVariable, (int*)vCustomizableBuffer[i].m_pData, 3);
			}
			break;
		case 16:
			if (vCustomizableBuffer[i].m_iType == 3) // FLOAT
			{
				if (strcmp(vCustomizableBuffer[i].m_strCommand, "slider") == 0)
					ImGui::SliderFloat4(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData, vCustomizableBuffer[i].m_fMin, vCustomizableBuffer[i].m_fMax);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "input") == 0)
					ImGui::InputFloat4(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData, 3);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "colorEdit") == 0
					|| strcmp(vCustomizableBuffer[i].m_strCommand, "colourEdit") == 0)
					ImGui::ColorEdit4(vCustomizableBuffer[i].m_strVariable, (float*)vCustomizableBuffer[i].m_pData);
			}
			else if (vCustomizableBuffer[i].m_iType == 2) // INT
			{
				if (strcmp(vCustomizableBuffer[i].m_strCommand, "slider") == 0)
					ImGui::SliderInt4(vCustomizableBuffer[i].m_strVariable, (int*)vCustomizableBuffer[i].m_pData, (int)vCustomizableBuffer[i].m_fMin, (int)vCustomizableBuffer[i].m_fMax);
				else if (strcmp(vCustomizableBuffer[i].m_strCommand, "input") == 0)
					ImGui::InputInt4(vCustomizableBuffer[i].m_strVariable, (int*)vCustomizableBuffer[i].m_pData, 3);
			}
			break;
		}
		//ImGui::ColorEdit3
	}
}

//--------------------------------------------------------------------------------------
// Windows
//--------------------------------------------------------------------------------------

void ControlWindow(
	ID3D11Texture2D* pRenderTargetTexture,
	Resource* pResource,
	ImVec4& clearColour,
	ImVec4& clearColourFade,
	Buffer* pBuffer,
	TextureLib* pTextureLib,
	ImGuiEnum::DefaultEditor& defaultEditor,
	ImGuiEnum::AspectRatio& aspectRatio,
	ImGuiEnum::Resolution& resolution,
	DirectX::XMFLOAT4 vMouse,
	std::string& strProj,
	bool& bPause, bool& bAutoReload,
	float& fGameT, float& playSpeed, 
	float fDeltaT, float fScaling,
	int iFrame, int iLocation, int& buttonPress,
	bool& bResChanged, bool& bNewProj,
	bool& bDefaultEditorSet, bool bIsFullwindow,
	bool& bGoFullscreenChange, bool& bVsync, 
	bool& bGrabbing, bool& bRenderdoc,
	ImVec4& vWindowInfo,
	ImVec4& vMainWindowInfo,
	std::vector<CustomizableBuffer>& vCustomizableBuffer,
	RENDERDOC_API_1_1_2** rdoc_api
)
{
	ImGui::Begin("Control Panel");

	if (bResChanged)
	{
		ImGui::SetWindowPos(ImVec2(vWindowInfo.x, vWindowInfo.y));
		ImGui::SetWindowSize(ImVec2(vWindowInfo.z, vWindowInfo.w));
	}
	else
	{
		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();
		vWindowInfo.x = pos.x;
		vWindowInfo.y = pos.y;
		vWindowInfo.z = size.x;
		vWindowInfo.w = size.y;
	}

	const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
	ImVec2 windowSize = ImGui::GetWindowSize();

	float pos = 0.0f;
	float buttonWidth = 150.0f;
	
	std::string basePath = std::string(PROJECT_PATH_DOUBLE_SLASH);
	std::string newPath = basePath + std::string(strProj.c_str());

	static bool copyClicked = false;
	static bool loadClicked = false;
	static bool newClicked = false;

	ImGui::Text("Current Project: %s", strProj.c_str());
	ImGui::Separator();

	if (ButtonLine("Load", windowSize, itemSpacing, buttonWidth, pos, fScaling, 3.0f))
		loadClicked = true;
	if (ButtonLine("Copy Project", windowSize, itemSpacing, buttonWidth, pos, fScaling, 3.0f))
		copyClicked = true;
	if (ButtonLine("New", windowSize, itemSpacing, buttonWidth, pos, fScaling, 3.0f))
		newClicked = true;

	if (copyClicked)
	{
		windowSize = ImGui::GetWindowSize();

		pos = 0.0f;
		buttonWidth = 150.0f;
		// Save this project
		ImGui::Begin("Save..");
		static char newStrProj[20] = "";
		ImGui::InputText("Project name", newStrProj, 20);

		ImGui::Separator();
		if (ButtonLine("Cancel", windowSize, itemSpacing, buttonWidth, pos, fScaling, 2.0f))
			copyClicked = false;
		if (ButtonLine("Save", windowSize, itemSpacing, buttonWidth, pos, fScaling, 2.0f))
		{
			if (strcmp(newStrProj, "") != 0)
			{
				std::string oldPath = basePath + std::string(newStrProj);
				system((std::string("md \"") + oldPath + std::string("\\channels\"")).c_str());
				system((std::string("md \"") + oldPath + std::string("\\shaders\"")).c_str());
				system((std::string("xcopy ") + std::string(".\\channels \"") + oldPath + std::string("\\channels") + std::string("\" /i /E")).c_str());
				system((std::string("xcopy ") + std::string(".\\shaders \"") + oldPath + std::string("\\shaders") + std::string("\" /i /E")).c_str());
				copyClicked = false;
			}
		}
		ImGui::End();
	}

	if (newClicked)
	{
		windowSize = ImGui::GetWindowSize();

		pos = 0.0f;
		buttonWidth = 150.0f;

		ImGui::Begin("New..");
		static char newStrProj[20] = "";
		ImGui::InputText("Project name", newStrProj, 20);

		ImGui::Separator();
		if (ButtonLine("Cancel", windowSize, itemSpacing, buttonWidth, pos, fScaling, 2.0f))
			newClicked = false;
		if (ButtonLine("New", windowSize, itemSpacing, buttonWidth, pos, fScaling, 2.0f))
		{
			strProj = std::string(newStrProj);
			newPath = basePath + std::string(newStrProj);

			// Create new directory for new project, with simple shader present
			system((std::string("md \"") + newPath + std::string("\\channels\"")).c_str());
			system((std::string("md \"") + newPath + std::string("\\shaders\"")).c_str());
			system((std::string("xcopy ") + std::string(".\\channels \"") + newPath + std::string("\\channels") + std::string("\" /i /E")).c_str());
			system((std::string("xcopy ") + std::string(".\\shaders \"") + newPath + std::string("\\shaders") + std::string("\" /i /E")).c_str());
			bNewProj = true;
			newClicked = false;
		}
		ImGui::End();
	}

	std::string defProjPath = PROJECT_PATH;
	if (loadClicked)
	{
		if (ImGuiFileDialog::Instance(pTextureLib)->FileDialog("Choose File", "d", defProjPath.c_str(), ""))
		{
			if (ImGuiFileDialog::Instance(pTextureLib)->IsOk == true)
			{
				strProj = ImGuiFileDialog::Instance(pTextureLib)->GetSelectedFile();
				if(strProj != "")
					bNewProj = true;
			}

			ImGuiFileDialog::Instance(pTextureLib)->Clear();

			if (strProj != "")
				loadClicked = false;
		}
	}

	ImGui::Spacing();

	if (ImGui::Button("Stop", ImVec2(windowSize.x / 4.0f, 25.0f)))
	{
		bPause = true;
		fGameT = 0.0f;
	}
	ImGui::SameLine();
	const char* playButtonLabel = "Pause";
	if (bPause)
		playButtonLabel = "Play";

	if (ImGui::Button(playButtonLabel, ImVec2(windowSize.x / 4.0f, 25.0f)))
		bPause = !bPause;

	const float ItemSpacing = ImGui::GetStyle().ItemSpacing.x;

	if (ImGui::Button("Restart", ImVec2(windowSize.x / 2.0f + ItemSpacing, 25.0f)))
	{
		fGameT = 0.0f;
		buttonPress |= MASK_RELOAD_SHADERS;
	}

	ImGui::SameLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - windowSize.x / 3.0f);
	ImGui::PushItemWidth(windowSize.x / 3.0f);
	ImGui::Text("Play Speed:");
	ImGui::PopItemWidth();

	if (ImGui::Button("Reload Shaders", ImVec2(windowSize.x / 2.0f + ItemSpacing, 25.0f)))
	{
		buttonPress |= MASK_RELOAD_SHADERS;
	}

	ImGui::SameLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - windowSize.x / 3.0f + 2.0f);
	ImGui::PushItemWidth(windowSize.x / 3.0f);
	ImGui::InputFloat("playback", &playSpeed, 0.01f, 0.0f, "%.2f");
	ImGui::PopItemWidth();

	if (ImGui::Button("Reload Textures", ImVec2(windowSize.x / 2.0f + ItemSpacing, 25.0f)))
	{
		buttonPress |= MASK_RELOAD_TEXTURES;
	}

	if (*rdoc_api != nullptr)
	{
		if (bGrabbing)
		{
			(*rdoc_api)->EndFrameCapture(NULL, NULL);
			bGrabbing = false;
		}

		if (ImGui::Button("RenderDoc grab", ImVec2(ImGui::GetWindowContentRegionMax().x, 30.0f)))
		{
			(*rdoc_api)->StartFrameCapture(NULL, NULL);
			bGrabbing = true;
		}
	}

	if (!bIsFullwindow)
	{
		if (ImGui::Button("Go Fullscreen", ImVec2(ImGui::GetWindowContentRegionMax().x, 30.0f)))
		{
			bGoFullscreenChange = true;
		}
	}
	else
	{
		if (ImGui::Button("Go Windowed", ImVec2(ImGui::GetWindowContentRegionMax().x, 30.0f)))
		{
			bGoFullscreenChange = true;
		}
	}

	ImGui::Separator();
	// Default editor picker
	const char* editorItems[] = { "Default", "Notepad", "Notepad++", "Custom" };
	int item_current = static_cast<int>(defaultEditor);
	ImGui::PushItemWidth(0.0f);
	ImGui::Text("Default Editor:");
	ImGui::PopItemWidth();
	ImGui::SameLine(windowSize.x / 2.0f - 5.0f);
	ImGui::PushItemWidth(windowSize.x / 2.0f);
	if (ImGui::Combo("Default Editor", &item_current, editorItems, IM_ARRAYSIZE(editorItems)))
	{
		if (item_current == static_cast<int>(ImGuiEnum::DefaultEditor::E_OTHER))
			bDefaultEditorSet = false;
	}
	defaultEditor = static_cast<ImGuiEnum::DefaultEditor>(item_current);
	ImGui::PopItemWidth();

	// Auto Reload Option
	ImGui::Checkbox("Auto Reload", &bAutoReload);

	// Use Renderdoc on startup
	ImGui::Checkbox("Renderdoc (requires app restart)", &bRenderdoc);

	ImGui::Separator();

	MainResolution(pRenderTargetTexture, aspectRatio, resolution, vMainWindowInfo, bResChanged);

	ImGui::Separator();

	if (iLocation == 0)
	{
		if (vCustomizableBuffer.size() > 0)
		{
			ImGui::Text("Customisable Shader Parameters:");
			CustomizableArea(vCustomizableBuffer);
		}
	}
	else
	{
		if (pBuffer[iLocation - 1].m_vCustomizableBuffer.size() > 0)
		{
			ImGui::Text("Customisable Shader Parameters:");
			CustomizableArea(pBuffer[iLocation - 1].m_vCustomizableBuffer);
		}
	}

	ImGui::Separator();

	//Constant Buffer info
	if (ImGui::TreeNode("Constant Buffer Info"))
	{
		ConstantBufferInfoWindow(
			pRenderTargetTexture,
			pResource,
			pBuffer,
			fScaling,
			iFrame, bPause,
			fGameT, fDeltaT,
			vMouse
		);

		ImGui::TreePop();
	}

	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Application:");
	ImGui::Checkbox("VSync", &bVsync);
	if (ImGui::ColorEdit3("clear colour", (float*)&clearColour))
		clearColourFade = clearColour;

	ImGui::Text("App avg. %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	ImGui::End();
}

void MainImageWindow(
	ID3D11ShaderResourceView* pShaderResourceView,
	Resource* pResource,
	Buffer* pBuffer,
	ImVec2& vWindowSize,
	ImVec2& vCurrentWindowSize,
	ImVec2& vMouse,
	ImVec2  vPadding,
	ImGuiWindowFlags& windowFlags,
	const char* strProj,
	std::string strDefaultEditor,
	ImGuiEnum::DefaultEditor defaultEditor,
	int* piIsUsed,
	int& iLocation, int iHovered,
	float scaling,
	bool& bTrackMouse,
	bool& bExpandedImage, 
	bool bResChanged,
	ImVec4& vWindowInfo
)
{
	ImGui::Begin("ShaderToy", 0, windowFlags);

	if (bResChanged)
	{
		ImGui::SetWindowPos(ImVec2(vWindowInfo.x, vWindowInfo.y));
		ImGui::SetWindowSize(ImVec2(vWindowInfo.z, vWindowInfo.w));
	}
	else
	{
		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();
		vWindowInfo.x = pos.x;
		vWindowInfo.y = pos.y;
		vWindowInfo.z = size.x;
		vWindowInfo.w = size.y;
	}

	const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;

	vWindowSize = ImVec2(vWindowInfo.z, vWindowInfo.w);

	float pos = 0.0f;
	float buttonWidth = 0.0f;

	std::string strName = ""; 

	for (int i = 4; i >= 0; --i)
	{
		if (i == 0)
			ViewButtonLine("Scene", iLocation, vWindowSize, itemSpacing, buttonWidth, pos, 0, true, iHovered, scaling, 5.0f);
		else
		{
			BufferSwitchLookup(strName, i - 1);
			ViewButtonLine(strName.c_str(), iLocation, vWindowSize, itemSpacing, buttonWidth, pos, i, piIsUsed[i - 1], iHovered, scaling, 5.0f);
		}
	}

	windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	bTrackMouse = false;

	if (ImGui::IsWindowHovered())
	{
		bTrackMouse = true;

		ImVec2 winPos = ImGui::GetWindowPos();
		ImVec2 imgPos = ImGui::GetCursorPos();
		vMouse.x = winPos.x + imgPos.x;
		vMouse.y = winPos.y + imgPos.y;
	}

	bool bMoveHover = false;

	const char* shaderName = "";

	if (iLocation == 0)
	{
		shaderName = "shaders/PixelShader.hlsl";
		ImGui::Image(pShaderResourceView, ImVec2(vCurrentWindowSize.x - vPadding.x, vCurrentWindowSize.y - vPadding.y));
		bMoveHover = ImGui::IsItemHovered();
	}
	else
	{
		if (pBuffer[iLocation - 1].m_bIsActive)
		{
			
			ImGui::Image(pBuffer[iLocation - 1].m_pShaderResourceView, ImVec2(vCurrentWindowSize.x - vPadding.x, vCurrentWindowSize.y - vPadding.y));
			bMoveHover = ImGui::IsItemHovered();
		}
		
		switch (iLocation)
		{
		case 1:
			shaderName = "shaders/PixelShaderBufferA.hlsl";
			break;
		case 2:
			shaderName = "shaders/PixelShaderBufferB.hlsl";
			break;
		case 3:
			shaderName = "shaders/PixelShaderBufferC.hlsl";
			break;
		case 4:
			shaderName = "shaders/PixelShaderBufferD.hlsl";
			break;
		}

	}

	if (ImGui::IsWindowHovered() && !bMoveHover)
	{
		windowFlags = 0;
		bTrackMouse = false;
	}

	ImVec2 position = ImGui::GetWindowPos();

	ImGui::SetCursorScreenPos(ImVec2(position.x + 10.0f, position.y + 60.0f));
	if (ImGui::Button("Expand"))
		bExpandedImage = !bExpandedImage;

	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(1.0f, 1.0f, 1.0f, 0.0f));
	ImGui::SetCursorScreenPos(ImVec2(position.x + 10.0f, position.y + vCurrentWindowSize.y - 30.0f));

	std::string path = std::string(PROJECT_PATH_DOUBLE_SLASH);
	if (ImGui::Button(shaderName))
	{
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_DEFAULT)
			system((std::string("start \"") + path + std::string(strProj) + std::string("\\") + std::string(shaderName) + std::string("\"")).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_NOTEPAD)
			system((std::string("start notepad \"") + path + std::string(strProj) + std::string("\\") + std::string(shaderName) + std::string("\"")).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_NOTEPADPP)
			system((std::string("start notepad++ \"") + path + std::string(strProj) + std::string("\\") + std::string(shaderName) + std::string("\" -n0")).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_OTHER)
			system((std::string("start \"") + strDefaultEditor + std::string(" ") + path + std::string(strProj) + std::string("\\") + std::string(shaderName + std::string("\""))).c_str());
	}
	ImGui::PopStyleColor();

	ImGui::End();
}

void SamplerSelection(
	ID3D11Device*			pDevice,
	ID3D11DeviceContext*	pContext,
	Buffer*					pBuffer,
	Channel*				pChannel,
	Resource*				pResource,
	int						iLocation,
	int						iResource,
	const char*				strProj
)
{
	int filterChange = -1;
	int wrapChange = -1;

	// Allow for changes to the filter of samplers
	if (ImGui::BeginMenu("Sampler Filter"))
	{
		Channel* pChannelCopy;
		if (iLocation == 0)
			pChannelCopy = &pChannel[iResource - 1];	
		else
			pChannelCopy = &pBuffer[iLocation - 1].m_Channels[iResource - 1];

		if (pChannelCopy->m_Filter == Channels::FilterType::E_Nearest)
		{
			ImGui::MenuItem("nearest", nullptr, false, false);

			if (ImGui::MenuItem("linear"))
			{
				pChannelCopy->m_Filter = Channels::FilterType::E_Linear;
				filterChange = (int)Channels::FilterType::E_Linear;
			}
			else if (ImGui::MenuItem("mipmap"))
			{
				pChannelCopy->m_Filter = Channels::FilterType::E_Mipmap;
				filterChange = (int)Channels::FilterType::E_Mipmap;
			}
		}
		else if (pChannelCopy->m_Filter == Channels::FilterType::E_Linear)
		{
			ImGui::MenuItem("linear", nullptr, false, false);

			if (ImGui::MenuItem("nearest"))
			{
				pChannelCopy->m_Filter = Channels::FilterType::E_Nearest;
				filterChange = (int)Channels::FilterType::E_Nearest;
			}
			else if (ImGui::MenuItem("mipmap"))
			{
				pChannelCopy->m_Filter = Channels::FilterType::E_Mipmap;
				filterChange = (int)Channels::FilterType::E_Mipmap;
			}
		}
		else if (pChannelCopy->m_Filter == Channels::FilterType::E_Mipmap)
		{
			ImGui::MenuItem("mipmap", nullptr, false, false);

			if (ImGui::MenuItem("nearest"))
			{
				pChannelCopy->m_Filter = Channels::FilterType::E_Nearest;
				filterChange = (int)Channels::FilterType::E_Nearest;
			}
			else if (ImGui::MenuItem("linear"))
			{
				pChannelCopy->m_Filter = Channels::FilterType::E_Linear;
				filterChange = (int)Channels::FilterType::E_Linear;
			}
		}

		ImGui::EndMenu();
	}

	// Allow to change the wrap type of the sampler
	if (ImGui::BeginMenu("Sampler Wrap"))
	{
		Channel* pChannelCopy;

		if (iLocation == 0)
			pChannelCopy = &pChannel[iResource - 1];
		else
			pChannelCopy = &pBuffer[iLocation - 1].m_Channels[iResource - 1];

		if (pChannelCopy->m_Wrap == Channels::WrapType::E_Clamp)
		{
			ImGui::MenuItem("clamp", nullptr, false, false);

			if (ImGui::MenuItem("repeat"))
			{
				pChannelCopy->m_Wrap = Channels::WrapType::E_Repeat;
				wrapChange = (int)Channels::WrapType::E_Repeat;
			}
		}
		else if (pChannelCopy->m_Wrap == Channels::WrapType::E_Repeat)
		{
			ImGui::MenuItem("repeat", nullptr, false, false);

			if (ImGui::MenuItem("clamp"))
			{
				pChannelCopy->m_Wrap = Channels::WrapType::E_Clamp;
				wrapChange = (int)Channels::WrapType::E_Clamp;
			}
		}

		ImGui::EndMenu();
	}

	if (filterChange >= 0 || wrapChange >= 0)
	{
		std::string path = std::string(PROJECT_PATH_DOUBLE_SLASH);

		if (iLocation == 0)
		{
			path = path + std::string(strProj) + std::string("\\channels\\channels.txt");
			WriteChannel(path.c_str(), pChannel);
		}
		else
		{
			ChannelPathSwitchLookup(path, strProj, iLocation - 1);
			WriteChannel(path.c_str(), pBuffer[iLocation - 1].m_Channels);
		}
	}
}

void ResourceWindow(
	ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext,
	ID3D11VertexShader* pVertexShader,
	Resource* pResource,
	Buffer* pBuffer,
	Channel* pChannel,
	TextureLib* pTextureLib,
	DWORD dwShaderFlags,
	const char* strProj,
	float scaling,
	int* piBufferUsed,
	int& iLocation, int& iPressIdentifier,
	int& iHovered,
	bool bResChanged, bool& bProjChange,
	ImVec2 vSize,
	ImVec4& vWindowInfo
)
{
	assert(pDevice != nullptr);
	assert(pContext != nullptr);
	assert(pVertexShader != nullptr);

	ImGui::Begin("Resources");
	
	// Resize and scaling
	if (bResChanged)
	{
		ImGui::SetWindowPos(ImVec2(vWindowInfo.x, vWindowInfo.y));
		ImGui::SetWindowSize(ImVec2(vWindowInfo.z, vWindowInfo.w));
	}
	else
	{
		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();
		vWindowInfo.x = pos.x;
		vWindowInfo.y = pos.y;
		vWindowInfo.z = size.x;
		vWindowInfo.w = size.y;
	}

	const float itemSpacing = ImGui::GetStyle().ItemSpacing.x + 6.0f;
	ImVec2 windowSize = ImGui::GetWindowSize();

	bool vertical = false;
	if (windowSize.y <= windowSize.x)
		vertical = true;

	bool pressed = false;
	static int iRightIdentifier = -1;
	static const char* selected = "";

	float imgButtonWidth = (windowSize.x) / 4.0f - 18.0f * scaling;
	float imgButtonPos = itemSpacing;

	// Setting the right image button resource
	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		int index = 0;
		Channels::ChannelType bufferType = Channels::ChannelType::E_None;
		if (iLocation == 0)
		{
			// Main image
			index = pResource[i].m_iBufferIndex;
			bufferType = pResource[i].m_Type;
		}
		else
		{
			// Buffers
			index = pBuffer[iLocation - 1].m_Res[i].m_iBufferIndex;
			bufferType = pBuffer[iLocation - 1].m_Res[i].m_Type;
		}

		if (bufferType == Channels::ChannelType::E_Buffer)
		{
			bool pressed = false;
			if(!vertical)
				pressed = ImGui::ImageButton(pBuffer[index].m_pShaderResourceView, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			else
			{
				ImGui::SameLine(imgButtonPos);
				pressed = ImGui::ImageButton(pBuffer[index].m_pShaderResourceView, ImVec2(imgButtonWidth, windowSize.y - 41.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			}

			if (ImGui::IsItemHovered())
			{
				iHovered = index + 1;

				if (ImGui::IsMouseClicked(0))
					iLocation = index + 1;
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
			{
				selected = "";
				ImGui::OpenPopup("select");
				iRightIdentifier = i + 1;
			}

			std::string strName = "";
			BufferSwitchLookup(strName, index);
			if (vertical)
			{
				ImGui::SameLine(imgButtonPos);
				OverlayLabel(strName.c_str());
				imgButtonPos += imgButtonWidth + itemSpacing;
			}
			else
			{
				ImGui::SameLine(12.0f);
				OverlayLabel(strName.c_str());
			}
		}
		else if (bufferType == Channels::ChannelType::E_Texture)
		{
			ID3D11ShaderResourceView* pShaderResourceViewCopy;

			if (iLocation == 0)
			{
				pShaderResourceViewCopy = pResource[i].m_pShaderResource;
			}
			else
			{
				pShaderResourceViewCopy = pBuffer[iLocation - 1].m_Res[i].m_pShaderResource;
			}

			if (!vertical)
				pressed = ImGui::ImageButton(pShaderResourceViewCopy, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			else
			{
				ImGui::SameLine(imgButtonPos);
				pressed = ImGui::ImageButton(pShaderResourceViewCopy, ImVec2(imgButtonWidth, windowSize.y - 41.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			}
			bool hov = ImGui::IsItemHovered();
			if (hov && ImGui::IsMouseDoubleClicked(0))
			{
				std::string cmd = std::string("explorer.exe " + std::string(pResource[i].m_strTexture));
				std::replace(cmd.begin(), cmd.end(), '/', '\\');
				system(cmd.c_str());
			}
			if (hov && ImGui::IsMouseClicked(1))
			{
				selected = pResource[i].m_strTexture;
				ImGui::OpenPopup("select");
				iRightIdentifier = i + 1;
			}

			if (pressed)
				iPressIdentifier = i + 1;

			std::string strName = "";
			TextureSwitchLookup(strName, i);
			if (vertical)
			{
				ImGui::SameLine(imgButtonPos);
				ImGui::SetNextWindowFocus();
				OverlayLabel(strName.c_str());
			}
			else
			{
				ImGui::SameLine(12.0f);
				OverlayLabel(strName.c_str());
			}
			imgButtonPos += imgButtonWidth + itemSpacing;
		}
		else
		{
			std::string buttonLabel = "";
			for (int j = 0; j < i; ++j)
				buttonLabel += " ";

			if(!vertical)
				ImGui::Button(buttonLabel.c_str(), ImVec2(windowSize.x - 15.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling));
			else
			{
				ImGui::SameLine(imgButtonPos);
				imgButtonPos += imgButtonWidth + itemSpacing;
				ImGui::Button(buttonLabel.c_str(), ImVec2(imgButtonWidth, windowSize.y - 41.0f * scaling));
			}
			bool hov = ImGui::IsItemHovered();
			if (hov && ImGui::IsMouseClicked(1))
			{
				selected = pBuffer[iLocation - 1].m_Res[i].m_strTexture;
				ImGui::OpenPopup("select");
				iRightIdentifier = i + 1;
			}
		}
	}

	static int iChangeInput = -1;
	// Right click options
	if (ImGui::BeginPopup("select"))
	{
		if (strcmp(selected, "") != 0)
		{
			if (ImGui::Selectable("Open in explorer..."))
			{
				std::string highlighted = std::string(selected);
				highlighted = ".\\" + highlighted;
				std::replace(highlighted.begin(), highlighted.end(), '/', '\\');
				std::string cmd = std::string("explorer.exe /select," + highlighted);
				system(cmd.c_str());
			}
		}
		if (ImGui::BeginMenu("Input Source"))
		{
			if (ImGui::MenuItem("None"))
				iChangeInput = 5;
			else if (ImGui::MenuItem("Texture"))
				iChangeInput = 0;
			else if (iLocation != 1 && ImGui::MenuItem("BufferA"))
				iChangeInput = 1;
			else if (iLocation != 2 && ImGui::MenuItem("BufferB"))
				iChangeInput = 2;
			else if (iLocation != 3 && ImGui::MenuItem("BufferC"))
				iChangeInput = 3;
			else if (iLocation != 4 && ImGui::MenuItem("BufferD"))
				iChangeInput = 4;
			else
			{
				// The padding shouldn't go higher than 5
				assert(iLocation < 5);
			}

			ImGui::EndMenu();
		}
		
		SamplerSelection(pDevice, pContext, pBuffer, pChannel, pResource, iLocation, iRightIdentifier, strProj);

		ImGui::EndPopup();
	}

	if (iChangeInput >= 0)
	{
		std::string path = std::string(PROJECT_PATH_DOUBLE_SLASH);

		if (iLocation == 0)
		{
			if (iChangeInput == 0)
			{
				pChannel[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Texture;
				pChannel[iRightIdentifier - 1].m_Filter = Channels::FilterType::E_Linear;
				pChannel[iRightIdentifier - 1].m_Wrap = Channels::WrapType::E_Repeat;
				piBufferUsed[pResource[iRightIdentifier - 1].m_iBufferIndex] -= 1;
			}
			else if (iChangeInput == 5)
			{
				pChannel[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_None;
				pResource[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_None;
				piBufferUsed[pResource[iRightIdentifier - 1].m_iBufferIndex] -= 1;
			}
			else
			{
				pChannel[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Buffer;
				pChannel[iRightIdentifier - 1].m_BufferId = Channels::BufferId(iChangeInput - 1);
				piBufferUsed[iChangeInput - 1] += 1;
			}

			strcpy(pChannel[iRightIdentifier - 1].m_strTexture, "textures/default.dds");
			path = path + std::string(strProj) + std::string("\\channels\\channels.txt");
			WriteChannel(path.c_str(), pChannel);
		}
		else
		{
			if (iChangeInput == 0)
			{
				pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Texture;
				pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_Filter = Channels::FilterType::E_Linear;
				pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_Wrap = Channels::WrapType::E_Repeat;
				piBufferUsed[pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_BufferId] -= 1;
			}
			else if (iChangeInput == 5)
			{
				pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_None;
				piBufferUsed[pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_BufferId] -= 1;
			}
			else
			{
				pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Buffer;
				pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_BufferId = Channels::BufferId(iChangeInput - 1);
				pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_Filter = Channels::FilterType::E_Linear;
				pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_Wrap = Channels::WrapType::E_Clamp;
				piBufferUsed[iChangeInput - 1] += 1;
			}

			strcpy(pBuffer[iLocation - 1].m_Channels[iRightIdentifier - 1].m_strTexture, "textures/default.dds");
			ChannelPathSwitchLookup(path, strProj, iLocation - 1);
			WriteChannel(path.c_str(), pBuffer[iLocation - 1].m_Channels);
		}

		bProjChange = true;
		iChangeInput = -1;
	}

	// Resource picker
	if (iPressIdentifier)
	{
		std::string selected = "";
		if (iLocation == 0)
			selected = pResource[iPressIdentifier - 1].m_strTexture;
		else
			selected = pBuffer[iLocation - 1].m_Res[iPressIdentifier - 1].m_strTexture;

		static bool newTexture = false;
		std::string fileName = "";

		if (ImGuiFileDialog::Instance(pTextureLib)->FileDialog("Choose File", selected, iLocation, "\0", ".\\textures", ""))
		{
			if (ImGuiFileDialog::Instance(pTextureLib)->IsOk == true)
			{
				fileName = ImGuiFileDialog::Instance(pTextureLib)->GetCurrentFileName();
				ImGuiFileDialog::Instance(pTextureLib)->Clear();
			}
			else
			{
				fileName = selected;
			}

			// Update the resource
			std::string path = std::string(PROJECT_PATH_DOUBLE_SLASH);
			if (iLocation == 0)
			{
				strcpy(pChannel[iPressIdentifier - 1].m_strTexture, fileName.c_str());
				path = path + std::string(strProj) + std::string("\\channels\\channels.txt");
				WriteChannel(path.c_str(), pChannel);
			}
			else
			{
				strcpy(pBuffer[iLocation - 1].m_Channels[iPressIdentifier - 1].m_strTexture, fileName.c_str());
				ChannelPathSwitchLookup(path, strProj, iLocation - 1);
				WriteChannel(path.c_str(), pBuffer[iLocation - 1].m_Channels);
			}

			newTexture = true;
		}

		if (newTexture || selected != std::string(pResource[iPressIdentifier - 1].m_strTexture))
		{
			if (!newTexture)
				fileName = selected;

			Channel channel = Channel(fileName.c_str());

			if (iLocation == 0)
			{
				// Main image
				strcpy(pResource[iPressIdentifier - 1].m_strTexture, fileName.c_str());

				// Get texture from texture lib
				pTextureLib->GetTexture(channel.m_strTexture, &pResource[(iPressIdentifier - 1) + iLocation * 4].m_pShaderResource);
			}
			else
			{
				// Buffers
				strcpy(pBuffer[iLocation - 1].m_Res[iPressIdentifier - 1].m_strTexture, fileName.c_str());

				// Get texture from texture lib
				pTextureLib->GetTexture(channel.m_strTexture, &pBuffer[iLocation - 1].m_Res[iPressIdentifier - 1].m_pShaderResource);
			}

			if (newTexture)
			{
				iPressIdentifier = 0;
				ImGuiFileDialog::Instance(pTextureLib)->Clear();
				newTexture = false;
			}
		}
	}

	ImGui::End();
}

void ShaderErrorWindow(
	Buffer* pBuffer, 
	std::vector<std::string> mainErrors, 
	std::string strProj,
	std::string strDefaultEditor, 
	ImGuiEnum::DefaultEditor defaultEditor, 
	bool bResChanged, 
	ImVec4& vWindowInfo
)
{
	assert(pBuffer != nullptr);

	ImGui::Begin("Shader Error Messages");

	if (bResChanged)
	{
		ImGui::SetWindowPos(ImVec2(vWindowInfo.x, vWindowInfo.y));
		ImGui::SetWindowSize(ImVec2(vWindowInfo.z, vWindowInfo.w));
	}
	else
	{
		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();
		vWindowInfo.x = pos.x;
		vWindowInfo.y = pos.y;
		vWindowInfo.z = size.x;
		vWindowInfo.w = size.y;
	}

	for(int n = 0; n < mainErrors.size(); ++n)
		ErrorLine(mainErrors[n], strProj, strDefaultEditor, defaultEditor);

	for (int i = 0; i < 4; ++i)
	{
		if (pBuffer[i].m_ShaderError.size() > 0)
		{
			for (int n = 0; n < pBuffer[i].m_ShaderError.size(); ++n)
			{
				ErrorLine(pBuffer[i].m_ShaderError[n], strProj, strDefaultEditor, defaultEditor);
			}
		}
	}

	ImGui::End();
}

void DefaultEditorSelector(
	ImVec2						pos, 
	ImGuiEnum::DefaultEditor&	defaultEditor, 
	std::string&				strDefaultEditor, 
	bool&						defaultEditorSelected)
{
	ImGui::Begin("Select Default Editor");

	ImVec2 size = ImGui::GetWindowSize();
	ImGui::SetWindowPos(ImVec2(pos.x - size.x / 2, pos.y - size.y / 2));

	ImGui::Text("Enter path to desired shader editor:");

	ImVec2 windowSize = ImGui::GetWindowSize();
	
	static char wc[MAX_PATH_LENGTH];

	ImGui::PushItemWidth(windowSize.x);
	ImGui::InputText("Location:", wc, MAX_PATH_LENGTH);
	ImGui::PopItemWidth(); 

	ImGui::PushItemWidth(100.0f);

	if (ImGui::Button("Done"))
	{
		std::string strWc = std::string(wc);
		defaultEditorSelected = true;
		if (strWc != "")
		{
			DefaultEditorStrFix(strWc);
			strDefaultEditor = strWc;
			defaultEditor = ImGuiEnum::DefaultEditor::E_OTHER;
		}
		else
		{
			std::wstring strKeyDefaultValue;
			GetStringRegKey(
				static_cast<int>(ImGuiEnum::DefaultEditor::E_NOTEPADPP),
				L"",
				strKeyDefaultValue,
				L"bad"
			);

			if (strKeyDefaultValue.length() > 0)
				defaultEditor = ImGuiEnum::DefaultEditor::E_NOTEPADPP;
			else
				defaultEditor = ImGuiEnum::DefaultEditor::E_NOTEPAD;
		}
	}
	ImGui::PopItemWidth();

	ImGui::SameLine(100.0f);
	ImGui::Text("*Can be left empty!");


	ImGui::End();
}

void Barrier(ImVec2 size)
{
	ImGui::SetNextWindowBgAlpha(0.8f);

	ImGuiWindowFlags flags = 0;
	flags |= ImGuiWindowFlags_NoScrollbar;
	flags |= ImGuiWindowFlags_NoMove;
	flags |= ImGuiWindowFlags_NoTitleBar;
	flags |= ImGuiWindowFlags_NoResize;

	ImGui::Begin("Barrier", 0, flags);

	ImGui::SetWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetWindowSize(size);

	ImGui::End();
}

void ShaderCompileItem(const char* strFlag, DWORD flag, DWORD oldFlags, DWORD& setFlags)
{
	bool check = oldFlags & flag ? true : false;
	ImGui::Checkbox(strFlag, &check);
	if (check)
		setFlags |= flag;
}

bool MenuBar(
	ID3D11DeviceContext*		pContext,
	ID3D11BlendState*			pBlendStateEnabled,
	ID3D11BlendState*			pBlendStateDisabled,
	DWORD&						dwShaderFlags
)
{
	assert(pContext != nullptr);

	DWORD shaderFlagCopy = dwShaderFlags;
	static bool blendEnabled = false;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Settings"))
		{
			if (ImGui::BeginMenu("Blend"))
			{
				ImGui::Checkbox("Enabled", &blendEnabled);

				FLOAT BlendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				UINT SampleMask = 0xffffffff;

				if (blendEnabled)
					pContext->OMSetBlendState(pBlendStateEnabled, BlendFactor, SampleMask);
				else
					pContext->OMSetBlendState(pBlendStateDisabled, BlendFactor, SampleMask);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Compiler"))
			{
				if (ImGui::BeginMenu("Shader Flags"))
				{
					dwShaderFlags = 0;

					ShaderCompileItem(
						"D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY",
						D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 
						shaderFlagCopy, 
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_ENABLE_STRICTNESS",
						D3DCOMPILE_ENABLE_STRICTNESS,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_DEBUG",
						D3DCOMPILE_DEBUG,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_SKIP_VALIDATION",
						D3DCOMPILE_SKIP_VALIDATION,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_OPTIMIZATION_LEVEL0",
						D3DCOMPILE_OPTIMIZATION_LEVEL0,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_OPTIMIZATION_LEVEL1",
						D3DCOMPILE_OPTIMIZATION_LEVEL1,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_OPTIMIZATION_LEVEL2",
						D3DCOMPILE_OPTIMIZATION_LEVEL2,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_OPTIMIZATION_LEVEL3",
						D3DCOMPILE_OPTIMIZATION_LEVEL3,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_PACK_MATRIX_ROW_MAJOR",
						D3DCOMPILE_PACK_MATRIX_ROW_MAJOR,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR",
						D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_IEEE_STRICTNESS",
						D3DCOMPILE_IEEE_STRICTNESS,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_PARTIAL_PRECISION",
						D3DCOMPILE_PARTIAL_PRECISION,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_AVOID_FLOW_CONTROL",
						D3DCOMPILE_AVOID_FLOW_CONTROL,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_PREFER_FLOW_CONTROL",
						D3DCOMPILE_PREFER_FLOW_CONTROL,
						shaderFlagCopy,
						dwShaderFlags
					);
					ShaderCompileItem(
						"D3DCOMPILE_WARNINGS_ARE_ERRORS",
						D3DCOMPILE_WARNINGS_ARE_ERRORS,
						shaderFlagCopy,
						dwShaderFlags
					);

					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (shaderFlagCopy != dwShaderFlags)
		return true;

	return false;
}

float GetImGuiDeltaTime()
{
	return ImGui::GetIO().DeltaTime;
}