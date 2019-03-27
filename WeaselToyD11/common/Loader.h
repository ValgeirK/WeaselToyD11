#pragma once

#include <string>

#include "type/Channel.h"

struct ImVec4;

bool LoadChannels(
	const char*			strChannelPath, 
	Channel*			channels, 
	int&				iSize
);

bool WriteChannel(
	const char*			strChannelPath, 
	Channel*			pChannels
);

bool WriteIni(
	const char*			strProj, 
	const char*			strEditor, 
	ImVec4				vColor, 
	RECT				vAppSize, 
	DWORD				shaderFlags, 
	int					iEditor, 
	bool				bAutoReload, 
	bool				bRenderdoc
);

bool ReadIni(
	std::string&		strProj, 
	std::string&		strEditor, 
	ImVec4&				vColor, 
	RECT&				vAppSize, 
	DWORD&				shaderFlags, 
	int&				iEditor, 
	bool&				bAutoReload, 
	bool&				bRenderdoc
);

bool RenderdocVersionCheck(std::string dllLocation);