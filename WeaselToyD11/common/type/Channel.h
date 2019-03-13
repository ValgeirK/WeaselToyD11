#pragma once

#include "HashDefines.h"

namespace Channels
{
	enum ChannelType
	{
		E_None = 0,
		E_Texture = 1,
		E_Buffer = 2,
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
	Channel() 
	{
		m_Type = Channels::ChannelType::E_None;
		strcpy(m_strTexture, "");
		m_Filter = Channels::FilterType::E_Linear;
		m_Wrap = Channels::WrapType::E_Clamp;
		m_BufferId = Channels::BufferId::E_BufferA;
		m_iTextureSlot = -1;
	};

	Channel(const char path[]) { strcpy(m_strTexture, path); };

	Channels::ChannelType	m_Type;
	char					m_strTexture[MAX_PATH];
	Channels::FilterType	m_Filter;
	Channels::WrapType		m_Wrap;
	Channels::BufferId		m_BufferId;
	int						m_iTextureSlot;
};