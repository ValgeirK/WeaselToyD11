#include "Buffer.h"

#include <stdlib.h>
#include <stddef.h>
#include <string>
#include <comdef.h>

#include "Shader.h"
#include "Loader.h"
#include "Textures.h"
#include "HelperFunction.h"
#include "TextureLib.h"
#include "Reflection.h"
#include "type/Channel.h"
#include "type/ConstantBuffer.h"
#include "type/HashDefines.h"

void Buffer::LoadTextures(
	ID3D11Device*			pd3dDevice,
	TextureLib*				pTextureLib,
	int*					piBufferUsed,
	const int				iWidth,
	const int				iHeight
	)
{
	assert(pd3dDevice != nullptr);
	assert(pTextureLib != nullptr);
	assert(piBufferUsed != nullptr);

	// Go through and create the textures that are needed and set them

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		Create2DTexture(pd3dDevice, &(m_ppTexture2DCopy[i]), nullptr, &(m_ppShaderResourceViewCopy[i]), iWidth, iHeight);

		assert(m_ppTexture2DCopy[i] != nullptr);
		assert(m_ppShaderResourceViewCopy[i] != nullptr);

		if (m_Channels[i].m_Type == Channels::ChannelType::E_Texture)
		{
			m_Res[i].m_Type = Channels::ChannelType::E_Texture;

			strcpy(m_Res[i].m_strTexture, m_Channels[i].m_strTexture);

			pTextureLib->GetTexture(m_Channels[i].m_strTexture, &m_Res[i].m_pShaderResource, m_Res[i].m_vChannelRes);
			assert(m_Res[i].m_pShaderResource != nullptr);
		}
		else if (m_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			m_Res[i].m_Type = Channels::ChannelType::E_Buffer;

			m_Res[i].m_vChannelRes = DirectX::XMFLOAT4((float)iWidth, (float)iHeight, 0.0f, 0.0f);
			m_Res[i].m_iBufferIndex = static_cast<int>(m_Channels[i].m_BufferId);

			piBufferUsed[m_Res[i].m_iBufferIndex] += 1;

			// There is a buffer that needs resizing
			m_bResizeBuffer = true;
		}
		else
		{
			m_Res[i].m_Type = Channels::ChannelType::E_None;
			m_Res[i].m_pShaderResource = nullptr;
		}

		// Create Sampler
		CreateSampler(pd3dDevice, &m_Res[i].m_pSampler, m_Channels[i], 0, i);
	}
}

