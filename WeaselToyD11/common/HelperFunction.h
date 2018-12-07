#pragma once

#if defined(_DEBUG) || defined(PROFILE)
#define USE_DEBUG_MARKERS
#endif

#include <windows.h>
#include <vector>
#include <string.h>

struct ImVec4;

struct ID3D11DeviceChild;

//--------------------------------------------------------------------------------------
// Helper Functions
//--------------------------------------------------------------------------------------

void SetDebugObjectName(ID3D11DeviceChild* pResource, const char * pName);

void SplitString(const std::string& inStr, std::vector<std::string>& container, char delim);

void ImGuiScaleMove(ImVec4& vWindow, float xScale, float yScale);

void KeepImGuiWindowsInsideApp(RECT rect, ImVec4& vWindow, bool& bResChanged);

void DefaultEditorStrFix(std::string&);