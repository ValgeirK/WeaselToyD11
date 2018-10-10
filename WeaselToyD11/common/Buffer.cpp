#include "Buffer.h"

#include <stdlib.h>
#include <stddef.h>

#include "Shader.h"
#include "Loader.h"
#include "Textures.h"
#include "HelperFunction.h"
#include "type/Channel.h"
#include "type/ConstantBuffer.h"

HRESULT Buffer::InitBuffer(
	ID3D11Device*        pd3dDevice, 
	ID3D11DeviceContext* pImmediateContext, 
	ID3D11VertexShader*  pVertShader,
	const int width, 
	const int height, 
	const int index
)
{
	HRESULT hr = S_OK;

	// Reading in Channel file for this channel
	const char* channelPath = "";
	const WCHAR* pixelShaderPath = (wchar_t *)malloc(sizeof(wchar_t) * 100);

	switch (index)
	{
	case 0:
		channelPath = "channels/channelsA.txt";
		pixelShaderPath = L"shaders/PixelShaderBufferA.hlsl";
		break;
	case 1:
		channelPath = "channels/channelsB.txt";
		pixelShaderPath = L"shaders/PixelShaderBufferB.hlsl";
		break;
	case 2:
		channelPath = "channels/channelsC.txt";
		pixelShaderPath = L"shaders/PixelShaderBufferC.hlsl";
		break;
	case 3:
		channelPath = "channels/channelsD.txt";
		pixelShaderPath = L"shaders/PixelShaderBufferD.hlsl";
		break;
	}

	mpVertexShader = pVertShader;

	hr = Create2DTexture(pd3dDevice, &mRenderTargetTexture, &mRenderTargetView, &mShaderResourceView, width, height);

	// Compile the pixel shader
	ID3DBlob* pPSBufferBlob = nullptr;
	hr = CompileShaderFromFile(pixelShaderPath, "main", "ps_4_0", &pPSBufferBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBufferBlob->GetBufferPointer(), pPSBufferBlob->GetBufferSize(), nullptr, &mpPixelShader);
	pPSBufferBlob->Release();
	if (FAILED(hr))
		return hr;

	SetDebugObjectName(mpPixelShader, "BufferPixelShader");

	int size = 0;
	if (!LoadChannels(channelPath, channels, size))
		return S_FALSE;

	TextLoc location = TextLoc::main;
	int padding = 0;

	switch (index)
	{
	case 0:
		location = TextLoc::BufferA;
		padding = 1;
		break;
	case 1:
		location = TextLoc::BufferB;
		padding = 2;
		break;
	case 2:
		location = TextLoc::BufferC;
		padding = 3;
		break;
	case 3:
		location = TextLoc::BufferD;
		padding = 4;
		break;
	}

	// Load texture
	for (int i = 0; i < size; ++i)
	{
		if (channels[i].type == ChannelType::texture)
		{
			LoadTexture(pd3dDevice, &mpTextureRV[i], channels[i].texture, mvChannelRes[i]);

			// Texture
			pImmediateContext->PSSetShaderResources(i + padding * 4, 1, &mpTextureRV[i]);
		}
		else if (channels[i].type == ChannelType::buffer)
		{
			Create2DTexture(pd3dDevice, &mRenderTargetTextureCopy, nullptr, &mShaderResourceViewCopy, width, height);
		}

		// Create Sampler for buffer
		CreateSampler(pd3dDevice, pImmediateContext, &mpSampler[i], channels[i], location, i);
	}

	isActive = true;

	return hr;
}

