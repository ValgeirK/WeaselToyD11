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
#include "HelperFunction.h"

#define SINGLE_THREAD_LOADING true

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
	typedef void (*TextureDeleteFunc)(void*);

	uint8_t* ddsData;
	const uint8_t* bitData;
	size_t bitSize = 0;
	const DirectX::DDS_HEADER* header;
	uint32_t width = 0;
	uint32_t height = 0;
	TextureDeleteFunc blockDestructor;

	int id;
} TEXTUREMEMORY;

DWORD WINAPI ThreadFileLoader(LPVOID lpParam);
DWORD WINAPI ThreadTextureLoader(LPVOID lpParam);

std::vector< TextureMemory > filesLoaded;

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

	delete m_pLoadData;
	m_pLoadData = nullptr;
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
			temp[i] = new char[MAX_PATH_LENGTH];
			memset(temp[i], 0, MAX_PATH_LENGTH);
			strcpy_s(temp[i], MAX_PATH_LENGTH, m_ppPath[i]);
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
			m_ppPath[i] = new char[MAX_PATH_LENGTH];
			strcpy_s(m_ppPath[i], MAX_PATH_LENGTH, temp[i]);
			m_pIsSet[i] = tempSet[i];
			delete[] temp[i];
		}

		delete[] temp;
		delete[] tempSet;
	}
	
	m_ppPath[m_iLength] = new char[MAX_PATH_LENGTH];
	memset(m_ppPath[m_iLength], 0, MAX_PATH_LENGTH);
	strcpy_s(m_ppPath[m_iLength], MAX_PATH_LENGTH, textPath);

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

			return;
		}
	}

	*shaderRes = m_pShaderResource[m_iLength - 1];
	m_bReload = true;
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

			return;
		}
	}

	channelRes = m_pResolution[m_iLength - 1];
	*shaderRes = m_pShaderResource[m_iLength - 1];
	m_bReload = true;
}

HRESULT TextureLib::InitializeViewsAndRes()
{
	HRESULT hr = S_OK;

	m_pShaderResource = new ID3D11ShaderResourceView*[m_iLength] { nullptr };
	m_pResolution = new DirectX::XMFLOAT4[m_iLength];

	return hr;
}

