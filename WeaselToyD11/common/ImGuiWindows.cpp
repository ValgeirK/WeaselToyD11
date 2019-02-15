#include "ImGuiWindows.h"

#include "../lib/imgui.h"
#include "../lib/imgui_internal.h"
#include "../lib/imgui_impl_dx11.h"
#include "../lib/imgui_impl_win32.h"

#include <d3d11.h>
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

//--------------------------------------------------------------------------------------
// Methods to decrease code
//--------------------------------------------------------------------------------------

void BufferSwitchLookup(std::string& strName, int index, std::string post = "")
{
	switch (index)
	{
	case 0:
		strName = "Buffer A";
		break;
	case 1:
		strName = "Buffer B";
		break;
	case 2:
		strName = "Buffer C";
		break;
	case 3:
		strName = "Buffer D";
		break;
	}

	strName += post;
}

void TextureSwitchLookup(std::string& strName, int index, std::string post = "")
{
	switch (index)
	{
	case 0:
		strName = "Texture A";
		break;
	case 1:
		strName = "Texture B";
		break;
	case 2:
		strName = "Texture C";
		break;
	case 3:
		strName = "Texture D";
		break;
	}

	strName += post;
}

void ChannelPathSwitchLookup(std::string& path, const char* strProj, int index)
{
	switch (index)
	{
	case 0:
		path = path + std::string(strProj) + std::string("\\channels\\channelsA.txt");
		break;
	case 1:
		path = path + std::string(strProj) + std::string("\\channels\\channelsB.txt");
		break;
	case 2:
		path = path + std::string(strProj) + std::string("\\channels\\channelsC.txt");
		break;
	case 3:
		path = path + std::string(strProj) + std::string("\\channels\\channelsD.txt");
		break;
	}
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

void ViewButtonLine(const char* strName, int& iPadding, ImVec2 vWindowSize, float fItemSpacing, float& fButtonWidth, float& fPos, int index, bool active, int hovered, float scale, float fButtonCount)
{
	bool useColour = false;
	ImVec4 colour = ImVec4();

	if (iPadding == index)
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

	if (ButtonLine(strName, vWindowSize, fItemSpacing, fButtonWidth, fPos, scale, fButtonCount))
		iPadding = index;

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
			system((std::string("start notepad++ ") + std::string(shaderName) + std::string(" -n0")).c_str());
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
	if (xRes == 1024 && yRes == 600)
		item_current = 2; // 1024x600
	if (xRes == 1024 && yRes == 768)
		item_current = 3; // 1024x768
	if (xRes == 1152&& yRes == 864)
		item_current = 4; // 1152x864
	if (xRes == 1280&& yRes == 720)
		item_current = 5; // 1280x720
	if (xRes == 1280 && yRes == 768)
		item_current = 6; // 1280x768
	if (xRes == 1280 && yRes == 800)
		item_current = 7; // 1280x800
	if (xRes == 1280 && yRes == 1024)
		item_current = 8; // 1080x1024
	if (xRes == 1360 && yRes == 768)
		item_current = 9; // 1360x768
	if (xRes == 1366 && yRes == 768)
		item_current = 10; // 1366x768
	if (xRes == 1440 && yRes == 900)
		item_current = 11; // 1440x900
	if (xRes == 1536 && yRes == 864)
		item_current = 12; // 1536x864
	if (xRes == 1600 && yRes == 900)
		item_current = 13; // 1600x900
	if (xRes == 1680 && yRes == 1050)
		item_current = 14; // 1680x1050
	if (xRes == 1920 && yRes == 1080)
		item_current = 15; // 1920x1080
	if (xRes == 1920 && yRes == 1200)
		item_current = 16; // 1920x1200
	if (xRes == 2560 && yRes == 1080)
		item_current = 17; // 2560x1080
	if (xRes == 2560 && yRes == 1440)
		item_current = 18; // 2560x1440
	if (xRes == 3440 && yRes == 1440)
		item_current = 19; // 3440x1440
	if (xRes == 3840 && yRes == 2160)
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

		switch (vCustomizableBuffer[i].size)
		{
		case 4:
			if (vCustomizableBuffer[i].type == 3) // FLOAT
			{
				if (strcmp(vCustomizableBuffer[i].strCommand, "slider") == 0)
					ImGui::SliderFloat(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data, vCustomizableBuffer[i].min, vCustomizableBuffer[i].max);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "input") == 0)
					ImGui::InputFloat(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data, vCustomizableBuffer[i].step);
			}
			else if (vCustomizableBuffer[i].type == 2) // INT
			{
				if (strcmp(vCustomizableBuffer[i].strCommand, "slider") == 0)
					ImGui::SliderInt(vCustomizableBuffer[i].strVariable, (int*)vCustomizableBuffer[i].data, (int)vCustomizableBuffer[i].min, (int)vCustomizableBuffer[i].max);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "input") == 0)
					ImGui::InputInt(vCustomizableBuffer[i].strVariable, (int*)vCustomizableBuffer[i].data, (int)vCustomizableBuffer[i].step);
			}
			break;
		case 8:
			if (vCustomizableBuffer[i].type == 3) // FLOAT
			{
				if (strcmp(vCustomizableBuffer[i].strCommand, "slider") == 0)
					ImGui::SliderFloat2(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data, vCustomizableBuffer[i].min, vCustomizableBuffer[i].max);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "input") == 0)
					ImGui::InputFloat2(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data, 3);
			}
			else if (vCustomizableBuffer[i].type == 2) // INT
			{
				if (strcmp(vCustomizableBuffer[i].strCommand, "slider") == 0)
					ImGui::SliderInt2(vCustomizableBuffer[i].strVariable, (int*)vCustomizableBuffer[i].data, (int)vCustomizableBuffer[i].min, (int)vCustomizableBuffer[i].max);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "input") == 0)
					ImGui::InputInt2(vCustomizableBuffer[i].strVariable, (int*)vCustomizableBuffer[i].data, 3);
			}
			break;
		case 12:
			if (vCustomizableBuffer[i].type == 3) // FLOAT
			{
				if (strcmp(vCustomizableBuffer[i].strCommand, "slider") == 0)
					ImGui::SliderFloat3(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data, vCustomizableBuffer[i].min, vCustomizableBuffer[i].max);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "input") == 0)
					ImGui::InputFloat3(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data, 3);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "colorEdit") == 0 
					|| strcmp(vCustomizableBuffer[i].strCommand, "colourEdit") == 0)
					ImGui::ColorEdit3(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data);
			}
			else if (vCustomizableBuffer[i].type == 2) // INT
			{
				if (strcmp(vCustomizableBuffer[i].strCommand, "slider") == 0)
					ImGui::SliderInt3(vCustomizableBuffer[i].strVariable, (int*)vCustomizableBuffer[i].data, (int)vCustomizableBuffer[i].min, (int)vCustomizableBuffer[i].max);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "input") == 0)
					ImGui::InputInt3(vCustomizableBuffer[i].strVariable, (int*)vCustomizableBuffer[i].data, 3);
			}
			break;
		case 16:
			if (vCustomizableBuffer[i].type == 3) // FLOAT
			{
				if (strcmp(vCustomizableBuffer[i].strCommand, "slider") == 0)
					ImGui::SliderFloat4(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data, vCustomizableBuffer[i].min, vCustomizableBuffer[i].max);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "input") == 0)
					ImGui::InputFloat4(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data, 3);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "colorEdit") == 0
					|| strcmp(vCustomizableBuffer[i].strCommand, "colourEdit") == 0)
					ImGui::ColorEdit4(vCustomizableBuffer[i].strVariable, (float*)vCustomizableBuffer[i].data);
			}
			else if (vCustomizableBuffer[i].type == 2) // INT
			{
				if (strcmp(vCustomizableBuffer[i].strCommand, "slider") == 0)
					ImGui::SliderInt4(vCustomizableBuffer[i].strVariable, (int*)vCustomizableBuffer[i].data, (int)vCustomizableBuffer[i].min, (int)vCustomizableBuffer[i].max);
				else if (strcmp(vCustomizableBuffer[i].strCommand, "input") == 0)
					ImGui::InputInt4(vCustomizableBuffer[i].strVariable, (int*)vCustomizableBuffer[i].data, 3);
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
	int iFrame, int& buttonPress,
	bool& bResChanged, bool& bNewProj,
	bool& bDefaultEditorSet, bool bIsFullwindow,
	bool& bGoFullscreenChange, bool& bVsync,
	ImVec4& vWindowInfo,
	ImVec4& vMainWindowInfo,
	std::vector<CustomizableBuffer>& vCustomizableBuffer
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
		ImGui::InputText("Project name", const_cast<char*>(strProj.c_str()), 20);

		ImGui::Separator();
		if (ButtonLine("Cancel", windowSize, itemSpacing, buttonWidth, pos, fScaling, 2.0f))
			copyClicked = false;
		if (ButtonLine("Save", windowSize, itemSpacing, buttonWidth, pos, fScaling, 2.0f))
		{
			std::string oldPath = basePath + strProj;
			system((std::string("md ") + newPath + std::string("\\channels")).c_str());
			system((std::string("md ") + newPath + std::string("\\shaders")).c_str());
			system((std::string("xcopy ") + oldPath + std::string("\\channels ") + newPath + std::string("\\channels") + std::string(" /i /E")).c_str());
			system((std::string("xcopy ") + oldPath + std::string("\\shaders ") + newPath + std::string("\\shaders") + std::string(" /i /E")).c_str());
			copyClicked = false;
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
			system((std::string("md ") + newPath + std::string("\\channels")).c_str());
			system((std::string("md ") + newPath + std::string("\\shaders")).c_str());
			system((std::string("xcopy channels ") + newPath + std::string("\\channels") + std::string(" /i /E")).c_str());
			system((std::string("xcopy shaders ") + newPath + std::string("\\shaders") + std::string(" /i /E")).c_str());
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
				bNewProj = true;
			}

			ImGuiFileDialog::Instance(pTextureLib)->Clear();
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

	ImGui::SameLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - windowSize.x / 3.0f);
	ImGui::PushItemWidth(windowSize.x / 3.0f);
	ImGui::Text("Play Speed:");
	ImGui::PopItemWidth();

	const float ItemSpacing = ImGui::GetStyle().ItemSpacing.x;

	if (ImGui::Button("Restart", ImVec2(windowSize.x / 2.0f + ItemSpacing, 25.0f)))
	{
		fGameT = 0.0f;
		buttonPress += 0x0001;
	}

	if (ImGui::Button("Reload Shaders", ImVec2(windowSize.x / 2.0f + ItemSpacing, 25.0f)))
	{
		buttonPress += 0x0001;
	}

	ImGui::SameLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - windowSize.x / 3.0f + 2.0f);
	ImGui::PushItemWidth(windowSize.x / 3.0f);
	ImGui::InputFloat("playback", &playSpeed, 0.01f, 0.0f, "%.2f");
	ImGui::PopItemWidth();

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

	ImGui::Separator();

	MainResolution(pRenderTargetTexture, aspectRatio, resolution, vMainWindowInfo, bResChanged);

	ImGui::Separator();

	if (vCustomizableBuffer.size() > 0)
	{
		ImGui::Text("Customisable Shader Parameters:");
		CustomizableArea(vCustomizableBuffer);
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
	int& iPadding, int iHovered,
	float scaling,
	bool& bTrackMouse,
	bool& bFullscreen, 
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
			ViewButtonLine("Scene", iPadding, vWindowSize, itemSpacing, buttonWidth, pos, 0, true, iHovered, scaling, 5.0f);

		BufferSwitchLookup(strName, i - 1);
		ViewButtonLine(strName.c_str(), iPadding, vWindowSize, itemSpacing, buttonWidth, pos, i, pBuffer[i - 1].m_bIsActive, iHovered, scaling, 5.0f);
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

	if (iPadding == 0)
	{
		shaderName = "shaders/PixelShader.hlsl";
		ImGui::Image(pShaderResourceView, ImVec2(vCurrentWindowSize.x - vPadding.x, vCurrentWindowSize.y - vPadding.y));
		bMoveHover = ImGui::IsItemHovered();
	}
	else
	{
		if (pBuffer[iPadding - 1].m_bIsActive)
		{
			
			ImGui::Image(pBuffer[iPadding - 1].m_pShaderResourceView, ImVec2(vCurrentWindowSize.x - vPadding.x, vCurrentWindowSize.y - vPadding.y));
			bMoveHover = ImGui::IsItemHovered();
		}
		
		switch (iPadding)
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
		bFullscreen = !bFullscreen;

	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(1.0f, 1.0f, 1.0f, 0.0f));
	ImGui::SetCursorScreenPos(ImVec2(position.x + 10.0f, position.y + vCurrentWindowSize.y - 30.0f));

	std::string path = std::string(PROJECT_PATH_DOUBLE_SLASH);
	if (ImGui::Button(shaderName))
	{
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_DEFAULT)
			system((std::string("start ") + path + std::string(strProj) + std::string("\\") + std::string(shaderName)).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_NOTEPAD)
			system((std::string("start notepad ") + path + std::string(strProj) + std::string("\\") + std::string(shaderName)).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_NOTEPADPP)
			system((std::string("start notepad++ ") + path + std::string(strProj) + std::string("\\") + std::string(shaderName) + std::string(" -n0")).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_OTHER)
			system((std::string("start ") + strDefaultEditor + std::string(" ") + path + std::string(strProj) + std::string("\\") + std::string(shaderName)).c_str());
	}
	ImGui::PopStyleColor();

	ImGui::End();
}

