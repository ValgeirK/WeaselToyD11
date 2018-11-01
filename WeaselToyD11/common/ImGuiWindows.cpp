#include "ImGuiWindows.h"

#include "../lib/imgui.h"
#include "../lib/imgui_impl_dx11.h"
#include "../lib/imgui_impl_win32.h"

#include <d3d11.h>
#include <ctime>
#include <string.h>
#include <process.h>

#include "Textures.h"
#include "HelperFunction.h"
#include "type/Resource.h"

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

void LineText(Resource* pResource, Buffer* pBuffer, int index)
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
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f);
	ImGui::PushItemWidth(100.0f);
	ImGui::Text("%u x %u", (int)x, (int)y);
	ImGui::PopItemWidth();
}

void BufferLineText(Buffer* pBuffer, int iResourceIndex, int iChannelIndex)
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
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 150.0f);
	ImGui::PushItemWidth(150.0f);
	ImGui::Text("%u x %u", (int)pBuffer[iResourceIndex].m_Res[iChannelIndex].m_vChannelRes.x, (int)pBuffer[iResourceIndex].m_Res[iChannelIndex].m_vChannelRes.y);
	ImGui::PopItemWidth();
}

void RecursiveLineText(const char* strName, Buffer* pBuffer, int index)
{
	if (pBuffer[index].m_bIsActive && ImGui::TreeNode(strName))
	{	
		for(int i = 0; i < 4; ++i)
			BufferLineText(pBuffer, index, i);

		ImGui::TreePop();
	}
}

void ButtonLine(const char* strName, int& iPadding, ImVec2 vWindowSize, float fItemSpacing, float& fButtonWidth, float& fPos, int index, bool active, int hovered)
{
	fButtonWidth = (vWindowSize.x) / 5.0f - 10.0f;
	fPos += fButtonWidth + fItemSpacing;
	ImGui::SameLine(vWindowSize.x - fPos);
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

	if (ImGui::Button(strName, ImVec2(fButtonWidth, vWindowSize.y - 42.0f)))
		iPadding = index;

	if(useColor)
		ImGui::PopStyleColor();

	fButtonWidth = ImGui::GetItemRectSize().x;
}

void ErrorLine(std::string strName, ImGuiEnum::DefaultEditor defaultEditor)
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
		std::string path = strName.substr(0, indexStart + 5);

		if(defaultEditor == ImGuiEnum::DefaultEditor::E_NOTEPAD)
			system(std::string("notepad.exe " + path).c_str());
		if (defaultEditor == ImGuiEnum::DefaultEditor::E_NOTEPADPP)
			system(std::string("start notepad++ " + path + " -n" + ints[0]).c_str());
	}
}

void OverlayLabel(const char* strName, ImVec2 windowSize)
{
	ImGui::SameLine(12.0f);
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(1.0f, 1.0f, 1.0f, 0.0f));
	ImGui::Button(strName, ImVec2((windowSize.x - 25.0f) / 4.0f, (windowSize.y / 4.0f - 18.0f) / 8.0f));
	ImGui::PopStyleColor();
}

//--------------------------------------------------------------------------------------
// Windows
//--------------------------------------------------------------------------------------

void ControlWindow(
	bool& pause, float& gameT,
	float playSpeed, ImVec4 clearColor,
	int& buttonPress, ImGuiEnum::DefaultEditor& defaultEditor)
{
	ImGui::Begin("Control Panel");

	if (ImGui::Button("Stop", ImVec2(100.0f, 25.0f)))
	{
		pause = true;
		gameT = 0.0f;
	}
	ImGui::SameLine();
	const char* playButtonLabel = "Pause";
	if (pause)
		playButtonLabel = "Play";

	if (ImGui::Button(playButtonLabel, ImVec2(100.0f, 25.0f)))
		pause = !pause;

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

	const char* items[] = { "Notepad", "Notepad++" };
	static int item_current = static_cast<int>(defaultEditor);
	ImGui::PushItemWidth(150.0f);
	ImGui::Combo("Default Editor", &item_current, items, IM_ARRAYSIZE(items));
	ImGui::PopItemWidth();
	defaultEditor = static_cast<ImGuiEnum::DefaultEditor>(item_current);

	ImGui::Separator();
	ImGui::ColorEdit3("clear color", (float*)&clearColor); // Edit 3 floats representing a color

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();
}

