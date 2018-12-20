#include <stdlib.h>
#include <stddef.h>
#include <ctime>

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "../lib/imgui.h"
#include "../lib/imgui_impl_dx11.h"
#include "../lib/imgui_impl_win32.h"

#include <vector>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "HelperFunction.h"

void SetDebugObjectName(ID3D11DeviceChild* pResource, const char * pName)
{
#ifdef USE_DEBUG_MARKERS
	size_t NumChars = strlen(pName);
	//pResource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)NumChars+1, pName);
#else
	UNREFERENCED_PARAMETER(resource);
	UNREFERENCED_PARAMETER(name);
#endif // USE_DEBUG_MARKERS
}

void SplitString(const std::string& inStr, std::vector<std::string>& container, char delim)
{
	std::stringstream ss(inStr);
	std::string token;
	while (std::getline(ss, token, delim)) {
		container.push_back(token);
	}
}

void KeepImGuiWindowsInsideApp(RECT rect, ImVec4& vWindow, bool& bResChanged)
{
	// Checking if x coord is outside
	if (vWindow.x + vWindow.z > rect.right - rect.left)
	{
		bResChanged = true;
		vWindow.x = (rect.right - rect.left) - vWindow.z;
	}
	if (vWindow.x < 0.0f)
	{
		bResChanged = true;
		vWindow.x = 0.0f;
	}

	// Check if y coord is outside
	if (vWindow.y + vWindow.w > rect.bottom - rect.top)
	{
		bResChanged = true;
		vWindow.y = (rect.bottom - rect.top) - vWindow.w;
	}
	if (vWindow.y < 0.0f)
	{
		bResChanged = true;
		vWindow.y = 0.0f;
	}
}

void ImGuiScaleMove(ImVec4& vWindow, float xScale, float yScale)
{
	vWindow.x *= xScale;
	vWindow.y *= yScale;
	vWindow.z *= xScale;
	vWindow.w *= yScale;
}

void DefaultEditorStrFix(std::string& str)
{
	std::string newStr = "";
	std::string word = "";
	bool hasSpace = false;
	for (int i = 0; i < str.length(); ++i)
	{
		if (str.at(i) == '\\')
		{
			if (word.length() > 0)
			{
				if(hasSpace)
					newStr += std::string("\"") + word + std::string("\"") + std::string("\\");
				else
					newStr += word + std::string("\\");
			}

			word = "";
			hasSpace = false;
		}
		else
			word += str.at(i);

		if (str.at(i) == ' ')
			hasSpace = true;
	}

	str = newStr + word;
}