#include <stdlib.h>
#include <stddef.h>
#include <ctime>

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

#include <vector>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "HelperFunction.h"

void SetDebugObjectName(ID3D11DeviceChild* pResource, const char * pName)
{
#ifdef USE_DEBUG_MARKERS
	size_t NumChars = strlen(pName);
	//pResource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)NumChars+1, pName);
#else
	UNREFERENCED_PARAMETER(resource);
	UNREFERENCED_PARAMETER(name);
#endif // USE_DEBUG_MARKERS
}

void SplitString(const std::string& inStr, std::vector<std::string>& container, char delim)
{
	std::stringstream ss(inStr);
	std::string token;
	while (std::getline(ss, token, delim)) {
		container.push_back(token);
	}
}
