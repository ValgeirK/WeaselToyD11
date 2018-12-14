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
	CustomizableBuffer()
	{
		data = nullptr;

		min = -1;
		max = -1;
		step = -1;
	}

	CustomizableBuffer(const CustomizableBuffer& cb)
	{
		strcpy(strCommand, cb.strCommand);
		strcpy(strVariable, cb.strVariable);

		offset = cb.offset;
		type = cb.type;
		size = cb.size;
		min = cb.min;
		max = cb.max;
		step = cb.step;
		isDataSet = cb.isDataSet;

		if (type == 2) // INT
		{
			data = new int[size / 4];

			for (int i = 0; i < size / 4; ++i)
				((int*)data)[i] = ((int*)cb.data)[i];
		}
		else if (type == 3) // FLOAT
		{
			data = new float[size / 4];

			for (int i = 0; i < size / 4; ++i)
				((float*)data)[i] = ((float*)cb.data)[i];
		}
	}

	void* data;

	char strCommand[MAX_PATH];
	char strVariable[MAX_PATH];

	UINT offset;
	int type;
	int size;
	float min;
	float max;
	float step;

	bool isDataSet = false;
};