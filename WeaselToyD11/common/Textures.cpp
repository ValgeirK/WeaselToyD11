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
	delete[] path;
	if (FAILED(hr))
		return hr;

	vChannelRes = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);

	return hr;
}

HRESULT CreateSampler(
	ID3D11Device* pd3dDevice,
	ID3D11SamplerState** pSampler,
	Channel channel,
	const int location,
	const int index
)
{
	HRESULT hr = S_OK;

	// Create the sample state
	D3D11_SAMPLER_DESC samplDesc = {};

	// Set the right filter
	switch (channel.m_Filter)
	{
	case Channels::FilterType::E_Mipmap:
		samplDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		break;
	case Channels::FilterType::E_Linear:
		samplDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		break;
	case Channels::FilterType::E_Nearest:
		samplDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	}

	// Set the right wrapping
	switch (channel.m_Wrap)
	{
	case Channels::WrapType::E_Clamp:
		samplDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		break;
	case Channels::WrapType::E_Repeat:
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

	switch (location)
	{
	case 0:
		SetDebugObjectName(*pSampler, "TextureSampler");
		break;
	case 1:
		SetDebugObjectName(*pSampler, "BufferA_Sampler");
		break;
	case 2:
		SetDebugObjectName(*pSampler, "BufferB_Sampler");
		break;
	case 3:
		SetDebugObjectName(*pSampler, "BufferC_Sampler");
		break;
	case 4:
		SetDebugObjectName(*pSampler, "BufferD_Sampler");
		break;
	}

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
	assert(pd3dDevice != nullptr);

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
	assert(mRenderTargetTexture != nullptr);

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

HRESULT CreateDepthStencilView(
	ID3D11Device* device, 
	ID3D11DepthStencilView** depthStencilView,
	ID3D11DepthStencilState** depthStencilState,
	ID3D11Texture2D** texture2D,
	const UINT width,
	const UINT height
	)
{
	assert(device != nullptr);

	HRESULT hr = S_OK;

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the Depth/Stencil View
	hr = device->CreateTexture2D(&depthStencilDesc, NULL, texture2D);
	hr = device->CreateDepthStencilView(*texture2D, NULL, depthStencilView);
	hr = device->CreateDepthStencilState(&dsDesc, depthStencilState);

	SetDebugObjectName(*texture2D, "DepthStencilBuffer");
	SetDebugObjectName(*depthStencilView, "DepthStencilView");
	SetDebugObjectName(*depthStencilState, "DepthStencilState");

	return hr;
}

HRESULT CreateRbtTexture(ID3D11Device* pDevice, void* data, ID3D11ShaderResourceView** ppShaderResourceView, int iWidth, int iHeight)
{
	assert(pDevice != nullptr);

	HRESULT hr = S_OK;

	int mipCount = 1, arraySize = 1;
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
	UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
	UINT cpuAccessFlags = 0;

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = static_cast<UINT>(iWidth);
	desc.Height = static_cast<UINT>(iHeight);
	desc.MipLevels = static_cast<UINT>(mipCount);
	desc.ArraySize = static_cast<UINT>(arraySize);
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = usage;
	desc.BindFlags = bindFlags;
	desc.CPUAccessFlags = cpuAccessFlags;

	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = data;
	initData.SysMemPitch = iWidth * sizeof(UINT32);
	initData.SysMemSlicePitch = 0;

	ID3D11Texture2D* tex = nullptr;
	hr = pDevice->CreateTexture2D(&desc, &initData, &tex);

	assert(SUCCEEDED(hr));

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = format;

	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = mipCount;

	hr = pDevice->CreateShaderResourceView(tex,	&SRVDesc, ppShaderResourceView);

	assert(ppShaderResourceView != nullptr);

	assert(SUCCEEDED(hr));

	// Releasing the texture but we keep around the shader resource view, which is referencing it
	// which keeps the texture active
	tex->Release();

	return hr;
}