HRESULT Buffer::InitBuffer(
	ID3D11Device*			pd3dDevice,
	ID3D11DeviceContext*	pImmediateContext,
	ID3D11VertexShader*		pVertShader,
	TextureLib*				pTextureLib,
	DWORD					dwShaderFlag,
	const char*				strProj,
	int*					piBufferUsed,
	const int				iWidth, 
	const int				iHeight, 
	int						iIndex
)
{
	HRESULT hr = S_OK;

	assert(pd3dDevice != nullptr);
	assert(pImmediateContext != nullptr);
	assert(pVertShader != nullptr);
	assert(pTextureLib != nullptr);

	// Reading in Channel file for this channel
	std::string projChannelPath = PROJECT_PATH + std::string(strProj);
	// Make room for characters
	std::wstring projPixelPathW(projChannelPath.length(), L' ');
	 // Copy string to wstring.
	std::copy(projChannelPath.begin(), projChannelPath.end(), projPixelPathW.begin());

	switch (iIndex)
	{
	case 0:
		projChannelPath += std::string("/channels/channelsA.txt");
		projPixelPathW += std::wstring(L"/shaders/PixelShaderBufferA.hlsl");
		break;
	case 1:
		projChannelPath += std::string("/channels/channelsB.txt");
		projPixelPathW += std::wstring(L"/shaders/PixelShaderBufferB.hlsl");
		break;
	case 2:
		projChannelPath += std::string("/channels/channelsC.txt");
		projPixelPathW += std::wstring(L"/shaders/PixelShaderBufferC.hlsl");
		break;
	case 3:
		projChannelPath += std::string("/channels/channelsD.txt");
		projPixelPathW += std::wstring(L"/shaders/PixelShaderBufferD.hlsl");
		break;
	}

	assert(pVertShader != nullptr);
	m_pVertexShader = pVertShader;

	// Compile the pixel shader
	ID3DBlob* pPSBufferBlob = nullptr;
	hr = CompileShaderFromFile(projPixelPathW.c_str(), "mainImage", PS_SHADER_COMPILE_VER, &pPSBufferBlob, dwShaderFlag, m_ShaderError);
	if (FAILED(hr))
	{
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			_bstr_t b(projPixelPathW.c_str());
			const char* c = b;
			std::string msg = "Error Pixel Shader \"" + std::string(c) + "\" not found!";

			SetForegroundWindow(nullptr);
			MessageBox(nullptr,
				(LPCSTR)msg.c_str(), (LPCSTR)"Error", MB_OK);
		}
	}

	hr = Create2DTexture(pd3dDevice, &m_pRenderTargetTexture, &m_pRenderTargetView, &m_pShaderResourceView, iWidth, iHeight);

	assert(m_pRenderTargetTexture != nullptr);
	assert(m_pRenderTargetView != nullptr);
	assert(m_pShaderResourceView != nullptr);

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBufferBlob->GetBufferPointer(), pPSBufferBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	assert(m_pPixelShader != nullptr);

	ID3D11ShaderReflection* pPixelShaderReflection = nullptr;
	Reflection::D3D11ReflectionSetup(pPSBufferBlob, &pPixelShaderReflection);
	assert(pPixelShaderReflection != nullptr);

	pPSBufferBlob->Release();
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(m_pPixelShader, "BufferPixelShader");

	if (!LoadChannels(projChannelPath.c_str(), m_Channels, m_iSize))
		return S_FALSE;

	// Load texture
	LoadTextures(pd3dDevice, pTextureLib, piBufferUsed, iWidth, iHeight);

	Reflection::D3D11ShaderReflection(pPixelShaderReflection, m_Res, dwShaderFlag);

	ScanShaderForCustomizable(strProj, std::string(projPixelPathW.begin(), projPixelPathW.end()), m_vCustomizableBuffer);

	Reflection::D3D11ShaderReflectionAndPopulation(pPixelShaderReflection, m_vCustomizableBuffer, m_uCustomizableBufferSize);

	hr = CreateCustomizableBuffer(pd3dDevice, &m_pCBCustomizable, m_uCustomizableBufferSize);
	assert(m_pCBCustomizable != nullptr);

	m_bIsActive = true;

	return hr;
}

void Buffer::ResizeTexture(ID3D11Device* pDevice, const UINT uWidth, const UINT uHeight)
{
	// Release the textures we have and create new ones using the right width and height
	assert(pDevice != nullptr);

	if (m_pRenderTargetTexture)m_pRenderTargetTexture->Release();
	if (m_pRenderTargetView)m_pRenderTargetView->Release();
	if (m_pShaderResourceView)m_pShaderResourceView->Release();

	Create2DTexture(pDevice, &m_pRenderTargetTexture, &m_pRenderTargetView, &m_pShaderResourceView, uWidth, uHeight);

	assert(m_pRenderTargetTexture != nullptr);
	assert(m_pRenderTargetView != nullptr);
	assert(m_pShaderResourceView != nullptr);

	for(int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (m_ppTexture2DCopy[i])m_ppTexture2DCopy[i]->Release();
		if (m_ppShaderResourceViewCopy[i])m_ppShaderResourceViewCopy[i]->Release();

		Create2DTexture(pDevice, &m_ppTexture2DCopy[i], nullptr, &m_ppShaderResourceViewCopy[i], uWidth, uHeight);

		assert(m_ppTexture2DCopy[i] != nullptr);
		assert(m_ppShaderResourceViewCopy[i] != nullptr);

		if (m_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			m_Res[i].m_vChannelRes.x = (float)uWidth;
			m_Res[i].m_vChannelRes.y = (float)uHeight;
		}
	}
}

void Buffer::ReloadTexture(
	ID3D11Device*			pd3dDevice,
	TextureLib*				pTextureLib,
	int*					piBufferUsed,
	const int				iWidth,
	const int				iHeight
)
{
	// release and then reload the textures
	assert(pd3dDevice != nullptr);
	assert(pTextureLib != nullptr);

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (m_Res[i].m_pSampler)
			m_Res[i].m_pSampler->Release();

		SAFE_RELEASE(m_ppShaderResourceViewCopy[i]);
		SAFE_RELEASE(m_ppTexture2DCopy[i]);
	}

	LoadTextures(pd3dDevice, pTextureLib, piBufferUsed, iWidth, iHeight);
}

