#include <windows.h>

#include <assert.h>
#include <algorithm>
#include <memory>

#include "DDSTextureLoader.h"

HRESULT LoadDDSTextureFile(
	_In_z_ const wchar_t* fileName,
	std::unique_ptr<uint8_t[]>& ddsData,
	const DirectX::DDS_HEADER** header,
	const uint8_t** bitData,
	size_t* bitSize
);

HRESULT LoadStbTexture(
	const char * imagepath, 
	void** data, 
	int&nrChannels, 
	uint32_t& width,
	uint32_t& height
);

void FreeStbTexture(void* data);