#include "FileIO.h"

#include <assert.h>
#include <algorithm>
#include <memory>

#include <dxgiformat.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

#include "DDSTextureLoader.h"

const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_BUMPDUDV    0x00080000  // DDPF_BUMPDUDV

struct DDS_HEADER_DXT10
{
	DXGI_FORMAT     dxgiFormat;
	uint32_t        resourceDimension;
	uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
	uint32_t        arraySize;
	uint32_t        miscFlags2;
};

struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

typedef public std::unique_ptr<void, handle_closer> ScopedHandle;

inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? nullptr : h; }

HRESULT LoadDDSTextureFile(const wchar_t * fileName, std::unique_ptr<uint8_t[]>& ddsData, const DirectX::DDS_HEADER ** header, const uint8_t ** bitData, size_t * bitSize)
{
	if (!header || !bitData || !bitSize)
	{
		return E_POINTER;
	}

	// open the file
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	ScopedHandle hFile(safe_handle(CreateFile2(fileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		OPEN_EXISTING,
		nullptr)));
#else
	ScopedHandle hFile(safe_handle(CreateFileW(fileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr)));
#endif

	if (!hFile)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// Get the file size
	FILE_STANDARD_INFO fileInfo;
	if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// File is too big for 32-bit allocation, so reject read
	if (fileInfo.EndOfFile.HighPart > 0)
	{
		return E_FAIL;
	}

	// Need at least enough data to fill the header and magic number to be a valid DDS
	if (fileInfo.EndOfFile.LowPart < (sizeof(DirectX::DDS_HEADER) + sizeof(uint32_t)))
	{
		return E_FAIL;
	}

	// create enough space for the file data
	ddsData.reset(new (std::nothrow) uint8_t[fileInfo.EndOfFile.LowPart]);
	if (!ddsData)
	{
		return E_OUTOFMEMORY;
	}

//	PIXBeginEvent(0, "ThreadFileRead");
	// read the data in
	DWORD BytesRead = 0;
	if (!ReadFile(hFile.get(),
		ddsData.get(),
		fileInfo.EndOfFile.LowPart,
		&BytesRead,
		nullptr
	))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

//	PIXEndEvent();

	if (BytesRead < fileInfo.EndOfFile.LowPart)
	{
		return E_FAIL;
	}

	// DDS files always start with the same magic number ("DDS ")
	uint32_t dwMagicNumber = *reinterpret_cast<const uint32_t*>(ddsData.get());
	if (dwMagicNumber != DDS_MAGIC)
	{
		return E_FAIL;
	}

	auto hdr = reinterpret_cast<const DirectX::DDS_HEADER*>(ddsData.get() + sizeof(uint32_t));

	// Verify header to validate DDS file
	if (hdr->size != sizeof(DirectX::DDS_HEADER) ||
		hdr->ddspf.size != sizeof(DirectX::DDS_PIXELFORMAT))
	{
		return E_FAIL;
	}

	// Check for DX10 extension
	bool bDXT10Header = false;
	if ((hdr->ddspf.flags & DDS_FOURCC) &&
		(MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.fourCC))
	{
		// Must be long enough for both headers and magic value
		if (fileInfo.EndOfFile.LowPart < (sizeof(DirectX::DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10)))
		{
			return E_FAIL;
		}

		bDXT10Header = true;
	}

	// setup the pointers in the process request
	*header = hdr;
	ptrdiff_t offset = sizeof(uint32_t) + sizeof(DirectX::DDS_HEADER)
		+ (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0);
	*bitData = ddsData.get() + offset;
	*bitSize = fileInfo.EndOfFile.LowPart - offset;

	return S_OK;
}

HRESULT LoadStbTexture(const char * imagepath, void** data, int&nrChannels, uint32_t& width, uint32_t& height)
{
	assert(strlen(imagepath) > 0);

	*data = (void*)stbi_load(imagepath, (int*)&width, (int*)&height, &nrChannels, 0);

	assert(*data != nullptr);
	assert(width > 0);
	assert(height > 0);
	assert(nrChannels > 0);

	return S_OK;
}

void FreeStbTexture(void* data)
{
	assert(data != nullptr);
	stbi_image_free(data);
}