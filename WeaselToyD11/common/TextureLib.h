#pragma once

#include <windows.h>
#include <d3d11.h>
#include <directxcolors.h>
#include <vector>

struct LoadData;

struct TextureLib
{
	TextureLib();
	~TextureLib();

	void Add(const char* textPath);

	void GetTexture(const char* strDesiredTex, ID3D11ShaderResourceView** ppShaderRes);
	void GetTexture(const char* strDesiredTex, ID3D11ShaderResourceView** ppShaderRes, DirectX::XMFLOAT4& rChannelRes);

	HRESULT ParallelLoadDDSTextures(ID3D11Device* pDevice, const char* strPath);

	char**						m_ppPath;
	ID3D11ShaderResourceView**	m_pShaderResource;
	DirectX::XMFLOAT4*			m_pResolution;
	bool*						m_pIsSet;
	int							m_iLength;
	int							m_iCapacity;
	bool						m_bReload = false;

	LoadData*					m_pLoadData = nullptr;

private:
	void LoadTexturesSingleThread(ID3D11Device* pDevice);
	void FindTexturePaths(const char* path);
	HRESULT InitializeViewsAndRes();
};