HRESULT Buffer::ReloadShader(ID3D11Device* pd3dDevice, ID3D11VertexShader*  pVertShader, DWORD dwShaderFlag, const char* strProj, const int iIndex)
{
	assert(pd3dDevice != nullptr);
	assert(pVertShader != nullptr);

	HRESULT hr = S_OK;

	// Reading in Channel file for this channel
	std::string channelPath = PROJECT_PATH + std::string(strProj);
	// Make room for characters
	std::wstring pixelShaderPath(channelPath.length(), L' ');
	// Copy string to wstring.
	std::copy(channelPath.begin(), channelPath.end(), pixelShaderPath.begin());

	switch (iIndex)
	{
	case 0:
		channelPath += std::string("/channels/channelsA.txt");
		pixelShaderPath += std::wstring(L"/shaders/PixelShaderBufferA.hlsl");
		break;
	case 1:
		channelPath += std::string("/channels/channelsB.txt");
		pixelShaderPath += std::wstring(L"/shaders/PixelShaderBufferB.hlsl");
		break;
	case 2:
		channelPath += std::string("/channels/channelsC.txt");
		pixelShaderPath += std::wstring(L"/shaders/PixelShaderBufferC.hlsl");
		break;
	case 3:
		channelPath += std::string("/channels/channelsD.txt");
		pixelShaderPath += std::wstring(L"/shaders/PixelShaderBufferD.hlsl");
		break;
	}

	m_pVertexShader = pVertShader;

	// Compile the pixel shader
	ID3DBlob* pPSBufferBlob = nullptr;
	m_ShaderError.clear();
	hr = CompileShaderFromFile(pixelShaderPath.c_str(), "mainImage", PS_SHADER_COMPILE_VER, &pPSBufferBlob, dwShaderFlag, m_ShaderError);
	if (FAILED(hr))
	{
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			_bstr_t b(pixelShaderPath.c_str());
			const char* c = b;
			std::string msg = "Error Pixel Shader \"" + std::string(c) + "\" not found!";

			SetForegroundWindow(nullptr);
			MessageBox(nullptr,
				(LPCSTR)msg.c_str(), (LPCSTR)"Error", MB_OK);
		}
	}

	m_pPixelShader->Release();

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBufferBlob->GetBufferPointer(), pPSBufferBlob->GetBufferSize(), nullptr, &m_pPixelShader);

	assert(SUCCEEDED(hr) && "If compile goes through, this should not fail!");

	ID3D11ShaderReflection* pPixelShaderReflection = nullptr;
	Reflection::D3D11ReflectionSetup(pPSBufferBlob, &pPixelShaderReflection);
	assert(pPixelShaderReflection != nullptr);

	pPSBufferBlob->Release();
	if (FAILED(hr))
		return hr;

	Reflection::D3D11ShaderReflection(pPixelShaderReflection, m_Res, dwShaderFlag);

	std::vector<CustomizableBuffer> customizableBufferCopy;
	for (int i = 0; i < m_vCustomizableBuffer.size(); ++i)
	{
		// Copy the customizable variables so we can maintain the potentially 
		// modified values
		CustomizableBuffer cb;
		cb = m_vCustomizableBuffer[i];
		customizableBufferCopy.push_back(cb);
	}

	ScanShaderForCustomizable(strProj, std::string(pixelShaderPath.begin(), pixelShaderPath.end()), m_vCustomizableBuffer);

	Reflection::D3D11ShaderReflectionAndPopulation(pPixelShaderReflection, m_vCustomizableBuffer, m_uCustomizableBufferSize, &customizableBufferCopy);

	hr = CreateCustomizableBuffer(pd3dDevice, &m_pCBCustomizable, m_uCustomizableBufferSize);
	assert(m_pCBCustomizable != nullptr);

	return hr;
}

void Buffer::ClearShaderResource(ID3D11DeviceContext* pImmediateContext, ID3D11DepthStencilView* pDepthStencilView)
{
	assert(pImmediateContext != nullptr);
	assert(pDepthStencilView != nullptr);

	// Clearing the Shader Resource so it's not bound on clear
	ID3D11ShaderResourceView* renderNull = nullptr;
	for(int i = 0; i < MAX_RESORCESCHANNELS; ++i)
		pImmediateContext->PSSetShaderResources(i, 1, &renderNull);
	if (renderNull) renderNull->Release();

	ID3D11RenderTargetView* viewNull = nullptr;
	pImmediateContext->OMSetRenderTargets(1, &viewNull, nullptr);

	if (viewNull) viewNull->Release();
}

