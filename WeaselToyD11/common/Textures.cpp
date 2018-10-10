#include "Textures.h"

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "DDSTextureLoader.h"
#include "HelperFunction.h"
#include "type/Channel.h"

HRESULT LoadTexture(
	ID3D11Device* pd3dDevice,
	ID3D11ShaderResourceView** pTextureRV, 
	const char* textPath, 
	DirectX::XMFLOAT4& vChannelRes
)
{
	HRESULT hr = S_OK;

	// Load Texture
	const size_t cSize = strlen(textPath) + 1;
	wchar_t* path = new wchar_t[cSize];
	mbstowcs(path, textPath, cSize);

	hr = DirectX::CreateDDSTextureFromFile(pd3dDevice, path, nullptr, pTextureRV);

	if (FAILED(hr))
		return hr;

	uint32_t width = 0, height = 0;
	hr = DirectX::GetTextureInformation(path, width, height);
	delete path;
	if (FAILED(hr))
		return hr;

	vChannelRes = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);

	return hr;
}

HRESULT CreateSampler(
	ID3D11Device* pd3dDevice,
	ID3D11DeviceContext* pImmediateContext,
	ID3D11SamplerState** pSampler,
	Channel channel,
	const TextLoc location,
	const int index
)
{
	HRESULT hr = S_OK;

	// Create the sample state
	D3D11_SAMPLER_DESC samplDesc = {};

	// Set the right filter
	switch (channel.filter)
	{
	case FilterType::mipmap:
		samplDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case FilterType::linear:
		samplDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		break;
	case FilterType::nearest:
		samplDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	}

	// Set the right wrapping
	switch (channel.wrap)
	{
	case WrapType::clamp:
		samplDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case WrapType::repeat:
		samplDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		break;
	}

	samplDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplDesc.MinLOD = 0;
	samplDesc.MaxLOD = D3D11_FLOAT32_MAX;

	hr = pd3dDevice->CreateSamplerState(&samplDesc, pSampler);
	if (FAILED(hr))
		return hr;

	int padding = 0;
	switch (location)
	{
	case TextLoc::BufferA:
		padding = 1;
		SetDebugObjectName(*pSampler, "BufferA_Sampler");
		break;
	case TextLoc::BufferB:
		padding = 2;
		SetDebugObjectName(*pSampler, "BufferB_Sampler");
		break;
	case TextLoc::BufferC:
		padding = 3;
		SetDebugObjectName(*pSampler, "BufferC_Sampler");
		break;
	case TextLoc::BufferD:
		padding = 4;
		SetDebugObjectName(*pSampler, "BufferD_Sampler");
		break;
	default:
		padding = 0;
		SetDebugObjectName(*pSampler, "TextureSampler");
		break;
	}

	// Sampler
	pImmediateContext->PSSetSamplers(index + 4*padding, 1, pSampler);

	return hr;
}

HRESULT Create2DTexture(
	ID3D11Device* pd3dDevice,
	ID3D11Texture2D**		   mRenderTargetTexture,
	ID3D11RenderTargetView**   mRenderTargetView,
	ID3D11ShaderResourceView** mShaderResourceView,
	const UINT width, const UINT height
)
{
	HRESULT hr = S_OK;

	// 2D Texture
	D3D11_TEXTURE2D_DESC mTextureDesc;

	bool renderTarget = true, shaderResource = true;

	// Initialize the texture description.
	ZeroMemory(&mTextureDesc, sizeof(mTextureDesc));
	mTextureDesc.Width = width;
	mTextureDesc.Height = height;
	mTextureDesc.MipLevels = 1;
	mTextureDesc.ArraySize = 1;
	mTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	mTextureDesc.SampleDesc.Count = 1;
	mTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	mTextureDesc.CPUAccessFlags = 0;
	mTextureDesc.MiscFlags = 0;

	// Checking what kind of texture we want to create
	if (mRenderTargetView == nullptr && mShaderResourceView == nullptr)
	{
		renderTarget = false;
		shaderResource = false;

		mTextureDesc.BindFlags = 0;
	}
	else if (mRenderTargetTexture == nullptr)
	{
		mRenderTargetView = false;
		mTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	}
	else if (mShaderResourceView == nullptr)
	{
		shaderResource = false;
		mTextureDesc.BindFlags = D3D10_BIND_RENDER_TARGET;
	}
	else
	{
		mTextureDesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	}

	// Create the texture
	hr = pd3dDevice->CreateTexture2D(&mTextureDesc, NULL, mRenderTargetTexture);

	// Render Target
	if (renderTarget)
	{
		D3D11_RENDER_TARGET_VIEW_DESC mRenderTargetViewDesc;

		// Creating render target
		mRenderTargetViewDesc.Format = mTextureDesc.Format;
		mRenderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		mRenderTargetViewDesc.Texture2D.MipSlice = 0;

		hr = pd3dDevice->CreateRenderTargetView(*mRenderTargetTexture, &mRenderTargetViewDesc, mRenderTargetView);
	}

	// Shader Resource View
	if (shaderResource)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC mShaderResourceViewDesc;

		// Creating shader resource view
		mShaderResourceViewDesc.Format = mTextureDesc.Format;
		mShaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		mShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		mShaderResourceViewDesc.Texture2D.MipLevels = 1;

		hr = pd3dDevice->CreateShaderResourceView(*mRenderTargetTexture, &mShaderResourceViewDesc, mShaderResourceView);
	}

	return hr;
}