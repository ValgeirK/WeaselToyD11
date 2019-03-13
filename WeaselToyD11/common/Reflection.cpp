#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>

#include "Reflection.h"
#include "type/Channel.h"
#include "type/Resource.h"
#include "type/HashDefines.h"
#include "type/ConstantBuffer.h"

HRESULT Reflection::D3D11ReflectionSetup(ID3DBlob* pPSBlob, ID3D11ShaderReflection** pPixelShaderReflection)
{
	assert(pPSBlob != nullptr);
	HRESULT hr = S_OK;

	hr = D3DReflect(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)pPixelShaderReflection);

	return hr;
}

void Reflection::D3D11ShaderReflectionAndPopulation(
	ID3D11ShaderReflection* pPixelShaderReflection,
	std::vector<CustomizableBuffer>& vCustomizableBuffer,
	UINT& iCustomizableBufferSize,
	std::vector<CustomizableBuffer> vCustomizableBufferCopy
)
{
	assert(pPixelShaderReflection != nullptr);
	D3D11_SHADER_DESC desc;
	pPixelShaderReflection->GetDesc(&desc);

	for (unsigned int iConstant = 0; iConstant < desc.ConstantBuffers; ++iConstant)
	{
		ID3D11ShaderReflectionConstantBuffer* pConstantBuffer = pPixelShaderReflection->GetConstantBufferByIndex(iConstant);

		// bufferDesc holds the name of the buffer and how many variables it holds
		D3D11_SHADER_BUFFER_DESC bufferDesc;
		pConstantBuffer->GetDesc(&bufferDesc);

		if (strcmp(bufferDesc.Name, "cbCustomisable") == 0)
		{
			for (unsigned int iVariables = 0; iVariables < bufferDesc.Variables; ++iVariables)
			{
				ID3D11ShaderReflectionVariable* pVar = pConstantBuffer->GetVariableByIndex(iVariables);

				D3D11_SHADER_VARIABLE_DESC varDesc;
				pVar->GetDesc(&varDesc);

				ID3D11ShaderReflectionType* newType = pVar->GetType();

				D3D11_SHADER_TYPE_DESC typeDesc;
				newType->GetDesc(&typeDesc);

				bool isFound = false;

				for (int i = 0; i < vCustomizableBuffer.size(); ++i)
				{
					if (strcmp(varDesc.Name, vCustomizableBuffer[i].strVariable) == 0)
					{
						// variable was found with a command in shader
						iCustomizableBufferSize = varDesc.StartOffset + varDesc.Size;
						vCustomizableBuffer[iVariables].offset = varDesc.StartOffset;

						int copyIndex = -1;

						for (int j = 0; j < vCustomizableBufferCopy.size(); ++j)
						{
							if (strcmp(vCustomizableBufferCopy[j].strVariable, vCustomizableBuffer[i].strVariable) == 0 && vCustomizableBufferCopy[i].isDataSet)
							{
								// we want to keep the values that have possibly been altered
								vCustomizableBufferCopy[j].isDataSet = true;
								copyIndex = j;
							}
						}

						if (typeDesc.Type == D3D_SVT_FLOAT)
						{
							vCustomizableBuffer[iVariables].data = new float[varDesc.Size / 4];
							vCustomizableBuffer[iVariables].isDataSet = true;

							for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
							{
								if (copyIndex >= 0)
									((float*)vCustomizableBuffer[iVariables].data)[i] = ((float*)(vCustomizableBufferCopy[copyIndex]).data)[i];
								else
								{
									if (varDesc.DefaultValue != nullptr)
										((float*)vCustomizableBuffer[iVariables].data)[i] = ((float*)varDesc.DefaultValue)[i];
									else
										((float*)vCustomizableBuffer[iVariables].data)[i] = 0.0f;
								}
							}
						}
						else if (typeDesc.Type == D3D_SVT_INT)
						{
							vCustomizableBuffer[iVariables].data = new int[varDesc.Size / 4];
							vCustomizableBuffer[iVariables].isDataSet = true;

							for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
							{
								if (copyIndex >= 0)
									((int*)vCustomizableBuffer[iVariables].data)[i] = ((int*)(vCustomizableBufferCopy[copyIndex]).data)[i];
								else
								{
									if (varDesc.DefaultValue != nullptr)
										((int*)vCustomizableBuffer[iVariables].data)[i] = ((int*)varDesc.DefaultValue)[i];
									else
										((int*)vCustomizableBuffer[iVariables].data)[i] = 0;
								}
							}
						}
						else
						{
							// Currently only supporting floats and ints
							assert(typeDesc.Type == D3D_SVT_FLOAT || typeDesc.Type == D3D_SVT_INT);
						}

						vCustomizableBuffer[iVariables].type = static_cast<int>(typeDesc.Type);
						vCustomizableBuffer[iVariables].size = varDesc.Size;

						isFound = true;
					}
				}

				if (!isFound)
				{
					// variable was not found with a command in the shader
					iCustomizableBufferSize = varDesc.StartOffset + varDesc.Size;

					CustomizableBuffer cb;
					strcpy(cb.strCommand, "input");
					strcpy(cb.strVariable, varDesc.Name);
					cb.offset = varDesc.StartOffset;
					cb.size = varDesc.Size;

					if (typeDesc.Type == D3D_SVT_FLOAT)
					{
						cb.data = new float[varDesc.Size / 4];
						cb.isDataSet = true;

						for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
						{
							cb.step = 0.1f;
							if ((float*)varDesc.DefaultValue == nullptr)
								((float*)cb.data)[i] = 1.0f;
							else
								((float*)cb.data)[i] = ((float*)varDesc.DefaultValue)[i];
						}
					}
					else if (typeDesc.Type == D3D_SVT_INT)
					{
						cb.data = new int[varDesc.Size / 4];
						cb.isDataSet = true;

						for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
						{
							cb.step = 1.0f;
							if ((int*)varDesc.DefaultValue == nullptr)
								((int*)cb.data)[i] = 1;
							else
								((int*)cb.data)[i] = ((int*)varDesc.DefaultValue)[i];
						}
					}
					cb.type = static_cast<int>(typeDesc.Type);

					vCustomizableBuffer.push_back(cb);
				}
			}
		}
	}
}

