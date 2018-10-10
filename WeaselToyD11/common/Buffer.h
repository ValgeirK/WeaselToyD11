#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "type/Channel.h"

class Buffer
{
public:
	Buffer() {};

	HRESULT InitBuffer(ID3D11Device*, ID3D11DeviceContext*, ID3D11VertexShader*, const int, const int, const int);
	void Render(ID3D11DeviceContext*, ID3D11DepthStencilView*, ID3D11Buffer*, const UINT, const UINT, const UINT, const int);
	HRESULT ReloadShader(ID3D11Device*, ID3D11VertexShader*, const int);
	void SetShaderResource(ID3D11DeviceContext*, const int);
	void ClearShaderResource(ID3D11DeviceContext*, const int);
	void Release(int);

	bool							isActive = false;

	ID3D11ShaderResourceView*		mShaderResourceView;

	// Texture
	ID3D11ShaderResourceView*		mpTextureRV[4];

	// Variables
	DirectX::XMFLOAT4				mvChannelRes[4];

private:
	void RecursiveRender(ID3D11DeviceContext*, const int);

	// Shader
	ID3D11VertexShader*				mpVertexShader = nullptr;
	ID3D11PixelShader*				mpPixelShader = nullptr;

	// Sampler
	ID3D11SamplerState*				mpSampler[4];

	Channel							channels[4];

	ID3D11Texture2D*				mRenderTargetTexture;
	ID3D11RenderTargetView*			mRenderTargetView;

	ID3D11Texture2D*				mRenderTargetTextureCopy;
	ID3D11ShaderResourceView*		mShaderResourceViewCopy;
};