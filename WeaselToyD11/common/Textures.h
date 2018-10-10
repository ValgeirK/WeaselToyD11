#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "type/Channel.h"

enum TextLoc
{
	main = 0,
	BufferA = 1,
	BufferB = 2,
	BufferC = 3,
	BufferD = 4
};

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
	const TextLoc,
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