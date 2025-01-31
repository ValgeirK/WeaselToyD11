#pragma once

#include <vector>
#include <string.h>

struct ID3D10Blob;
struct CustomizableBuffer;

typedef long HRESULT;
typedef wchar_t WCHAR;
typedef char CHAR;
typedef const CHAR* LPCSTR;
typedef ID3D10Blob ID3DBlob;
typedef unsigned long DWORD;

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(
	const WCHAR*					szFileName, 
	LPCSTR							szEntryPoint, 
	LPCSTR							szShaderModel, 
	ID3DBlob**						ppBlobOut,
	DWORD							dwShaderFlags,
	std::vector<std::string>&		errorList
);

HRESULT ScanShaderForCustomizable(
	const char* strProj, 
	const std::string& strShaderPath,
	std::vector<CustomizableBuffer>& vCustomizableBuffer
);