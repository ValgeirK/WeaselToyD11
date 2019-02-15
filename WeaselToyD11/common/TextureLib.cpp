#include "TextureLib.h"

#include <windows.h>
#include <pix3.h>
#include <d3d11.h>
#include <directxcolors.h>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <memory>
#include <sstream>

#include "Textures.h"
#include "FileIO.h"
#include "DDSTextureLoader.h"

#define BUFFER_SIZE 20

CONDITION_VARIABLE BufferNotEmpty;
CONDITION_VARIABLE BufferNotFull;
CRITICAL_SECTION CriticalSection;

struct LoadData
{
	ID3D11Device*  device;
	void*    textureLib;
};

struct TextureMemory
{
	uint8_t* ddsData;
	const uint8_t* bitData;
	size_t bitSize = 0;
	const DirectX::DDS_HEADER* header;

	int id;
} TEXTUREMEMORY;

DWORD WINAPI ThreadFileLoader(LPVOID lpParam);
DWORD WINAPI ThreadTextureLoader(LPVOID lpParam);

std::vector< TextureMemory > filesLoaded;

//TextureInfo
//{
//	FileName;
//	SIZE_T;
//
//	num mips.
//		flags;
//	COMPRESSION;
//};
//arrayof(textureinfo);

TextureLib::TextureLib() : m_iLength(0), m_iCapacity(1)
{
	m_ppPath = new char*[m_iCapacity];
	m_pIsSet = new bool[m_iCapacity];
}

TextureLib::~TextureLib()
{
	// Release resources used by the critical section object.
	DeleteCriticalSection(&CriticalSection);

	for (int i = 0; i < m_iLength; ++i)
	{
		delete[] m_ppPath[i];
		m_ppPath[i] = nullptr;

		m_pShaderResource[i]->Release();
		m_pShaderResource[i] = nullptr;
	}

	delete[] m_ppPath;
	m_ppPath = nullptr;

	delete[] m_pShaderResource;
	m_pShaderResource = nullptr;

	delete[] m_pIsSet;
	m_pIsSet = nullptr;

	HeapFree(GetProcessHeap(), 0, m_pLoadData);
}

void TextureLib::Release()
{ 
	// Release resources used by the critical section object.
	DeleteCriticalSection(&CriticalSection);

	// Release all the DirectX resources
	for (int i = 0; i < m_iLength; ++i)
	{
		delete[] m_ppPath[i];
		m_ppPath[i] = nullptr;

		ULONG refs = 0;
		if (m_pShaderResource[i])
			m_pShaderResource[i]->Release();
		else
			refs = 0;
		if (refs > 0)
		{
			_RPTF2(_CRT_WARN, "TextureLib %s still has %i references.\n", m_ppPath[i], refs);
		}
	}

	delete[] m_ppPath;
	m_ppPath = nullptr;

	delete[] m_pShaderResource;
	m_pShaderResource = nullptr;

	delete[] m_pIsSet;
	m_pIsSet = nullptr;

	HeapFree(GetProcessHeap(), 0, m_pLoadData);
}

void TextureLib::Add(const char* textPath)
{
	// Adding a texture path to the array of texture paths
	// the directory is hard coded, expecting all the textures to be in the textures folder
	char direct[50] = "textures/";
	
	if (m_iLength >= m_iCapacity)
	{
		char** temp;
		bool* tempSet;

		temp = new char*[2 * m_iCapacity];
		tempSet = new bool[2 * m_iCapacity];
		
		for (int i = 0; i < m_iCapacity; ++i)
		{
			temp[i] = new char[MAX_PATH];
			memset(temp[i], 0, MAX_PATH);
			strcpy_s(temp[i], MAX_PATH, m_ppPath[i]);
			delete[] m_ppPath[i];
			tempSet[i] = m_pIsSet[i];
		}

		m_iCapacity *= 2;
		delete[] m_ppPath;
		delete m_pIsSet;

		m_ppPath = new char*[m_iCapacity];
		m_pIsSet = new bool[m_iCapacity];

		for (int i = 0; i < m_iCapacity / 2; ++i)
		{
			m_ppPath[i] = new char[MAX_PATH];
			strcpy_s(m_ppPath[i], MAX_PATH, temp[i]);
			m_pIsSet[i] = tempSet[i];
			delete[] temp[i];
		}

		delete[] temp;
		delete[] tempSet;
	}
	
	m_ppPath[m_iLength] = new char[MAX_PATH];
	memset(m_ppPath[m_iLength], 0, MAX_PATH);
	strncat(direct, textPath, strlen(textPath));
	strcpy_s(m_ppPath[m_iLength], MAX_PATH, direct);

	m_pIsSet[m_iLength] = false;

	m_iLength++;
}

