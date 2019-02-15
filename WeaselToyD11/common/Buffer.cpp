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
#include "type/Channel.h"
#include "type/ConstantBuffer.h"
#include "type/HashDefines.h"

HRESULT Buffer::InitBuffer(
	ID3D11Device*        pd3dDevice,
	ID3D11DeviceContext* pImmediateContext,
	ID3D11VertexShader*  pVertShader,
	TextureLib*			 pTextureLib,
	Buffer*				 pBuffer,
	const char*			 strProj,
	const int			 width, 
	const int			 height, 
	Channels::BufferId   index
)
{
	HRESULT hr = S_OK;

	int padding = 0;

	// Reading in Channel file for this channel
	std::string projChannelPath = PROJECT_PATH + std::string(strProj);
	// Make room for characters
	std::wstring projPixelPathW(projChannelPath.length(), L' ');
	 // Copy string to wstring.
	std::copy(projChannelPath.begin(), projChannelPath.end(), projPixelPathW.begin());

	switch (index)
	{
	case 0:
		projChannelPath += std::string("/channels/channelsA.txt");
		projPixelPathW += std::wstring(L"/shaders/PixelShaderBufferA.hlsl");
		padding = 1;
		break;
	case 1:
		projChannelPath += std::string("/channels/channelsB.txt");
		projPixelPathW += std::wstring(L"/shaders/PixelShaderBufferB.hlsl");
		padding = 2;
		break;
	case 2:
		projChannelPath += std::string("/channels/channelsC.txt");
		projPixelPathW += std::wstring(L"/shaders/PixelShaderBufferC.hlsl");
		padding = 3;
		break;
	case 3:
		projChannelPath += std::string("/channels/channelsD.txt");
		projPixelPathW += std::wstring(L"/shaders/PixelShaderBufferD.hlsl");
		padding = 4;
		break;
	}

	m_pVertexShader = pVertShader;

	// Compile the pixel shader
	ID3DBlob* pPSBufferBlob = nullptr;
	hr = CompileShaderFromFile(projPixelPathW.c_str(), "main", "ps_4_0", &pPSBufferBlob, m_ShaderError);
	if (FAILED(hr))
	{
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			_bstr_t b(projPixelPathW.c_str());
			const char* c = b;
			std::string msg = "Error Pixel Shader \"" + std::string(c) + "\" not found!";

			MessageBox(nullptr,
				(LPCSTR)msg.c_str(), (LPCSTR)"Error", MB_OK);
		}

		MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
	}

	hr = Create2DTexture(pd3dDevice, &m_pRenderTargetTexture, &m_pRenderTargetView, &m_pShaderResourceView, width, height);

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBufferBlob->GetBufferPointer(), pPSBufferBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	pPSBufferBlob->Release();
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(m_pPixelShader, "BufferPixelShader");

	if (!LoadChannels(projChannelPath.c_str(), m_Channels, m_iSize))
		return S_FALSE;

	// Load texture
	for (int i = 0; i < MAX_RESORCES; ++i)
	{
		int iBufferIndex = static_cast<int>(index);
		if (m_Channels[i].m_Type == Channels::ChannelType::E_Texture)
		{
			m_Res[i].m_Type = Channels::ChannelType::E_Texture;

			strcpy(m_Res[i].m_strTexture, m_Channels[i].m_strTexture);

			pTextureLib->GetTexture(m_Channels[i].m_strTexture, &m_Res[i].m_pShaderResource, m_Res[i].m_vChannelRes);

			// Texture
			pImmediateContext->PSSetShaderResources(i + padding * MAX_RESORCES, 1, &m_Res[i].m_pShaderResource);
		}
		else if (m_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			m_Res[i].m_Type = Channels::ChannelType::E_Buffer;

			// Initialize buffer
			pBuffer[static_cast<int>(m_Channels[i].m_BufferId)].InitBuffer(pd3dDevice, pImmediateContext, pVertShader, pTextureLib, pBuffer, strProj, width, height, m_Channels[i].m_BufferId);

			m_Res[i].m_vChannelRes = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
			m_Res[i].m_iBufferIndex = static_cast<int>(m_Channels[i].m_BufferId);
			
			// There is a buffer that needs resizing
			m_bResizeBuffer = true;
		}
		else
		{
			m_Res[i].m_Type = Channels::ChannelType::E_None;
			m_Res[i].m_pShaderResource = nullptr;
		}

		if (m_Channels[i].m_Type != Channels::ChannelType::E_None)
	    {
			// Create Sampler for buffer
			CreateSampler(pd3dDevice, pImmediateContext, &m_pSampler[i], m_Channels[i], iBufferIndex, i);
	    }
	}

	m_bIsActive = true;

	return hr;
}

