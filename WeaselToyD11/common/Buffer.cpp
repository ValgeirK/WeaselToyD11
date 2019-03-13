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

HRESULT Buffer::InitBuffer(
	ID3D11Device*			pd3dDevice,
	ID3D11DeviceContext*	pImmediateContext,
	ID3D11VertexShader*		pVertShader,
	TextureLib*				pTextureLib,
	DWORD					dwShaderFlag,
	const char*				strProj,
	int*					piBufferUsed,
	const int				width, 
	const int				height, 
	int						index
)
{
	HRESULT hr = S_OK;

	assert(pd3dDevice != nullptr);
	assert(pImmediateContext != nullptr);

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
	hr = CompileShaderFromFile(projPixelPathW.c_str(), "mainImage", "ps_5_0", &pPSBufferBlob, dwShaderFlag, m_ShaderError);
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

		SetForegroundWindow(nullptr);
		// Trying without a pop-up box
		/*MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);*/
	}

	hr = Create2DTexture(pd3dDevice, &m_pRenderTargetTexture, &m_pRenderTargetView, &m_pShaderResourceView, width, height);

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBufferBlob->GetBufferPointer(), pPSBufferBlob->GetBufferSize(), nullptr, &m_pPixelShader);

	ID3D11ShaderReflection* pPixelShaderReflection;
	Reflection::D3D11ReflectionSetup(pPSBufferBlob, &pPixelShaderReflection);

	pPSBufferBlob->Release();
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(m_pPixelShader, "BufferPixelShader");

	if (!LoadChannels(projChannelPath.c_str(), m_Channels, m_iSize))
		return S_FALSE;

	// Load texture
	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		Create2DTexture(pd3dDevice, &(m_ppTexture2DCopy[i]), nullptr, &(m_ppShaderResourceViewCopy[i]), width, height);

		int iBufferIndex = static_cast<int>(index);
		if (m_Channels[i].m_Type == Channels::ChannelType::E_Texture)
		{
			m_Res[i].m_Type = Channels::ChannelType::E_Texture;

			strcpy(m_Res[i].m_strTexture, m_Channels[i].m_strTexture);

			pTextureLib->GetTexture(m_Channels[i].m_strTexture, &m_Res[i].m_pShaderResource, m_Res[i].m_vChannelRes);
		}
		else if (m_Channels[i].m_Type == Channels::ChannelType::E_Buffer)
		{
			m_Res[i].m_Type = Channels::ChannelType::E_Buffer;

			m_Res[i].m_vChannelRes = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
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
	}

	Reflection::D3D11ShaderReflection(pPixelShaderReflection, m_Res, dwShaderFlag);

	m_bIsActive = true;

	return hr;
}

void Buffer::ResizeTexture(ID3D11Device* device, ID3D11DeviceContext* context, const UINT width, const UINT height)
{
	if (m_pRenderTargetTexture)m_pRenderTargetTexture->Release();
	if (m_pRenderTargetView)m_pRenderTargetView->Release();
	if (m_pShaderResourceView)m_pShaderResourceView->Release();

	Create2DTexture(device, &m_pRenderTargetTexture, &m_pRenderTargetView, &m_pShaderResourceView, width, height);

	for(int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (m_ppTexture2DCopy[i])m_ppTexture2DCopy[i]->Release();
		if (m_ppShaderResourceViewCopy[i])m_ppShaderResourceViewCopy[i]->Release();

		Create2DTexture(device, &m_ppTexture2DCopy[i], nullptr, &m_ppShaderResourceViewCopy[i], width, height);

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

HRESULT Buffer::ReloadShader(ID3D11Device* pd3dDevice, ID3D11VertexShader*  pVertShader, DWORD dwShaderFlag, const char* strProj, const int index)
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
	hr = CompileShaderFromFile(pixelShaderPath.c_str(), "mainImage", "ps_5_0", &pPSBufferBlob, dwShaderFlag, m_ShaderError);
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

		SetForegroundWindow(nullptr);
		// Trying without a pop-up box
		/*MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);*/
	}

	m_pPixelShader->Release();

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBufferBlob->GetBufferPointer(), pPSBufferBlob->GetBufferSize(), nullptr, &m_pPixelShader);

	ID3D11ShaderReflection* pPixelShaderReflection;
	Reflection::D3D11ReflectionSetup(pPSBufferBlob, &pPixelShaderReflection);

	pPSBufferBlob->Release();
	if (FAILED(hr))
		return hr;

	Reflection::D3D11ShaderReflection(pPixelShaderReflection, m_Res, dwShaderFlag);

	return hr;
}

void Buffer::ClearShaderResource(ID3D11DeviceContext* pImmediateContext, ID3D11DepthStencilView* pDepthStencilView)
{
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
	const UINT indexCount,
	const UINT width,
	const UINT height,
	const int index
)
{
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
	cbNC.Resolution = DirectX::XMFLOAT4((float)width, (float)height, 1.0f / (float)width, 1.0f / (float)height);
	cbNC.ChannelResolution[0] = m_Res[0].m_vChannelRes;
	cbNC.ChannelResolution[1] = m_Res[1].m_vChannelRes;
	cbNC.ChannelResolution[2] = m_Res[2].m_vChannelRes;
	cbNC.ChannelResolution[3] = m_Res[3].m_vChannelRes;

	pImmediateContext->UpdateSubresource(pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	// Set constant buffers
	pImmediateContext->PSSetConstantBuffers(0, 1, &pCBNeverChanges);


	pImmediateContext->DrawIndexed(indexCount, 0, 0);

	ClearShaderResource(pImmediateContext, pDepthStencilView);
}

void Buffer::Release(int index)
{
	ULONG refs = 0;
	
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

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (m_ppShaderResourceViewCopy[i])
			refs = m_ppShaderResourceViewCopy[i]->Release();
		else
			refs = 0;

		if (refs > 0)
		{
			_RPTF2(_CRT_WARN, "Buffer m_ppShaderResourceViewCopy[%i] still has %i references.\n", i, refs);
		}

		if (m_ppTexture2DCopy[i])
			refs = m_ppTexture2DCopy[i]->Release();
		else
			refs = 0;

		if (refs > 0)
		{
			_RPTF2(_CRT_WARN, "Buffer m_ppTexture2DCopy[%i] still has %i references.\n", i, refs);
		}
	}
}