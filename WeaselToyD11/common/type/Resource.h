#pragma once

#include <directxcolors.h>

struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;

enum Channels::ChannelType;

struct Resource
{
	ID3D11ShaderResourceView* m_pShaderResource = nullptr;
	ID3D11SamplerState*		  m_pSampler = nullptr;
	DirectX::XMFLOAT4	      m_vChannelRes;
	char     				  m_strTexture[MAX_PATH_LENGTH];
	Channels::ChannelType	  m_Type;
	bool					  m_bIsActive;
	int						  m_iSize;
	int						  m_iBufferIndex;
	int						  m_iTextureSlot = -1;
};