void ResourceWindow(
	ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext,
	ID3D11VertexShader* pVertexShader,
	Resource* pResource,
	Buffer* pBuffer,
	Channel* pChannel,
	TextureLib* pTextureLib, 
	const char* strProj,
	float scaling,
	int& iPadding, int& iPressIdentifier, 
	int& iHovered, 
	bool bResChanged, bool& bProjChange,
	ImVec2 vSize,
	ImVec4& vWindowInfo
)
{
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
	for (int i = 0; i < MAX_RESORCES; ++i)
	{
		int index = 0;
		Channels::ChannelType bufferType = Channels::ChannelType::E_None;
		if (iPadding == 0)
		{
			// Main image
			index = pResource[i].m_iBufferIndex;
			bufferType = pResource[i].m_Type;
		}
		else
		{
			// Buffers
			index = pBuffer[iPadding - 1].m_Res[i].m_iBufferIndex;
			bufferType = pBuffer[iPadding - 1].m_Res[i].m_Type;
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
					iPadding = index + 1;
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
			if (iPadding == 0)
			{
				// Main Image
				if(!vertical)
					pressed = ImGui::ImageButton(pResource[i].m_pShaderResource, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
				else
				{
					ImGui::SameLine(imgButtonPos);
					pressed = ImGui::ImageButton(pResource[i].m_pShaderResource, ImVec2(imgButtonWidth, windowSize.y - 41.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
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
			}
			else
			{
				if(!vertical)
					pressed = ImGui::ImageButton(pBuffer[iPadding - 1].m_Res[i].m_pShaderResource, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
				else
				{
					ImGui::SameLine(imgButtonPos);
					pressed = ImGui::ImageButton(pBuffer[iPadding - 1].m_Res[i].m_pShaderResource, ImVec2(imgButtonWidth, windowSize.y - 41.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
				}
				bool hov = ImGui::IsItemHovered();
				if (hov && ImGui::IsMouseDoubleClicked(0))
				{
					std::string cmd = std::string("explorer.exe " + std::string(pBuffer[iPadding - 1].m_Res[i].m_strTexture));
					std::replace(cmd.begin(), cmd.end(), '/', '\\');
					system(cmd.c_str());
				}
				if (hov && ImGui::IsMouseClicked(1))
				{
					selected = pBuffer[iPadding - 1].m_Res[i].m_strTexture;
					ImGui::OpenPopup("select");
					iRightIdentifier = i + 1;
				}
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
			if(!vertical)
				ImGui::ImageButton(nullptr, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			else
			{
				ImGui::SameLine(imgButtonPos);
				imgButtonPos += imgButtonWidth + itemSpacing;
				ImGui::ImageButton(nullptr, ImVec2(imgButtonWidth, windowSize.y - 41.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			}
			bool hov = ImGui::IsItemHovered();
			if (hov && ImGui::IsMouseClicked(1))
			{
				selected = pBuffer[iPadding - 1].m_Res[i].m_strTexture;
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
			else if (iPadding != 1 && ImGui::MenuItem("BufferA"))
				iChangeInput = 1;
			else if (iPadding != 2 && ImGui::MenuItem("BufferB"))
				iChangeInput = 2;
			else if (iPadding != 3 && ImGui::MenuItem("BufferC"))
				iChangeInput = 3;
			else if (iPadding != 4 && ImGui::MenuItem("BufferD"))
				iChangeInput = 4;
			else
			{
				// The padding shouldn't go higher than 5
				assert(iPadding < 5);
			}

			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	if (iChangeInput >= 0)
	{
		std::string path = std::string(PROJECT_PATH_DOUBLE_SLASH);

		if (iPadding == 0)
		{
			if (iChangeInput == 0)
			{
				pChannel[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Texture;
				pChannel[iRightIdentifier - 1].m_Filter = Channels::FilterType::E_Linear;
				pChannel[iRightIdentifier - 1].m_Wrap = Channels::WrapType::E_Repeat;
			}
			else if (iChangeInput == 5)
			{
				pChannel[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_None;
				pResource[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_None;
			}
			else
			{
				pChannel[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Buffer;
				pChannel[iRightIdentifier - 1].m_BufferId = Channels::BufferId(iChangeInput - 1);
				if (!pBuffer[iRightIdentifier - 1].m_bIsActive)
					pBuffer[iRightIdentifier - 1].InitBuffer(
						pDevice, 
						pContext, 
						pVertexShader,
						pTextureLib, 
						pBuffer, 
						strProj, 
						(int)vSize.x, 
						(int)vSize.y, 
						pChannel[iRightIdentifier - 1].m_BufferId);
			}

			strcpy(pChannel[iRightIdentifier - 1].m_strTexture, "textures/default.dds");
			path = path + std::string(strProj) + std::string("\\channels\\channels.txt");
			WriteChannel(path.c_str(), pChannel);
		}
		else
		{
			if (iChangeInput == 0)
			{
				pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Texture;
				pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_Filter = Channels::FilterType::E_Linear;
				pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_Wrap = Channels::WrapType::E_Repeat;
			}
			else if (iChangeInput == 5)
			{
				pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_None;
			}
			else
			{
				pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Buffer;
				pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_BufferId = Channels::BufferId(iChangeInput - 1);
				pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_Filter = Channels::FilterType::E_Linear;
				pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_Wrap = Channels::WrapType::E_Clamp;
				if (!pBuffer[iRightIdentifier - 1].m_bIsActive)
					pBuffer[iRightIdentifier - 1].InitBuffer(
						pDevice, 
						pContext, 
						pVertexShader, 
						pTextureLib, 
						pBuffer, 
						strProj, 
						(int)vSize.x, 
						(int)vSize.y, 
						pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_BufferId);
			}

			strcpy(pBuffer[iPadding - 1].m_Channels[iRightIdentifier - 1].m_strTexture, "textures/default.dds");
			ChannelPathSwitchLookup(path, strProj, iPadding - 1);
			WriteChannel(path.c_str(), pBuffer[iPadding - 1].m_Channels);
		}

		bProjChange = true;
		iChangeInput = -1;
	}

	// Resource picker
	if (iPressIdentifier)
	{
		std::string selected = "";
		if (iPadding == 0)
			selected = pResource[iPressIdentifier - 1].m_strTexture;
		else
			selected = pBuffer[iPadding - 1].m_Res[iPressIdentifier - 1].m_strTexture;

		static bool newTexture = false;
		std::string fileName = "";

		if (ImGuiFileDialog::Instance(pTextureLib)->FileDialog("Choose File", selected, iPadding, ".dds\0", ".\\textures", ""))
		{
			if (ImGuiFileDialog::Instance(pTextureLib)->IsOk == true)
			{
				fileName = ImGuiFileDialog::Instance(pTextureLib)->GetCurrentFileName();
				ImGuiFileDialog::Instance(pTextureLib)->Clear();

				fileName = std::string("textures/" + fileName);
			}
			else
			{
				fileName = selected;
			}

			// Update the resource
			std::string path = std::string(PROJECT_PATH_DOUBLE_SLASH);
			if (iPadding == 0)
			{
				strcpy(pChannel[iPressIdentifier - 1].m_strTexture, fileName.c_str());
				path = path + std::string(strProj) + std::string("\\channels\\channels.txt");
				WriteChannel(path.c_str(), pChannel);
			}
			else
			{
				strcpy(pBuffer[iPadding - 1].m_Channels[iPressIdentifier - 1].m_strTexture, fileName.c_str());
				ChannelPathSwitchLookup(path, strProj, iPadding - 1);
				WriteChannel(path.c_str(), pBuffer[iPadding - 1].m_Channels);
			}

			newTexture = true;
		}

		if (newTexture || selected != std::string(pResource[iPressIdentifier - 1].m_strTexture))
		{
			if (!newTexture)
				fileName = selected;

			Channel channel = Channel(fileName.c_str());

			if (iPadding == 0)
			{
				// Main image

				strcpy(pResource[iPressIdentifier - 1].m_strTexture, fileName.c_str());

				// Get texture from texture lib
				pTextureLib->GetTexture(channel.m_strTexture, &pResource[(iPressIdentifier - 1) + iPadding * 4].m_pShaderResource);

				// Texture
				pContext->PSSetShaderResources((iPressIdentifier - 1) + iPadding * 4, 1, &pResource[(iPressIdentifier - 1) + iPadding * 4].m_pShaderResource);
			}
			else
			{
				// Buffers

				strcpy(pBuffer[iPadding - 1].m_Res[iPressIdentifier - 1].m_strTexture, fileName.c_str());

				// Get texture from texture lib
				pTextureLib->GetTexture(channel.m_strTexture, &pBuffer[iPadding - 1].m_Res[iPressIdentifier - 1].m_pShaderResource);

				// Texture
				pContext->PSSetShaderResources((iPressIdentifier - 1) + iPadding * 4, 1, &pBuffer[iPadding - 1].m_Res[iPressIdentifier - 1].m_pShaderResource);
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

void DefaultEditorSelector(ImVec2 pos, ImGuiEnum::DefaultEditor& defaultEditor, std::string& strDefaultEditor, bool& defaultEditorSelected)
{
	ImGui::Begin("Select Default Editor");

	ImVec2 size = ImGui::GetWindowSize();
	ImGui::SetWindowPos(ImVec2(pos.x - size.x / 2, pos.y - size.y / 2));

	ImGui::Text("Enter path to desired shader editor:");

	ImVec2 windowSize = ImGui::GetWindowSize();
	
	static char wc[MAX_PATH];

	ImGui::PushItemWidth(windowSize.x);
	ImGui::InputText("Location:", wc, MAX_PATH);
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

float GetImGuiDeltaTime()
{
	return ImGui::GetIO().DeltaTime;
}