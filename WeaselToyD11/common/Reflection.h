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
	static HRESULT D3D11ReflectionSetup(
		ID3DBlob*							pBlob, 
		ID3D11ShaderReflection**			ppPixelShaderReflection
	);

	static void D3D11ShaderReflectionAndPopulation(
		ID3D11ShaderReflection*				pPixelShaderReflection,
		std::vector<CustomizableBuffer>&	rvCustomizableBuffer,
		UINT&								uCustomizableBufferSize,
		std::vector<CustomizableBuffer>*	vCustomizableBufferCopy = nullptr
	);

	static void D3D11ShaderReflection(
		ID3D11ShaderReflection*				pPixelShaderReflection, 
		Resource*							pResource, 
		DWORD								dwShaderFlags
	);
};
