#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "type/Channel.h"

HRESULT LoadTexture(
	ID3D11Device*,
	ID3D11ShaderResourceView**,
	const char*,
	DirectX::XMFLOAT4&
);

HRESULT CreateSampler(
	ID3D11Device*,
	ID3D11DeviceContext*,
	ID3D11SamplerState**,
	Channel,
	const int,
	const int 
);

HRESULT Create2DTexture(
	ID3D11Device*,
	ID3D11Texture2D**,
	ID3D11RenderTargetView**,
	ID3D11ShaderResourceView**,
	const UINT,
	const UINT
);

HRESULT CreateDepthStencilView(
	ID3D11Device*,
	ID3D11DepthStencilView**,
	ID3D11DepthStencilState**,
	ID3D11Texture2D**,
	const UINT,
	const UINT
);