#pragma once

#include <string>

#include "type/Channel.h"

struct ImVec4;

bool LoadChannels(const char*, Channel*, int&);

bool WriteChannel(const char*, Channel*);

bool WriteIni(const char*, const char*, ImVec4, RECT, int, bool);

bool ReadIni(std::string&, std::string&, ImVec4&, RECT&, int&, bool&);