#pragma once

#include <directxcolors.h>

struct CBNeverChanges
{
	DirectX::XMFLOAT4 ChannelResolution[4];
	DirectX::XMFLOAT4 Resolution;
};

struct CBChangesEveryFrame
{
	DirectX::XMFLOAT4 Mouse;
	DirectX::XMFLOAT4 Date;
	float             Time;
	float             TimeDelta;
	int			      Frame;
	int			      Padding;
};