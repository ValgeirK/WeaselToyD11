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

	void LoadTexturesSingleThread(ID3D11Device*);

	void Add(const char*);

	void GetTexture(const char*, ID3D11ShaderResourceView**);
	void GetTexture(const char*, ID3D11ShaderResourceView**, DirectX::XMFLOAT4&);

	HRESULT ParallelLoadDDSTextures(ID3D11Device*, const char*);
	HRESULT LoadDefaultTexture(ID3D11Device*);

	char**						m_ppPath;
	ID3D11ShaderResourceView**	m_pShaderResource;
	DirectX::XMFLOAT4*			m_pResolution;
	bool*						m_pIsSet;
	int							m_iLength;
	int							m_iCapacity;
	bool						m_bReload = false;

	LoadData*					m_pLoadData = nullptr;
};