void Buffer::ResizeTexture(ID3D11Device* device, ID3D11DeviceContext* context, const UINT width, const UINT height)
{
	if (m_pRenderTargetTexture)m_pRenderTargetTexture->Release();
	if (m_pRenderTargetView)m_pRenderTargetView->Release();
	if (m_pShaderResourceView)m_pShaderResourceView->Release();

	Create2DTexture(device, &m_pRenderTargetTexture, &m_pRenderTargetView, &m_pShaderResourceView, width, height);

	for(int i = 0; i < MAX_RESORCES; ++i)
	{
		if (m_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			m_Res[i].m_vChannelRes.x = (float)width;
			m_Res[i].m_vChannelRes.y = (float)height;
		}
	}
}

void Buffer::ReloadTexture(TextureLib* pTextureLib, int idx)
{
	pTextureLib->GetTexture(m_Channels[idx].m_strTexture, &m_Res[idx].m_pShaderResource, m_Res[idx].m_vChannelRes);
}

HRESULT Buffer::ReloadShader(ID3D11Device* pd3dDevice, ID3D11VertexShader*  pVertShader, const char* strProj, const int index)
{
	HRESULT hr = S_OK;

	// Reading in Channel file for this channel
	std::string channelPath = PROJECT_PATH + std::string(strProj);
	// Make room for characters
	std::wstring pixelShaderPath(channelPath.length(), L' ');
	// Copy string to wstring.
	std::copy(channelPath.begin(), channelPath.end(), pixelShaderPath.begin());

	switch (index)
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
	hr = CompileShaderFromFile(pixelShaderPath.c_str(), "main", "ps_4_0", &pPSBufferBlob, m_ShaderError);
	if (FAILED(hr))
	{
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			_bstr_t b(pixelShaderPath.c_str());
			const char* c = b;
			std::string msg = "Error Pixel Shader \"" + std::string(c) + "\" not found!";

			MessageBox(nullptr,
				(LPCSTR)msg.c_str(), (LPCSTR)"Error", MB_OK);
		}

		MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
	}

	m_pPixelShader->Release();

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBufferBlob->GetBufferPointer(), pPSBufferBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	pPSBufferBlob->Release();
	if (FAILED(hr))
		return hr;

	return hr;
}

void Buffer::SetShaderResource(ID3D11DeviceContext* pImmediateContext, const int index)
{
	pImmediateContext->PSSetShaderResources(index, 1, &m_pShaderResourceView);
}

void Buffer::ClearShaderResource(ID3D11DeviceContext* pImmediateContext, ID3D11DepthStencilView* pDepthStencilView)
{
	// Clearing the Shader Resource so it's not bound on clear
	ID3D11ShaderResourceView* renderNull = nullptr;
	for(int i = 0; i < MAX_RESORCES * (MAX_RESORCES + 1); ++i)
		pImmediateContext->PSSetShaderResources(i, 1, &renderNull);
	
	ID3D11RenderTargetView* viewNull = nullptr;
	pImmediateContext->OMSetRenderTargets(1, &viewNull, pDepthStencilView);

	if (renderNull) renderNull->Release();
	if (viewNull) viewNull->Release();
}

