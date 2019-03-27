#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>

#include "Reflection.h"
#include "type/Channel.h"
#include "type/Resource.h"
#include "type/HashDefines.h"
#include "type/ConstantBuffer.h"

HRESULT Reflection::D3D11ReflectionSetup(ID3DBlob* pPSBlob, ID3D11ShaderReflection** ppPixelShaderReflection)
{
	assert(pPSBlob != nullptr);
	HRESULT hr = S_OK;

	hr = D3DReflect(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)ppPixelShaderReflection);

	return hr;
}

void Reflection::D3D11ShaderReflectionAndPopulation(
	ID3D11ShaderReflection*				pPixelShaderReflection,
	std::vector<CustomizableBuffer>&	rvCustomizableBuffer,
	UINT&								uCustomizableBufferSize,
	std::vector<CustomizableBuffer>* 	pCustomizableBufferCopy
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

				for (int i = 0; i < rvCustomizableBuffer.size(); ++i)
				{
					if (strcmp(varDesc.Name, rvCustomizableBuffer[i].m_strVariable) == 0)
					{
						// variable was found with a command in shader
						uCustomizableBufferSize = varDesc.StartOffset + varDesc.Size;
						rvCustomizableBuffer[iVariables].m_uOffset = varDesc.StartOffset;

						int copyIndex = -1;

						if (pCustomizableBufferCopy != nullptr)
						{
							for (int j = 0; j < pCustomizableBufferCopy->size(); ++j)
							{
								if (strcmp(pCustomizableBufferCopy->at(j).m_strVariable, rvCustomizableBuffer[i].m_strVariable) == 0 && pCustomizableBufferCopy->at(i).m_bIsDataSet)
								{
									// we want to keep the values that have possibly been altered
									pCustomizableBufferCopy->at(j).m_bIsDataSet = true;
									copyIndex = j;
								}
							}
						}

						if (typeDesc.Type == D3D_SVT_FLOAT)
						{
							rvCustomizableBuffer[iVariables].m_pData = new float[varDesc.Size / 4];
							rvCustomizableBuffer[iVariables].m_bIsDataSet = true;

							for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
							{
								if (copyIndex >= 0)
									((float*)rvCustomizableBuffer[iVariables].m_pData)[i] = ((float*)(pCustomizableBufferCopy->at(copyIndex)).m_pData)[i];
								else
								{
									if (varDesc.DefaultValue != nullptr)
										((float*)rvCustomizableBuffer[iVariables].m_pData)[i] = ((float*)varDesc.DefaultValue)[i];
									else
										((float*)rvCustomizableBuffer[iVariables].m_pData)[i] = 0.0f;
								}
							}
						}
						else if (typeDesc.Type == D3D_SVT_INT)
						{
							rvCustomizableBuffer[iVariables].m_pData = new int[varDesc.Size / 4];
							rvCustomizableBuffer[iVariables].m_bIsDataSet = true;

							for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
							{
								if (copyIndex >= 0)
									((int*)rvCustomizableBuffer[iVariables].m_pData)[i] = ((int*)(pCustomizableBufferCopy->at(copyIndex)).m_pData)[i];
								else
								{
									if (varDesc.DefaultValue != nullptr)
										((int*)rvCustomizableBuffer[iVariables].m_pData)[i] = ((int*)varDesc.DefaultValue)[i];
									else
										((int*)rvCustomizableBuffer[iVariables].m_pData)[i] = 0;
								}
							}
						}
						else
						{
							// Currently only supporting floats and ints
							assert(typeDesc.Type == D3D_SVT_FLOAT || typeDesc.Type == D3D_SVT_INT);
						}

						rvCustomizableBuffer[iVariables].m_iType = static_cast<int>(typeDesc.Type);
						rvCustomizableBuffer[iVariables].m_iSize = varDesc.Size;

						isFound = true;
					}
				}

				if (!isFound)
				{
					// variable was not found with a command in the shader
					uCustomizableBufferSize = varDesc.StartOffset + varDesc.Size;

					CustomizableBuffer cb;
					strcpy(cb.m_strCommand, "input");
					strcpy(cb.m_strVariable, varDesc.Name);
					cb.m_uOffset = varDesc.StartOffset;
					cb.m_iSize = varDesc.Size;

					if (typeDesc.Type == D3D_SVT_FLOAT)
					{
						cb.m_pData = new float[varDesc.Size / 4];
						cb.m_bIsDataSet = true;

						for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
						{
							cb.m_fStep = 0.1f;
							if ((float*)varDesc.DefaultValue == nullptr)
								((float*)cb.m_pData)[i] = 1.0f;
							else
								((float*)cb.m_pData)[i] = ((float*)varDesc.DefaultValue)[i];
						}
					}
					else if (typeDesc.Type == D3D_SVT_INT)
					{
						cb.m_pData = new int[varDesc.Size / 4];
						cb.m_bIsDataSet = true;

						for (unsigned int i = 0; i < varDesc.Size / 4; ++i)
						{
							cb.m_fStep = 1.0f;
							if ((int*)varDesc.DefaultValue == nullptr)
								((int*)cb.m_pData)[i] = 1;
							else
								((int*)cb.m_pData)[i] = ((int*)varDesc.DefaultValue)[i];
						}
					}
					cb.m_iType = static_cast<int>(typeDesc.Type);

					rvCustomizableBuffer.push_back(cb);
				}
			}
		}
	}
}

void Reflection::D3D11ShaderReflection(
	ID3D11ShaderReflection*		pPixelShaderReflection, 
	Resource*					pResource, 
	DWORD						dwShaderFlags)
{
	assert(pPixelShaderReflection != nullptr);
	assert(pResource != nullptr);

	HRESULT hr;

	D3D11_SHADER_DESC shaderReflectionDesc;
	hr = pPixelShaderReflection->GetDesc(&shaderReflectionDesc);

	int samplerCounter = 0;
	int textureCounter = 0;
	int samplerSlot[MAX_RESORCESCHANNELS];
	int textureSlot[MAX_RESORCESCHANNELS];

	// Setting the normal values, which matches with the usual d3d11 setup of textures and samplers
	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		samplerSlot[i] = i;
		textureSlot[i] = i;
	}

	// When we want backwards compatibility turned on we need to rearrange the texture and sampler bind slots
	if (dwShaderFlags & D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY)
	{
		D3D11_SHADER_INPUT_BIND_DESC bindDescArray[D3D10_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
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