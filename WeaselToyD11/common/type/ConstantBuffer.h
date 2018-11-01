#pragma once

#include <directxcolors.h>

struct CBNeverChanges
{
	DirectX::XMFLOAT4 ChannelResolution[4];
	DirectX::XMFLOAT4 Resolution;
};

struct CBChangesEveryFrame
{
	DirectX::XMFLOAT4 m_vMouse;
	DirectX::XMFLOAT4 m_vDate;
	float             m_fTime;
	float             m_fTimeDelta;
	int			      m_iFrame;
	int			      m_iPadding;
};