void TextureLib::FindTexturePaths(const char* path)
{
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(path, &data);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		PIXBeginEvent(PIX_COLOR_INDEX((byte)4), "Finding Textures");

		do
		{
			const char* fileName = data.cFileName;
			std::string strPath = std::string(path);

			// dwFileAttributes

			if (data.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			{
				// this could be a .dds texture
				size_t length = strlen(fileName);

				char ext[5];
				memcpy(ext, &fileName[length - 4], 4);
				ext[4] = '\0';

				// this is a .dds texture
				Add(std::string(strPath.substr(0, strPath.length() - 1) + std::string(fileName)).c_str());
			}
			else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (strcmp(fileName, ".") != 0 && strcmp(fileName, "..") != 0)
				{
					FindTexturePaths(std::string(strPath.substr(0, strPath.length()-1) + std::string(fileName) + "/*").c_str());
				}
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
		PIXEndEvent(); // FINDINGTEXTURES
	}
}

HRESULT TextureLib::ParallelLoadDDSTextures(ID3D11Device* device, const char* path)
{
	// Starts by going through the texture directory and find all the textures with the .dds extension
	assert(device != nullptr);

	HRESULT hr = S_OK;

	FindTexturePaths(path);

	InitializeViewsAndRes();

	// If we have any textures then we want to load them
	if (m_iLength > 0)
	{
		PIXBeginEvent(0, "ThreadedTextureLoading");
		// Threaded version
		InitializeCriticalSection(&CriticalSection);
		InitializeConditionVariable(&BufferNotEmpty);

		DWORD   dwThreadIdArrayFile, dwThreadIdArrayTexture;
		HANDLE threadFileLoad, threadTextureLoad;

		m_pLoadData = new LoadData();

		m_pLoadData->device = device;
		m_pLoadData->textureLib = this;

#if SINGLE_THREAD_LOADING
		// For single thread texture loading
		LoadTexturesSingleThread(device);

#else
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
#endif

		PIXEndEvent(); // THREADEDTEXTURELOADING

	}

	return hr;
}

/////////////////////////////////////////////////////////////
// Single Thread related functions
/////////////////////////////////////////////////////////////


void TextureLib::LoadTexturesSingleThread(ID3D11Device* pDevice)
{
	assert(pDevice != nullptr);

	for (int i = 0; i < m_iLength; ++i)
	{
		HRESULT hr = S_OK;

		TextureMemory tm;

		const size_t cSize = strlen(m_ppPath[i]) + 1;
		wchar_t* path = new wchar_t[cSize];
		mbstowcs(path, m_ppPath[i], cSize);

		const char* pathExtension = "";
		GetFileExtension(m_ppPath[i], &pathExtension);

		uint32_t width = 0, height = 0;

		tm.id = i;
		if (strcmp(pathExtension, "dds") == 0)
		{
			std::unique_ptr<uint8_t[]> ddsData;

			LoadDDSTextureFile(path, ddsData, &tm.header, &tm.bitData, &tm.bitSize);

			hr = DirectX::CreateDDSTextureFromFileCustom(
				pDevice,
				path,
				const_cast<uint8_t*>(tm.bitData),
				tm.header,
				tm.bitData,
				tm.bitSize,
				nullptr,
				&m_pShaderResource[tm.id]);

			assert(SUCCEEDED(hr));

			hr = DirectX::GetTextureInformation(path, width, height);

			assert(SUCCEEDED(hr));

			uint8_t* pDataPointer = ddsData.release();
			delete pDataPointer;
			pDataPointer = nullptr;
		}
		else
		{
			int nrChannels = 0;
			void* pStbData = nullptr;
			void * p32BitData = nullptr;

			LoadStbTexture(m_ppPath[i], &pStbData, nrChannels, width, height);

			assert(nrChannels == 3 || nrChannels == 4 && "Unhandled case");
			assert(pStbData != nullptr);

			if (nrChannels == 3)
			{
				// Need to add an alpha mask
				p32BitData = AddAlphaMask(pStbData, width * height);
				assert(p32BitData != nullptr);
			}
			else
				p32BitData = pStbData;

			// free up stb data
			FreeStbTexture(pStbData);
			pStbData = nullptr;

			hr = CreateRbtTexture(pDevice, p32BitData, &m_pShaderResource[tm.id], width, height);
			assert(SUCCEEDED(hr));

			// free up the 32-bit data
			delete[] p32BitData;
		}

		m_pResolution[tm.id] = DirectX::XMFLOAT4((float)width, (float)height, 0.0f, 0.0f);
		m_pIsSet[tm.id] = true;

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

	for (int i = 0; i < ((TextureLib*)data->textureLib)->m_iLength; ++i)
	{
		TextureMemory tm;

		// PIX Event
		PIXBeginEvent(0, L"Thread_LoadingTextures");

		const size_t cSize = strlen(((TextureLib*)data->textureLib)->m_ppPath[i]) + 1;
		wchar_t* path = new wchar_t[cSize];
		mbstowcs(path, ((TextureLib*)data->textureLib)->m_ppPath[i], cSize);

		const char* pathExtension = "";
		GetFileExtension(((TextureLib*)data->textureLib)->m_ppPath[i], &pathExtension);

		if (strcmp(pathExtension, "dds") == 0)
		{
			std::unique_ptr<uint8_t[]> ddsData;

			LoadDDSTextureFile(path, ddsData, &tm.header, &tm.bitData, &tm.bitSize);
			assert(tm.bitData != nullptr);

			tm.ddsData = ddsData.release();
		}
		else
		{
			int nrChannels = 0;
			void* pStbData = nullptr;
			void* p32BitData = nullptr;
			LoadStbTexture(((TextureLib*)data->textureLib)->m_ppPath[i], &pStbData, nrChannels, tm.width, tm.height);
			
			assert(pStbData != nullptr);
			assert(tm.width > 0);
			assert(tm.height > 0);
			
			assert(nrChannels == 3 || nrChannels == 4 && "Unsupported!");
			if (nrChannels == 3)
			{
				// Need to add an alpha mask
				p32BitData = AddAlphaMask(pStbData, tm.width * tm.height);
				assert(p32BitData != nullptr);
				tm.blockDestructor = &DeleteArrayWrapper;
			}
			else
			{
				p32BitData = pStbData;
				tm.blockDestructor = &free;
			}

			FreeStbTexture(pStbData);
			pStbData = nullptr;


			tm.bitData = (uint8_t*)p32BitData;
		}

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

	int counter = 0;

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

		const char* pathExtension = "";
		GetFileExtension(((TextureLib*)data->textureLib)->m_ppPath[tm.id], &pathExtension);

		if (strcmp(pathExtension, "dds") == 0)
		{
			DirectX::CreateDDSTextureFromFileCustom(
				data->device,
				path,
				const_cast<uint8_t*>(tm.bitData),
				tm.header,
				tm.bitData,
				tm.bitSize,
				nullptr,
				&((TextureLib*)data->textureLib)->m_pShaderResource[tm.id]);

			assert(((TextureLib*)data->textureLib)->m_pShaderResource[tm.id] != nullptr);

			DirectX::GetTextureInformation(path, tm.width, tm.height);

			delete tm.ddsData;
			tm.ddsData = nullptr;
		}
		else
		{
			CreateRbtTexture(data->device, (void*)tm.bitData, &((TextureLib*)data->textureLib)->m_pShaderResource[tm.id], tm.width, tm.height);
			
			assert(((TextureLib*)data->textureLib)->m_pShaderResource[tm.id] != nullptr);

			// free up the 32-bit data
			tm.blockDestructor((void*)tm.bitData);
		}

		((TextureLib*)data->textureLib)->m_pResolution[tm.id] = DirectX::XMFLOAT4((float)tm.width, (float)tm.height, 0.0f, 0.0f);

		((TextureLib*)data->textureLib)->m_pIsSet[tm.id] = true;
		counter++;

		delete[] path;

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