#pragma once

#if defined(_DEBUG) || defined(PROFILE)
#define USE_DEBUG_MARKERS
#endif

#include <vector>
#include <string.h>

typedef unsigned int UINT;
typedef long HRESULT;

struct ImVec4;

struct ID3D11DeviceChild;
struct ID3D11Device;
struct ID3D11Buffer;

//--------------------------------------------------------------------------------------
// Helper Functions
//--------------------------------------------------------------------------------------

void SetDebugObjectName(ID3D11DeviceChild* pResource, const char * pName);

void SplitString(const std::string& inStr, std::vector<std::string>& container, char delim);

void ImGuiScaleMove(ImVec4& vWindow, float xScale, float yScale);

void KeepImGuiWindowsInsideApp(RECT rect, ImVec4& vWindow, bool& bResChanged);

void DefaultEditorStrFix(std::string& str);

HRESULT CreateCustomizableBuffer(ID3D11Device* pDevice, ID3D11Buffer** ppBuffer, UINT uBufferSize);

void GetFileExtension(const char* strPath, const char** strExtension);

void* AddAlphaMask(void* pData, int iTextureSize);

void DeleteArrayWrapper(void* block);