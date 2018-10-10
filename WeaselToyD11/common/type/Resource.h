#pragma once

#include <directxcolors.h>

#include "Channel.h"

#include "../Buffer.h"

struct Resource
{
	Buffer					  buffers;
	ID3D11ShaderResourceView* pShaderResource;
	ID3D11SamplerState*		  pSampler;
	DirectX::XMFLOAT4	      channelRes;
	ChannelType				  type;
	bool					  isActive;
};