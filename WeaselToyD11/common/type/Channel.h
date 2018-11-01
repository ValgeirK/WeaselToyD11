#pragma once

#include <cstring>

namespace Channels
{
	enum ChannelType
	{
		E_Texture = 0,
		E_Buffer = 1
	};

	enum FilterType
	{
		E_Mipmap = 0,
		E_Nearest = 1,
		E_Linear = 2
	};

	enum WrapType
	{
		E_Clamp = 0,
		E_Repeat = 1
	};

	enum BufferId
	{
		E_BufferA = 0,
		E_BufferB = 1,
		E_BufferC = 2,
		E_BufferD = 3
	};
}

struct Channel
{
	Channel() {};
	Channel(const char path[]) { strcpy(m_strTexture, path); };

	Channels::ChannelType	m_Type;
	char					m_strTexture[50];
	Channels::FilterType	m_Filter;
	Channels::WrapType		m_Wrap;
	Channels::BufferId		m_BufferId;
};