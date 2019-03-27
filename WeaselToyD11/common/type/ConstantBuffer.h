#pragma once

#include <directxcolors.h>

#include "HashDefines.h"

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
	DirectX::XMFLOAT4 ChannelResolution[MAX_RESORCESCHANNELS];
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
	CustomizableBuffer()
	{
		m_pData = nullptr;

		m_fMin = -1;
		m_fMax = -1;
		m_fStep = -1;
	}

	CustomizableBuffer(const CustomizableBuffer& cb)
	{
		strcpy(m_strCommand, cb.m_strCommand);
		strcpy(m_strVariable, cb.m_strVariable);

		m_uOffset = cb.m_uOffset;
		m_iType = cb.m_iType;
		m_iSize = cb.m_iSize;
		m_fMin = cb.m_fMin;
		m_fMax = cb.m_fMax;
		m_fStep = cb.m_fStep;
		m_bIsDataSet = cb.m_bIsDataSet;

		if (m_iType == 2) // INT
		{
			m_pData = new int[m_iSize / sizeof(int)];

			for (int i = 0; i < m_iSize / sizeof(int); ++i)
				((int*)m_pData)[i] = ((int*)cb.m_pData)[i];
		}
		else if (m_iType == 3) // FLOAT
		{
			m_pData = new float[m_iSize / sizeof(float)];

			for (int i = 0; i < m_iSize / sizeof(float); ++i)
				((float*)m_pData)[i] = ((float*)cb.m_pData)[i];
		}
	}

	void*		m_pData;

	char		m_strCommand[MAX_PATH_LENGTH];
	char		m_strVariable[MAX_PATH_LENGTH];

	UINT		m_uOffset;
	int			m_iType;
	int			m_iSize;
	float		m_fMin;
	float		m_fMax;
	float		m_fStep;

	bool		m_bIsDataSet = false;
};