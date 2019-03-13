#pragma once

#include <string>

#include "type/Channel.h"

struct ImVec4;

bool LoadChannels(
	const char* channelPath, 
	Channel* channels, 
	int& size
);

bool WriteChannel(
	const char* channelPath, 
	Channel* channels
);

bool WriteIni(
	const char* strProj, 
	const char* strEditor, 
	ImVec4 vColor, 
	RECT vAppSize, 
	DWORD shaderFlags, 
	int iEditor, 
	bool autoReload, 
	bool bRenderdoc
);

bool ReadIni(
	std::string& strProj, 
	std::string& strEditor, 
	ImVec4& vColor, 
	RECT& vAppSize, 
	DWORD& shaderFlags, 
	int& iEditor, 
	bool& autoReload, 
	bool& bRenderdoc
);

bool RenderdocCheck(std::string dllLocation);