void Reflection::D3D11ShaderReflection(ID3D11ShaderReflection* pPixelShaderReflection, Resource* pResource, DWORD dwShaderFlags)
{
	assert(pPixelShaderReflection != nullptr);

	HRESULT hr;

	D3D11_SHADER_DESC shaderReflectionDesc;
	hr = pPixelShaderReflection->GetDesc(&shaderReflectionDesc);

	int samplerCounter = 0;
	int textureCounter = 0;
	int samplerSlot[MAX_RESORCESCHANNELS] = {};
	int textureSlot[MAX_RESORCESCHANNELS] = {};

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		samplerSlot[i] = i;
		textureSlot[i] = i;
	}

	if (dwShaderFlags & D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY)
	{
		D3D11_SHADER_INPUT_BIND_DESC bindDescArray[128];
		for (unsigned int param = 0; param < shaderReflectionDesc.BoundResources; ++param)
		{
			hr = pPixelShaderReflection->GetResourceBindingDesc(param, &(bindDescArray[param]));

			if (bindDescArray[param].Type == D3D_SIT_SAMPLER)
				samplerSlot[samplerCounter++] = bindDescArray[param].BindPoint;

			if (bindDescArray[param].Type == D3D_SIT_TEXTURE)
				textureSlot[textureCounter++] = bindDescArray[param].BindPoint;

			assert(hr == S_OK);
		}

		assert(samplerCounter == textureCounter);

		for (int i = 0; i < samplerCounter; ++i)
		{
			pResource[samplerSlot[i]].m_iTextureSlot = textureSlot[i];
		}
	}
	else
	{
		for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
		{
			pResource[samplerSlot[i]].m_iTextureSlot = textureSlot[i];
		}
	}
}