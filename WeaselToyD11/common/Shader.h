#pragma once

#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <string.h>

struct CustomizableBuffer;

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, std::vector<std::string>& errorList);

HRESULT ScanShaderForCustomizable(const char* strProj, std::vector<CustomizableBuffer>& vCustomizableBuffer);