#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <vector>
#include <string.h>

#include "TextureLib.h"
#include "type/Channel.h"
#include "type/Resource.h"

class Buffer
{
public:
	Buffer() : m_bIsActive(false) {};

	HRESULT							InitBuffer(ID3D11Device*, ID3D11DeviceContext*, ID3D11VertexShader*, TextureLib*, Buffer*, const char*, const int, const int, Channels::BufferId);
	void							Render(Buffer*,ID3D11DeviceContext*, ID3D11DepthStencilView*, ID3D11Buffer*, const UINT, const UINT, const UINT, const int);
	HRESULT							ReloadShader(ID3D11Device*, ID3D11VertexShader*, const char*, const int);
	void							SetShaderResource(ID3D11DeviceContext*, const int);
	void							ClearShaderResource(ID3D11DeviceContext*, ID3D11DepthStencilView*);
	void							ResizeTexture(ID3D11Device*, ID3D11DeviceContext*, const UINT, const UINT);
	void							ReloadTexture(TextureLib*, int);
	void							Release(int);

	bool							m_bIsActive = false;

	ID3D11ShaderResourceView*		m_pShaderResourceView = nullptr;

	// Texture
	//ID3D11ShaderResourceView*		m_pTextureRV[4];

	// Variables
	DirectX::XMFLOAT4				m_BufferResolution;

	// Resource
	Resource					    m_Res[4];

	int								m_iSize = 0;
	std::vector<std::string>		m_ShaderError;

	// Shader
	ID3D11VertexShader*				m_pVertexShader = nullptr;
	ID3D11PixelShader*				m_pPixelShader = nullptr;

	// Sampler
	ID3D11SamplerState*				m_pSampler[4];

	Channel							m_Channels[4];

	ID3D11Texture2D*				m_pRenderTargetTexture = nullptr;
	ID3D11RenderTargetView*			m_pRenderTargetView = nullptr;

	bool							m_bResizeBuffer = false;
};