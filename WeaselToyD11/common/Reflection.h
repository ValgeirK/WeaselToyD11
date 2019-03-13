#pragma once

#include <vector>

struct ID3D11ShaderReflection;
struct ID3D10Blob;
struct CustomizableBuffer;

struct Resource;

typedef long HRESULT;
typedef ID3D10Blob ID3DBlob;

class Reflection
{
public:
	static HRESULT D3D11ReflectionSetup(ID3DBlob* blob, ID3D11ShaderReflection** pPixelShaderReflection);

	static void D3D11ShaderReflectionAndPopulation(
		ID3D11ShaderReflection* pPixelShaderReflection,
		std::vector<CustomizableBuffer>& vCustomizableBuffer,
		UINT& iCustomizableBufferSize,
		std::vector<CustomizableBuffer> vCustomizableBufferCopy
	);

	static void D3D11ShaderReflection(ID3D11ShaderReflection* pPixelShaderReflection, Resource* pResource, DWORD dwShaderFlags);
};
