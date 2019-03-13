#pragma once

#include <vector>
#include <string.h>
#include <directxcolors.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11VertexShader;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11PixelShader;
struct ID3D11RenderTargetView;
struct ID3D11ShaderReflection;

struct TextureLib;
struct Channel;
class  Buffer;

typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef long HRESULT;

#include "type/Channel.h"
#include "type/Resource.h"
#include "type/HashDefines.h"

class Buffer
{
public:
	Buffer() : m_bIsActive(false) {};

	HRESULT							InitBuffer(ID3D11Device*, ID3D11DeviceContext*, ID3D11VertexShader*, TextureLib*, DWORD, const char*, int*, const int, const int, int);
	void							Render(Buffer*,ID3D11DeviceContext*, ID3D11DepthStencilView*, ID3D11Buffer*, const UINT, const UINT, const UINT, const int);
	HRESULT							ReloadShader(ID3D11Device*, ID3D11VertexShader*, DWORD, const char*, const int);
	void							ClearShaderResource(ID3D11DeviceContext*, ID3D11DepthStencilView*);
	void							ResizeTexture(ID3D11Device*, ID3D11DeviceContext*, const UINT, const UINT);
	void							ReloadTexture(TextureLib*, int);
	void							Release(int);

	bool							m_bIsActive = false;

	ID3D11Texture2D*				m_ppTexture2DCopy[MAX_RESORCESCHANNELS];
	ID3D11ShaderResourceView*		m_ppShaderResourceViewCopy[MAX_RESORCESCHANNELS];

	ID3D11ShaderResourceView*		m_pShaderResourceView = nullptr;

	// Variables
	DirectX::XMFLOAT4				m_BufferResolution;

	// Resource
	Resource					    m_Res[MAX_RESORCESCHANNELS];

	int								m_iSize = 0;
	std::vector<std::string>		m_ShaderError;

	// Shader
	ID3D11VertexShader*				m_pVertexShader = nullptr;
	ID3D11PixelShader*				m_pPixelShader = nullptr;

	Channel							m_Channels[MAX_RESORCESCHANNELS];

	ID3D11Texture2D*				m_pRenderTargetTexture = nullptr;
	ID3D11RenderTargetView*			m_pRenderTargetView = nullptr;

	bool							m_bResizeBuffer = false;
};