void Buffer::Render(
	Buffer* pBuffers,
	ID3D11DeviceContext* pImmediateContext,
	ID3D11DepthStencilView* pDepthStencilView,
	ID3D11Buffer* pCBNeverChanges,
	const UINT indexCount,
	const UINT width,
	const UINT height,
	const int index
)
{
	ClearShaderResource(pImmediateContext, pDepthStencilView);

	int padding = 0;
	switch (index)
	{
	case 0:
		padding = 1;
		break;
	case 1:
		padding = 2;
		break;
	case 2:
		padding = 3;
		break;
	case 3:
		padding = 4;
		break;
	default:
		// Index should only be in the range 0 to 3
		assert(0);
		break;
	}


	for (int i = 0; i < MAX_RESORCES; ++i)
	{
		if (m_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			if (pBuffers[(int)m_Channels[i].m_BufferId].m_pRenderTargetTexture != NULL)
			{
				pImmediateContext->PSSetShaderResources(i + (index + 1)* MAX_RESORCES, 1, &pBuffers[(int)m_Channels[i].m_BufferId].m_pShaderResourceView);
			}
		}
		else if (m_Channels[i].m_Type == Channels::ChannelType::E_Texture)
		{
			pImmediateContext->PSSetShaderResources(i + padding * MAX_RESORCES, 1, &m_Res[i].m_pShaderResource);
		}
	}

	// Set the shaders
	pImmediateContext->VSSetShader(m_pVertexShader, nullptr, 0);
	pImmediateContext->PSSetShader(m_pPixelShader, nullptr, 0);

	pImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, pDepthStencilView);

	pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, DirectX::Colors::Aqua);

	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4((float)width, (float)height, 1.0f / (float)width, 1.0f / (float)height);
	cbNC.ChannelResolution[0] = m_Res[0].m_vChannelRes;
	cbNC.ChannelResolution[1] = m_Res[1].m_vChannelRes;
	cbNC.ChannelResolution[2] = m_Res[2].m_vChannelRes;
	cbNC.ChannelResolution[3] = m_Res[3].m_vChannelRes;

	pImmediateContext->UpdateSubresource(pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	// Set constant buffers
	pImmediateContext->PSSetConstantBuffers(0, 1, &pCBNeverChanges);


	pImmediateContext->DrawIndexed(indexCount, 0, 0);

	ID3D11RenderTargetView* renderNull = nullptr;
	pImmediateContext->OMSetRenderTargets(1, &renderNull, nullptr);
	if (renderNull) renderNull->Release();

	SetShaderResource(pImmediateContext, index);
}

void Buffer::Release(int index)
{
	ULONG refs = 0;

	for (int i = 0; i < MAX_RESORCES; ++i)
	{
		if (m_Res[i].m_Type != Channels::ChannelType::E_None && m_pSampler[i])
			refs = m_pSampler[i]->Release();
		else
			refs = 0;

		if (refs > 0)
		{
			_RPTF2(_CRT_WARN, "Buffer Sampler %i still has %i references.\n", i, refs);
		}
	}
	
	if (m_pPixelShader)
		refs = m_pPixelShader->Release();
	else
		refs = 0;

	if (refs > 0)
	{
		_RPTF2(_CRT_WARN, "Buffer PixelShader still has %i references.\n", refs);
	}

	if (m_pRenderTargetTexture)
		refs = m_pRenderTargetTexture->Release();
	else
		refs = 0;

	if (refs > 0)
	{
		_RPTF2(_CRT_WARN, "Buffer RenderTargetTexture still has %i references.\n", refs);
	}

	if (m_pRenderTargetView)
		refs = m_pRenderTargetView->Release();
	else
		refs = 0;

	if (refs > 0)
	{
		_RPTF2(_CRT_WARN, "Buffer RenderTargetView still has %i references.\n", refs);
	}

	if (m_pShaderResourceView)
		refs = m_pShaderResourceView->Release();
	else
		refs = 0;

	if (refs > 0)
	{
		_RPTF2(_CRT_WARN, "Buffer ShaderResourceView still has %i references.\n", refs);
	}
}