void TextureLib::GetTexture(const char* desiredTex, ID3D11ShaderResourceView** shaderRes)
{
	if (shaderRes)
	{
		*shaderRes = nullptr;
	}

	for (int i = 0; i < m_iLength; ++i)
	{
		if (strcmp(desiredTex, m_ppPath[i]) == 0)
		{
			if (m_pIsSet[i])
				*shaderRes = m_pShaderResource[i];
			else
			{
				*shaderRes = m_pShaderResource[0];
				this->m_bReload = true;
			}
			return;
		}
	}
}

void TextureLib::GetTexture(const char* desiredTex, ID3D11ShaderResourceView** shaderRes, DirectX::XMFLOAT4& channelRes)
{
	if (shaderRes)
	{
		*shaderRes = nullptr;
	}

	for (int i = 0; i < m_iLength; ++i)
	{
		if (strcmp(desiredTex, m_ppPath[i]) == 0)
		{
			if (m_pIsSet[i])
			{
				channelRes = m_pResolution[i];
				*shaderRes = m_pShaderResource[i];
			}
			else
			{
				channelRes = m_pResolution[0];
				*shaderRes = m_pShaderResource[0];
				this->m_bReload = true;
			}
			return;
		}
	}
}

HRESULT TextureLib::LoadDefaultTexture(ID3D11Device* device)
{
	HRESULT hr = S_OK;

	const char* path = "default.dds";
	this->Add(path);

	m_pShaderResource = new ID3D11ShaderResourceView*[m_iLength] { nullptr };
	m_pResolution = new DirectX::XMFLOAT4[m_iLength];

	hr = LoadTexture(device, &m_pShaderResource[0], this->m_ppPath[0], m_pResolution[0]);

	this->m_pIsSet[0] = true;

	return hr;
}

HRESULT TextureLib::ParallelLoadDDSTextures(ID3D11Device* device, const char* path)
{
	// Starts by going through the texture directory and find all the textures with the .dds extension

	HRESULT hr = S_OK;

	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(path, &data);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		{
			PIXBeginEvent(PIX_COLOR_INDEX((byte)4), "Finding Textures");
			do
			{
				const char* fileName = data.cFileName;

				// dwFileAttributes

				if (data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
				{
					// this could be a .dds texture
					size_t length = strlen(fileName);

					char ext[5];
					memcpy(ext, &fileName[length - 4], 4);
					ext[4] = '\0';

					if (strcmp(ext, ".dds") == 0)
					{
						// do not want to load the default texture again
						if (strcmp(fileName, "default.dds") != 0)
							// this is a .dds texture
							this->Add(fileName);
					}
				}
			} while (FindNextFile(hFind, &data));
			FindClose(hFind);
			PIXEndEvent(); // FINDINGTEXTURES
		}
	}

	this->LoadDefaultTexture(device);

	// If we have any textures then we want to load them

	if (this->m_iLength > 0)
	{
		PIXBeginEvent(0, "ThreadedTextureLoading");
		// Threaded version
		InitializeCriticalSection(&CriticalSection);
		InitializeConditionVariable(&BufferNotEmpty);

		DWORD   dwThreadIdArrayFile, dwThreadIdArrayTexture;
		HANDLE threadFileLoad, threadTextureLoad;

		m_pLoadData = (LoadData*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LoadData));

		m_pLoadData->device = device;
		m_pLoadData->textureLib = this;

		// For single thread texture loading
		//LoadTexturesSingleThread(device);

		threadFileLoad = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			ThreadFileLoader,		// thread function name
			m_pLoadData,              // argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArrayFile        // returns the thread identifier 
		);
				
		threadTextureLoad = CreateThread(
			NULL,
			0,
			ThreadTextureLoader,
			m_pLoadData,
			0,
			&dwThreadIdArrayTexture
		);

		PIXEndEvent(); // THREADEDTEXTURELOADING

	}

	return hr;
}

/////////////////////////////////////////////////////////////
// Single Thread related functions
/////////////////////////////////////////////////////////////