HRESULT Buffer::ReloadShader(ID3D11Device* pd3dDevice, ID3D11VertexShader*  pVertShader, const int index)
{
	HRESULT hr = S_OK;

	const char* channelPath = "";
	const WCHAR* pixelShaderPath = (wchar_t *)malloc(sizeof(wchar_t) * 100);

	switch (index)
	{
	case 0:
		channelPath = "channels/channelsA.txt";
		pixelShaderPath = L"shaders/PixelShaderBufferA.hlsl";
		break;
	case 1:
		channelPath = "channels/channelsB.txt";
		pixelShaderPath = L"shaders/PixelShaderBufferB.hlsl";
		break;
	case 2:
		channelPath = "channels/channelsC.txt";
		pixelShaderPath = L"shaders/PixelShaderBufferC.hlsl";
		break;
	case 3:
		channelPath = "channels/channelsD.txt";
		pixelShaderPath = L"shaders/PixelShaderBufferD.hlsl";
		break;
	}

	mpVertexShader = pVertShader;

	// Compile the pixel shader
	ID3DBlob* pPSBufferBlob = nullptr;
	hr = CompileShaderFromFile(pixelShaderPath, "main", "ps_4_0", &pPSBufferBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			(LPCSTR)"Error with the pixel shader.", (LPCSTR)"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = pd3dDevice->CreatePixelShader(pPSBufferBlob->GetBufferPointer(), pPSBufferBlob->GetBufferSize(), nullptr, &mpPixelShader);
	pPSBufferBlob->Release();
	if (FAILED(hr))
		return hr;

	return hr;
}

void Buffer::SetShaderResource(ID3D11DeviceContext* pImmediateContext, const int index)
{
	pImmediateContext->PSSetShaderResources(index, 1, &mShaderResourceView);
}

void Buffer::ClearShaderResource(ID3D11DeviceContext* pImmediateContext, const int index)
{
	ID3D11ShaderResourceView* renderNull = nullptr;
	pImmediateContext->PSSetShaderResources(index, 1, &renderNull);
	if (renderNull) renderNull->Release();
}

void Buffer::RecursiveRender(ID3D11DeviceContext* pImmediateContext, const int index)
{
	ClearShaderResource(pImmediateContext, index);

	pImmediateContext->CopyResource(mRenderTargetTextureCopy, mRenderTargetTexture);
}

void Buffer::Render(
	ID3D11DeviceContext* pImmediateContext,
	ID3D11DepthStencilView* pDepthStencilView,
	ID3D11Buffer* pCBNeverChanges,
	const UINT indexCount,
	const UINT width,
	const UINT height,
	const int index
)
{
	for (int i = 0; i < 4; ++i)
	{
		if (channels[i].type == ChannelType::buffer)
		{
			pImmediateContext->CopyResource(mRenderTargetTextureCopy, mRenderTargetTexture);
			pImmediateContext->PSSetShaderResources(i + (index + 1)* 4, 1, &mShaderResourceViewCopy);
		}
	}

	// Set the shaders
	pImmediateContext->VSSetShader(mpVertexShader, nullptr, 0);
	pImmediateContext->PSSetShader(mpPixelShader, nullptr, 0);

	pImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, pDepthStencilView);

	pImmediateContext->ClearRenderTargetView(mRenderTargetView, DirectX::Colors::Aqua);

	CBNeverChanges cbNC;
	cbNC.Resolution = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
	cbNC.ChannelResolution[0] = mvChannelRes[0];
	cbNC.ChannelResolution[1] = mvChannelRes[1];
	cbNC.ChannelResolution[2] = mvChannelRes[2];
	cbNC.ChannelResolution[3] = mvChannelRes[3];

	pImmediateContext->UpdateSubresource(pCBNeverChanges, 0, nullptr, &cbNC, 0, 0);

	// Set constant buffers
	pImmediateContext->PSSetConstantBuffers(0, 1, &pCBNeverChanges);


	pImmediateContext->DrawIndexed(indexCount, 0, 0);

	ID3D11RenderTargetView* renderNull = nullptr;
	pImmediateContext->OMSetRenderTargets(1, &renderNull, nullptr);
	if (renderNull) renderNull->Release();
}

void Buffer::Release(int index)
{
	for (int i = 0; i < 4; ++i)
	{
		if (mpSampler[i]) mpSampler[i]->Release();
		if (mpTextureRV[i]) mpTextureRV[i]->Release();
	}

	if (mRenderTargetTextureCopy) mRenderTargetTextureCopy->Release();
	if (mShaderResourceViewCopy) mShaderResourceViewCopy->Release();

	if (mpPixelShader) mpPixelShader->Release();
	if (mRenderTargetTexture) mRenderTargetTexture->Release();
	if (mRenderTargetView) mRenderTargetView->Release();

	if (mShaderResourceView) mShaderResourceView->Release();
}