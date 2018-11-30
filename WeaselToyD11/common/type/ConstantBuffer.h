#pragma once

#include <directxcolors.h>

namespace Types
{
	enum VarType
	{
		E_FLOAT,
		E_INT
	};
}

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

struct CustomizableBuffer
{
	UINT offset;
	char strVariable[MAX_PATH];
	char strType[MAX_PATH];
	char strData[MAX_PATH];
	float min;
	float max;
	float step;
};