void Buffer::Render(
	Buffer* pBuffers,
	ID3D11DeviceContext* pImmediateContext,
	ID3D11DepthStencilView* pDepthStencilView,
	ID3D11Buffer* pCBNeverChanges,
	const UINT uIndexCount,
	const UINT uWidth,
	const UINT uHeight,
	const int iIndex
)
{
	assert(pBuffers != nullptr);
	assert(pImmediateContext != nullptr);
	assert(pDepthStencilView != nullptr);
	assert(pCBNeverChanges != nullptr);

	pImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, pDepthStencilView);
	pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, DirectX::Colors::Aqua);

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (m_Res[i].m_iTextureSlot >= 0)
		{
			if (m_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
			{
				pImmediateContext->CopyResource(m_ppTexture2DCopy[i], pBuffers[(int)m_Channels[i].m_BufferId].m_pRenderTargetTexture);
				pImmediateContext->PSSetShaderResources(m_Res[i].m_iTextureSlot, 1, &m_ppShaderResourceViewCopy[i]);
			}
			else if (m_Channels[i].m_Type == Channels::ChannelType::E_Texture)
			{
				pImmediateContext->PSSetShaderResources(m_Res[i].m_iTextureSlot, 1, &m_Res[i].m_pShaderResource);
			}
		}
	}

	// Set the shaders
	pImmediateContext->VSSetShader(m_pVertexShader, nullptr, 0);
	pImmediateContext->PSSetShader(m_pPixelShader, nullptr, 0);

	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4((float)uWidth, (float)uHeight, 1.0f / (float)uWidth, 1.0f / (float)uHeight);
	cbNC.ChannelResolution[0] = m_Res[0].m_vChannelRes;
	cbNC.ChannelResolution[1] = m_Res[1].m_vChannelRes;
	cbNC.ChannelResolution[2] = m_Res[2].m_vChannelRes;
	cbNC.ChannelResolution[3] = m_Res[3].m_vChannelRes;

	pImmediateContext->UpdateSubresource(pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	int allocSize = MEMORY_ALIGNINT(m_uCustomizableBufferSize, 4 * sizeof(float));
	void* customizableBufferData = malloc(allocSize);

	for (int i = 0; i < m_vCustomizableBuffer.size(); ++i)
	{
		if (m_vCustomizableBuffer[i].m_bIsDataSet)
		{
			if (m_vCustomizableBuffer[i].m_iType == D3D_SVT_FLOAT)
			{
				for (int j = 0; j < m_vCustomizableBuffer[i].m_iSize / sizeof(float); ++j)
					((float*)customizableBufferData)[m_vCustomizableBuffer[i].m_uOffset / sizeof(float) + j] = ((float*)m_vCustomizableBuffer[i].m_pData)[j];
			}
			else if (m_vCustomizableBuffer[i].m_iType == D3D_SVT_INT)
			{
				for (int j = 0; j < m_vCustomizableBuffer[i].m_iSize / sizeof(int); ++j)
					((int*)customizableBufferData)[m_vCustomizableBuffer[i].m_uOffset / sizeof(int) + j] = ((int*)m_vCustomizableBuffer[i].m_pData)[j];
			}
			else
			{
				// Currently not supporting other types
				assert(m_vCustomizableBuffer[i].m_iType == D3D_SVT_FLOAT || m_vCustomizableBuffer[i].m_iType == D3D_SVT_INT);
			}
		}
	}
	pImmediateContext->UpdateSubresource(m_pCBCustomizable, 0, nullptr, customizableBufferData, 0, 0);

	free(customizableBufferData);
	customizableBufferData = nullptr;

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		pImmediateContext->PSSetSamplers(i, 1, &m_Res[i].m_pSampler);
	}

	// Set constant buffers
	pImmediateContext->PSSetConstantBuffers(0, 1, &pCBNeverChanges);

	if(m_uCustomizableBufferSize > 0)
		pImmediateContext->PSSetConstantBuffers(2, 1, &m_pCBCustomizable);

	pImmediateContext->DrawIndexed(uIndexCount, 0, 0);

	ClearShaderResource(pImmediateContext, pDepthStencilView);
}

void Buffer::Terminate()
{	
	SAFE_RELEASE(m_pPixelShader);
	SAFE_RELEASE(m_pRenderTargetTexture);
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_pShaderResourceView);

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (m_Res[i].m_pSampler)
			m_Res[i].m_pSampler->Release();

		SAFE_RELEASE(m_ppShaderResourceViewCopy[i]);
		SAFE_RELEASE(m_ppTexture2DCopy[i]);
	}

	SAFE_RELEASE(m_pCBCustomizable);
}