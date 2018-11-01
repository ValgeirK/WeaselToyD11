#pragma once

#if defined(_DEBUG) || defined(PROFILE)
#define USE_DEBUG_MARKERS
#endif

#include <vector>
#include <string.h>

struct ID3D11DeviceChild;

//--------------------------------------------------------------------------------------
// Helper Functions
//--------------------------------------------------------------------------------------

void SetDebugObjectName(ID3D11DeviceChild* pResource, const char * pName);


void SplitString(const std::string& inStr, std::vector<std::string>& container, char delim);