void ConstantBufferInfoWindow(ID3D11Texture2D* pRenderTargetTexture, 
	Resource* pResource,
	Buffer* pBuffer,
	int iFrame, bool bPause,
	float fGameT, float fDeltaT,
	DirectX::XMFLOAT4 vMouse)
{
	ImGui::Begin("Constant Buffer Info");

	ImGui::Separator();
	ImGui::Separator();

	ImGui::Text("Never Changes");

	ImGui::Separator();


	ImGui::Text("Resolution:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f);
	D3D11_TEXTURE2D_DESC desc = {};
	pRenderTargetTexture->GetDesc(&desc);
	ImGui::PushItemWidth(100.0f);
	ImGui::Text("%u x %u", (int)desc.Width, (int)desc.Height);
	ImGui::PopItemWidth();

	ImGui::Spacing();
	ImGui::Text("ChanelResolution");

	for( int i = 0; i < 4; ++i)
		LineText(pResource, pBuffer, i);

	ImGui::Spacing();

	for (int i = 0; i < 4; ++i)
	{
		std::string label = "";
		BufferSwitchLookup(label, i, " Resource");
		RecursiveLineText(label.c_str(), pBuffer, i);
	}

	ImGui::Separator();
	ImGui::Separator();

	ImGui::Text("Changes Every Frame");

	ImGui::Separator();

	ImGui::Text("Frame:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f);
	ImGui::PushItemWidth(100.0f);
	ImGui::Text("%u", iFrame);
	ImGui::PopItemWidth();

	ImGui::Text("Time:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f);
	ImGui::PushItemWidth(100.0f);
	ImGui::Text("%.3f sec", !bPause ? fGameT + fDeltaT : fGameT);
	ImGui::PopItemWidth();

	ImGui::Text("Delta Time:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 100.0f);
	ImGui::PushItemWidth(100.0f);
	ImGui::Text("%.5f ms", fDeltaT * 1000.0f);
	ImGui::PopItemWidth();

	ImGui::Text("Mouse:");
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 280.0f);
	ImGui::PushItemWidth(280.0f);
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
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 210.0f);
	ImGui::PushItemWidth(210.0f);
	ImGui::Text("%u:%u:%u %u seconds", ltm.tm_mday, 1 + ltm.tm_mon, 1900 + ltm.tm_year, ((hour * 60 + min) * 60) + sec);
	ImGui::PopItemWidth();

	ImGui::Separator();
	ImGui::Separator();

	ImGui::End();
}

void MainImageWindow(
	ID3D11ShaderResourceView* pShaderResourceView,
	Resource* pResource,
	Buffer* pBuffer,
	ImVec2& vWindowSize,
	ImVec2& vCurrentWindowSize,
	ImGuiWindowFlags& windowFlags,
	int iPadding,
	bool& bTrackMouse
)
{
	ImGui::Begin("ShaderToy", 0, windowFlags);

	vWindowSize = ImGui::GetWindowSize();

	windowFlags = ImGuiWindowFlags_NoMove;

	bTrackMouse = false;

	if (ImGui::IsWindowHovered())
		bTrackMouse = true;

	if (ImGui::IsItemHovered())
	{
		windowFlags = 0;
		bTrackMouse = false;
	}

	const char* shaderName = "";

	if (iPadding == 0)
	{
		shaderName = "shaders/PixelShader.hlsl";
		ImGui::Image(pShaderResourceView, ImVec2(vCurrentWindowSize.x - 17, vCurrentWindowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));
	}
	else
	{
		if (pBuffer[iPadding - 1].m_bIsActive)
			ImGui::Image(pBuffer[iPadding - 1].m_pShaderResourceView, ImVec2(vCurrentWindowSize.x - 17, vCurrentWindowSize.y - 37), ImVec2(0, 0), ImVec2(1, 1));

		switch (iPadding)
		{
		case 1:
			shaderName = "shaders/PixelShaderA.hlsl";
			break;
		case 2:
			shaderName = "shaders/PixelShaderB.hlsl";
			break;
		case 3:
			shaderName = "shaders/PixelShaderC.hlsl";
			break;
		case 4:
			shaderName = "shaders/PixelShaderD.hlsl";
			break;
		}

	}

	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(1.0f, 1.0f, 1.0f, 0.0f));
	ImVec2 pos = ImGui::GetWindowPos();
	ImGui::SetCursorScreenPos(ImVec2(pos.x + 10.0f, pos.y + vCurrentWindowSize.y - 30.0f));
	ImGui::Button(shaderName);
	ImGui::PopStyleColor();

	ImGui::End();
}

void ViewToggleWindow(int& iPadding, Buffer* pBuffers, int iHovered)
{
	ImGui::Begin("View Toggle");

	const float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
	ImVec2 windowSize = ImGui::GetWindowSize();

	float pos = 0.0f;
	float buttonWidth = 0.0f;

	std::string strName = "";

	for (int i = 4; i >= 0; --i)
	{
		if (i == 0)
			ButtonLine("Main", iPadding, windowSize, itemSpacing, buttonWidth, pos, 0, true, iHovered);

		BufferSwitchLookup(strName, i - 1);
		ButtonLine(strName.c_str(), iPadding, windowSize, itemSpacing, buttonWidth, pos, i, pBuffers[i - 1].m_bIsActive, iHovered);
	}	

	ImGui::End();
}

void ResourceWindow(
	ID3D11DeviceContext* pContext, 
	Resource* pResource,
	Buffer* pBuffer,
	TextureLib* pTextureLib, 
	int& iPadding, int& iPressIdentifier, int& iHovered
)
{
	ImGui::Begin("Resources");

	ImVec2 windowSize = ImGui::GetWindowSize();

	for (int i = 0; i < 4; ++i)
	{
		int index = 0;
		bool isBuffer = false;
		if (iPadding == 0)
		{
			// Main image
			index = pResource[i].m_iBufferIndex;
			isBuffer = pResource[i].m_Type == Channels::ChannelType::E_Buffer ? true : false;
		}
		else
		{
			// Buffers
			index = pBuffer[iPadding - 1].m_Res[i].m_iBufferIndex;
			isBuffer = pBuffer[iPadding - 1].m_Res[i].m_Type == Channels::ChannelType::E_Buffer ? true : false;
		}

		if (isBuffer)
		{
			bool pressed = ImGui::ImageButton(pBuffer[index].m_pShaderResourceView, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			if (ImGui::IsItemHovered())
			{
				iHovered = index + 1;
			}

			if (pressed)
			{
				iPadding = index + 1;
			}

			std::string strName = "";
			BufferSwitchLookup(strName, index);
			OverlayLabel(strName.c_str(), windowSize);
		}
		else
		{
			bool pressed = false;
			if (iPadding == 0)
				// Main Image
				pressed = ImGui::ImageButton(pResource[i].m_pShaderResource, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
			else
				pressed = ImGui::ImageButton(pBuffer[iPadding - 1].m_Res[i].m_pShaderResource, ImVec2(windowSize.x - 25.0f, (windowSize.y) / 4.0f - 18.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

			if (pressed)
				iPressIdentifier = i + 1;

			std::string strName = "";
			TextureSwitchLookup(strName, i);
			OverlayLabel(strName.c_str(), windowSize);
		}
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
			std::string filePathName = "";
			std::string path = "";
			std::string filter = "";

			if (ImGuiFileDialog::Instance(pTextureLib)->IsOk == true)
			{
				filePathName = ImGuiFileDialog::Instance(pTextureLib)->GetFilepathName();
				path = ImGuiFileDialog::Instance(pTextureLib)->GetCurrentPath();
				fileName = ImGuiFileDialog::Instance(pTextureLib)->GetCurrentFileName();
				filter = ImGuiFileDialog::Instance(pTextureLib)->GetCurrentFilter();

				fileName = std::string("textures/" + fileName);
			}
			else
			{
				fileName = selected;
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

void ShaderErrorWindow(Buffer* pBuffer, ImGuiEnum::DefaultEditor defaultEditor)
{
	ImGui::Begin("Shader Error Messages");

	for (int i = 0; i < 4; ++i)
	{
		if (pBuffer[i].m_ShaderError.size() > 0)
		{
			for (int n = 0; n < pBuffer[i].m_ShaderError.size(); ++n)
			{
				ErrorLine(pBuffer[i].m_ShaderError[n], defaultEditor);
			}
		}
	}

	ImGui::End();
}