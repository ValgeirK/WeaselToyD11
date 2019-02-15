#include "Shader.h"

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <string>
#include <fstream>

#include "HelperFunction.h"
#include "type/ConstantBuffer.h"
#include "type/HashDefines.h"

HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, std::vector<std::string>& errorList)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;

	hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			std::string str = std::string(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			SplitString(str, errorList, '\n');
			pErrorBlob->Release();
		}

		D3DCompileFromFile(L"shaders/DefaultPixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
			dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

HRESULT ScanShaderForCustomizable(const char* strProj, std::vector<CustomizableBuffer>& vCustomizableBuffer)
{
	// Open file
	std::string path = std::string(PROJECT_PATH) + strProj + std::string("/shaders/PixelShader.hlsl");
	std::ifstream inputFile(path.c_str(), std::ios::in);

	// Make sure the file was opened
	if (!inputFile)
	{
		return false;
	}

	vCustomizableBuffer.clear();

	// Initialize variables
	char line[MAX_PATH] = "";
	char dummy[MAX_PATH] = "";
	char dummy2[MAX_PATH] = "";
	char dummy3[MAX_PATH] = "";
	char bufferName[MAX_PATH] = "";

	do
	{
		// Finding the line where the constant buffer is set
		sscanf(line, "%s %s\n", &dummy, &bufferName);
	} while (inputFile.getline(line, MAX_PATH) && strcmp(bufferName, "cbCustomisable") != 0);

	// Skipping the line with the curly brackets
	inputFile.getline(line, MAX_PATH);

	if (strcmp(bufferName, "cbCustomisable") != 0)
		return S_OK;

	do
	{
		char strCommand[MAX_PATH] = "";
		float min = 0.0f;
		float max = 1.0f;
		float step = 1.0f;
		char strVarName[MAX_PATH] = "";
		char strMin[MAX_PATH] = "";
		char strMax[MAX_PATH] = "";
		char strStep[MAX_PATH] = "";

		CustomizableBuffer cb;

		int check = sscanf(line, "%s %s", &dummy, &strCommand);

		if (strcmp(strCommand, "slider") == 0)
		{
			sscanf(line, "%s %s %s %s %f %s %f \n", &dummy, &strCommand, &strVarName, &strMin, &min, &strMax, &max);
			cb.min = min;
			cb.max = max;
			cb.step = step;

			strcpy(cb.strCommand, strCommand);
			strcpy(cb.strVariable, strVarName);

			vCustomizableBuffer.push_back(cb);
		}
		else if (strcmp(strCommand, "input") == 0)
		{
			sscanf(line, "%s %s %s %s %f \n", &dummy, &strCommand, &strVarName, &strStep, &step);
			cb.min = min;
			cb.max = max;
			cb.step = step;

			strcpy(cb.strCommand, strCommand);
			strcpy(cb.strVariable, strVarName);

			vCustomizableBuffer.push_back(cb);
		}
		else if (strcmp(strCommand, "colorEdit") == 0
			|| strcmp(strCommand, "colourEdit") == 0)
		{
			sscanf(line, "%s %s %s \n", &dummy, &strCommand, &strVarName);
			cb.min = min;
			cb.max = max;
			cb.step = step;

			strcpy(cb.strCommand, strCommand);
			strcpy(cb.strVariable, strVarName);

			vCustomizableBuffer.push_back(cb);
		}


		inputFile.getline(line, MAX_PATH);
	} while (strcmp(dummy, "};") != 0);

	return S_OK;
}

void ShaderReflectionAndPopulation(
	ID3D11ShaderReflection* pPixelShaderReflection, 
	std::vector<CustomizableBuffer>& vCustomizableBuffer, 
	UINT& iCustomizableBufferSize,
	std::vector<CustomizableBuffer> vCustomizableBufferCopy
)
{
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
							if (strcmp(vCustomizableBufferCopy[j].strVariable, vCustomizableBuffer[i].strVariable) == 0 && vCustomizableBuffer[i].isDataSet)
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

				if(!isFound)
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
							if((float*)varDesc.DefaultValue == nullptr)
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
							if((int*)varDesc.DefaultValue == nullptr)
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