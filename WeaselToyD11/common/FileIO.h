#include <windows.h>

#include <assert.h>
#include <algorithm>
#include <memory>

#include "DDSTextureLoader.h"

HRESULT LoadTextureFile(
	_In_z_ const wchar_t* fileName,
	std::unique_ptr<uint8_t[]>& ddsData,
	const DirectX::DDS_HEADER** header,
	const uint8_t** bitData,
	size_t* bitSize
);