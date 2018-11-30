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
#include "type/Resource.h"
#include "type/Channel.h"

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
	bool useColor = false;
	ImVec4 color = ImVec4();

	if (iPadding == index)
	{
		// Active tab
		useColor = true;
		color = (ImVec4)ImColor(1.0f, 0.5f, 0.5f, 1.0f);
	}
	else
	if (!active)
	{
		// Unused tabs
		useColor = true;
		color = (ImVec4)ImColor(0.5f, 0.5f, 0.5f, 1.0f);
	}
	else
	if (hovered == index)
	{
		// Hovered tab
		useColor = true;
		color = (ImVec4)ImColor(0.5f, 0.8f, 0.5f, 1.0f);
	}

	if (useColor)
		ImGui::PushStyleColor(ImGuiCol_Button, color);

	if (ButtonLine(strName, vWindowSize, fItemSpacing, fButtonWidth, fPos, scale, fButtonCount))
		iPadding = index;

	if(useColor)
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

void OverlayLabel(const char* strName, ImVec2 windowSize)
{
	ImGui::SameLine(12.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(1.0f, 1.0f, 1.0f, 0.0f));
	ImGui::Button(strName, ImVec2((windowSize.x - 25.0f) / 4.0f, (windowSize.y / 4.0f - 18.0f) / 8.0f));
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
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 280.0f * scaling);
	ImGui::PushItemWidth(280.0f * scaling);
	ImGui::Text("x:%.4f y:%.4f LC:%u RC:%u", vMouse.x, vMouse.y, (int)vMouse.z, (int)vMouse.w);
	ImGui::PopItemWidth();

	// Get time
	time_t now = time(0);
	struct tm ltm;
	localtime_s(&ltm, &now);
	int hour = 1 + ltm.tm_hour;
	int min = 1 + ltm.tm_min;
	int sec = 1 + ltm.tm_sec;

	ImGui::Text("Date:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 210.0f * scaling);
	ImGui::PushItemWidth(210.0f * scaling);
	ImGui::Text("%u:%u:%u %u seconds", ltm.tm_mday, 1 + ltm.tm_mon, 1900 + ltm.tm_year, ((hour * 60 + min) * 60) + sec);
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
	ImGui::SameLine(size.x - 225.0f);
	ImGui::PushItemWidth(100.0f);
	ImGui::InputInt("x", &xRes, 0, 100, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::PopItemWidth();

	int yRes = desc.Height;
	ImGui::SameLine(size.x - 108.0f);
	ImGui::PushItemWidth(100.0f);
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
	ImGui::SameLine(size.x - 217.0f - 8.0f);
	ImGui::PushItemWidth(217.0f);
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
	ImGui::SameLine(size.x - 217.0f - 8.0f);
	ImGui::PushItemWidth(217.0f);
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
				yRes = 1024;
				break;
			case 8:
				xRes = 1360;
				yRes = 768;
				break;
			case 9:
				xRes = 1366;
				yRes = 768;
				break;
			case 10:
				xRes = 1440;
				yRes = 900;
				break;
			case 11:
				xRes = 1536;
				yRes = 864;
				break;
			case 12:
				xRes = 1600;
				yRes = 900;
				break;
			case 13:
				xRes = 1680;
				yRes = 1050;
				break;
			case 14:
				xRes = 1920;
				yRes = 1080;
				break;
			case 15:
				xRes = 1920;
				yRes = 1200;
				break;
			case 16:
				xRes = 2560;
				yRes = 1080;
				break;
			case 17:
				xRes = 2560;
				yRes = 1440;
				break;
			case 18:
				xRes = 3440;
				yRes = 1440;
				break;
			case 19:
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

//--------------------------------------------------------------------------------------
// Windows
//--------------------------------------------------------------------------------------

void ControlWindow(
	ID3D11Texture2D* pRenderTargetTexture,
	Resource* pResource,
	ImVec4& clearColor,
	Buffer* pBuffer,
	TextureLib* pTextureLib,
	ImGuiEnum::DefaultEditor& defaultEditor,
	ImGuiEnum::AspectRatio& aspectRatio,
	ImGuiEnum::Resolution& resolution,
	DirectX::XMFLOAT4 vMouse,
	std::string& strProj,
	bool& bPause, bool& bAutoReload,
	float& fGameT, float playSpeed, 
	float fDeltaT, float fScaling,
	int iFrame, int& buttonPress,
	bool& bResChanged, bool& bNewProj,
	bool& bDefaultEditorSet,
	ImVec4& vWindowInfo,
	ImVec4& vMainWindowInfo
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
	
	std::string basePath = std::string("..\\..\\ShaderToyLibrary\\");
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
		strProj = std::string(newStrProj);

		ImGui::Separator();
		if (ButtonLine("Cancel", windowSize, itemSpacing, buttonWidth, pos, fScaling, 2.0f))
			newClicked = false;
		if (ButtonLine("New", windowSize, itemSpacing, buttonWidth, pos, fScaling, 2.0f))
		{
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

	std::string defProjPath = "../../ShaderToyLibrary";
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

	if (ImGui::Button("Stop", ImVec2(100.0f, 25.0f)))
	{
		bPause = true;
		fGameT = 0.0f;
	}
	ImGui::SameLine();
	const char* playButtonLabel = "Pause";
	if (bPause)
		playButtonLabel = "Play";

	if (ImGui::Button(playButtonLabel, ImVec2(100.0f, 25.0f)))
		bPause = !bPause;

	ImGui::SameLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 150.0f);
	ImGui::PushItemWidth(150.0f);
	ImGui::Text("Play Speed:");
	ImGui::PopItemWidth();

	const float ItemSpacing = ImGui::GetStyle().ItemSpacing.x;

	if (ImGui::Button("Reload Shaders", ImVec2(200.0f + ItemSpacing, 25.0f)))
	{
		buttonPress += 0x0001;
	}

	ImGui::SameLine();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 150.0f);
	ImGui::PushItemWidth(150.0f);
	ImGui::InputFloat("playback", &playSpeed, 0.01f);
	ImGui::PopItemWidth();


	ImGui::Separator();
	// Default editor picker
	const char* editorItems[] = { "Default", "Notepad", "Notepad++", "Custom" };
	int item_current = static_cast<int>(defaultEditor);
	ImGui::PushItemWidth(0.0f);
	ImGui::Text("Default Editor:");
	ImGui::PopItemWidth();
	ImGui::SameLine(vWindowInfo.z - 217.0f - 8.0f);
	ImGui::PushItemWidth(217.0f);
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
	ImGui::ColorEdit3("clear color", (float*)&clearColor); // Edit 3 floats representing a color

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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


	ImGui::End();
}

void MainImageWindow(
	ID3D11ShaderResourceView* pShaderResourceView,
	Resource* pResource,
	Buffer* pBuffer,
	ImVec2& vWindowSize,
	ImVec2& vCurrentWindowSize,
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

	windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;

	bTrackMouse = false;

	if (ImGui::IsWindowHovered())
		bTrackMouse = true;

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
	if (ImGui::Button("Fullscreen"))
		bFullscreen = !bFullscreen;

	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(1.0f, 1.0f, 1.0f, 0.0f));
	ImGui::SetCursorScreenPos(ImVec2(position.x + 10.0f, position.y + vCurrentWindowSize.y - 30.0f));

	std::string path = std::string("..\\..\\ShaderToyLibrary\\");
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

	ImVec2 windowSize = ImGui::GetWindowSize();
	bool pressed = false;
	static int iRightIdentifier = -1;
	static const char* selected = "";

	// Setting the right image button resource
	for (int i = 0; i < 4; ++i)
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
			bool pressed = ImGui::ImageButton(pBuffer[index].m_pShaderResourceView, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
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
			OverlayLabel(strName.c_str(), windowSize);
		}
		else
		if (bufferType == Channels::ChannelType::E_Texture)
		{
			if (iPadding == 0)
			{
				// Main Image
				pressed = ImGui::ImageButton(pResource[i].m_pShaderResource, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
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
				pressed = ImGui::ImageButton(pBuffer[iPadding - 1].m_Res[i].m_pShaderResource, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
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
			OverlayLabel(strName.c_str(), windowSize);
		}
		else
		{
			ImGui::ImageButton(nullptr, ImVec2(windowSize.x - 22.0f * scaling, (windowSize.y) / 4.0f - 18.0f * scaling), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
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
				std::replace(highlighted.begin(), highlighted.end(), '/', '\\');
				std::string cmd = std::string("explorer.exe /select," + highlighted);
				system(cmd.c_str());
			}
		}
		if (ImGui::BeginMenu("Input Source"))
		{
			if (ImGui::MenuItem("None"))
				iChangeInput = 5;
			if (ImGui::MenuItem("Texture"))
				iChangeInput = 0;
			if (ImGui::MenuItem("BufferA"))
				iChangeInput = 1;
			if (ImGui::MenuItem("BufferB"))
				iChangeInput = 2;
			if (ImGui::MenuItem("BufferC"))
				iChangeInput = 3;
			if (ImGui::MenuItem("BufferD"))
				iChangeInput = 4;

			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	if (iChangeInput >= 0)
	{
		std::string path = std::string("..\\..\\ShaderToyLibrary\\");

		if (iPadding == 0)
		{
			if (iChangeInput == 0)
			{
				pChannel[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_Texture;
			}
			else if (iChangeInput == 5)
			{
				pChannel[iRightIdentifier - 1].m_Type = Channels::ChannelType::E_None;
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
			std::string path = std::string("..\\..\\ShaderToyLibrary\\");
			strcpy(pChannel[iPressIdentifier - 1].m_strTexture, fileName.c_str());
			if (iPadding == 0)
			{
				path = path + std::string(strProj) + std::string("\\channels\\channels.txt");
				WriteChannel(path.c_str(), pChannel);
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
	}

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

	ImGui::SetWindowSize(size);

	ImGui::End();
}