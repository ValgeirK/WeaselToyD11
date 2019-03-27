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
#include "type/ConstantBuffer.h"

class Buffer
{
public:
	Buffer() : m_bIsActive(false) {};

	HRESULT	InitBuffer(
		ID3D11Device*			pd3dDevice,
		ID3D11DeviceContext*	pImmediateContext,
		ID3D11VertexShader*		pVertShader,
		TextureLib*				pTextureLib,
		DWORD					dwShaderFlag,
		const char*				strProj,
		int*					piBufferUsed,
		const int				iWidth,
		const int				iHeight,
		int						iIndex
	);
	void Render(
		Buffer*					pBuffers,
		ID3D11DeviceContext*	pImmediateContext,
		ID3D11DepthStencilView* pDepthStencilView,
		ID3D11Buffer*			pCBNeverChanges,
		const UINT				uIndexCount,
		const UINT				uWidth,
		const UINT				wHeight,
		const int				iIndex
	);
	HRESULT	ReloadShader(
		ID3D11Device*			pd3dDevice, 
		ID3D11VertexShader*		pVertShader, 
		DWORD					dwShaderFlag, 
		const char*				strProj, 
		const int				iIndex
	);
	void ClearShaderResource(
		ID3D11DeviceContext*	pImmediateContext, 
		ID3D11DepthStencilView* pDepthStencilView
	);
	void ResizeTexture(
		ID3D11Device*			pDevice,
		const UINT				uWidth, 
		const UINT				uHeight
	);
	void ReloadTexture(
		ID3D11Device*			pd3dDevice,
		TextureLib*				pTextureLib,
		int*					piBufferUsed,
		const int				iWidth,
		const int				iHeight
	);
	void Terminate();



	bool							m_bIsActive = false;

	ID3D11Texture2D*				m_ppTexture2DCopy[MAX_RESORCESCHANNELS];
	ID3D11ShaderResourceView*		m_ppShaderResourceViewCopy[MAX_RESORCESCHANNELS];

	ID3D11ShaderResourceView*		m_pShaderResourceView = nullptr;

	ID3D11Buffer*					m_pCBCustomizable = nullptr;

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

	std::vector<CustomizableBuffer>	m_vCustomizableBuffer;
	UINT							m_uCustomizableBufferSize = 0;

private:
	void LoadTextures(
		ID3D11Device*			pd3dDevice,
		TextureLib*				pTextureLib,
		int*					piBufferUsed,
		const int				iWidth,
		const int				iHeight
	);
};