void TextureLib::LoadTexturesSingleThread(ID3D11Device* device)
{
	for (int i = 1; i < this->m_iLength; ++i)
	{
		TextureMemory tm;

		const size_t cSize = strlen(this->m_ppPath[i]) + 1;
		wchar_t* path = new wchar_t[cSize];
		mbstowcs(path, this->m_ppPath[i], cSize);

		std::unique_ptr<uint8_t[]> ddsData;

		LoadTextureFile(path, ddsData, &tm.header, &tm.bitData, &tm.bitSize);

		//tm.ddsData = ddsData.release();
		tm.id = i;

		DirectX::CreateDDSTextureFromFileCustom(
			device,
			path,
			const_cast<uint8_t*>(tm.bitData),
			tm.header,
			tm.bitData,
			tm.bitSize,
			nullptr,
			&this->m_pShaderResource[tm.id]);

		uint32_t width = 0, height = 0;
		DirectX::GetTextureInformation(path, width, height);

		this->m_pResolution[tm.id] = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);

		this->m_pIsSet[tm.id] = true;

		uint8_t* pDataPointer = ddsData.release();
		delete pDataPointer;
		pDataPointer = nullptr;

		delete[] path;
		path = nullptr;
	}
}

/////////////////////////////////////////////////////////////
// Thread related functions
/////////////////////////////////////////////////////////////

DWORD WINAPI ThreadFileLoader(LPVOID lpParam)
{
	LoadData* data = (LoadData*)lpParam;

	for (int i = 1; i < ((TextureLib*)data->textureLib)->m_iLength; ++i)
	{
		TextureMemory tm;

		// PIX Event
		PIXBeginEvent(0, L"Thread_LoadingTextures");

		const size_t cSize = strlen(((TextureLib*)data->textureLib)->m_ppPath[i]) + 1;
		wchar_t* path = new wchar_t[cSize];
		mbstowcs(path, ((TextureLib*)data->textureLib)->m_ppPath[i], cSize);

		std::unique_ptr<uint8_t[]> ddsData;

		LoadTextureFile(path, ddsData, &tm.header, &tm.bitData, &tm.bitSize);

		tm.ddsData = ddsData.release();
		tm.id = i;

		// need critical section

		// Request ownership of the critical section.
		EnterCriticalSection(&CriticalSection);

		filesLoaded.push_back(tm);

		// Release ownership of the critical section.
		LeaveCriticalSection(&CriticalSection);

		WakeConditionVariable(&BufferNotEmpty);

		delete[] path;

		PIXEndEvent(); // THREAD_LOADINGTEXTURES
	}
	
	/*while (true)
	{
		SleepConditionVariableCS(&BufferNotEmpty, &CriticalSection, INFINITE);
	}*/

	return 0;
}


DWORD WINAPI ThreadTextureLoader(LPVOID lpParam)
{
	// Cast the input into the correct type
	LoadData* data = (LoadData*)lpParam;

	TextureMemory tm;

	int counter = 1;

	while (true)
	{
		// PIX Event
		PIXBeginEvent(0, L"Thread_LoadToShaderView");

		// Request ownership of the critical section.
		EnterCriticalSection(&CriticalSection);

		while ((filesLoaded.size() == 0))
		{
			SleepConditionVariableCS(&BufferNotEmpty, &CriticalSection, INFINITE);
		}

		tm = filesLoaded.back();
		filesLoaded.pop_back();

		// Release ownership of the critical section.
		LeaveCriticalSection(&CriticalSection);

		const size_t cSize = strlen(((TextureLib*)data->textureLib)->m_ppPath[tm.id]) + 1;
		wchar_t* path = new wchar_t[cSize];
		mbstowcs(path, ((TextureLib*)data->textureLib)->m_ppPath[tm.id], cSize);

		DirectX::CreateDDSTextureFromFileCustom(
			data->device,
			path,
			const_cast<uint8_t*>(tm.bitData),
			tm.header,
			tm.bitData,
			tm.bitSize,
			nullptr,
			&((TextureLib*)data->textureLib)->m_pShaderResource[tm.id]);

		uint32_t width = 0, height = 0;
		DirectX::GetTextureInformation(path, width, height);

		((TextureLib*)data->textureLib)->m_pResolution[tm.id] = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);

		((TextureLib*)data->textureLib)->m_pIsSet[tm.id] = true;
		counter++;

		delete[] path;

		delete tm.ddsData;
		tm.ddsData = nullptr;

		PIXEndEvent(); // THREAD_LOADTOSHADERVIEW

		// Exit since we loaded all the textures
		if (counter >= ((TextureLib*)data->textureLib)->m_iLength && filesLoaded.size() == 0)
		{
			((TextureLib*)data->textureLib)->m_bReload = true;
			break;
		}
	}

	return 0;
}