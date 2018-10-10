#pragma once

#include <stdio.h>

enum ChannelType
{
	texture = 0,
	buffer = 1
};

enum FilterType
{
	mipmap = 0,
	nearest = 1,
	linear = 2
};

enum WrapType
{
	clamp = 0,
	repeat = 1
};

struct Channel
{
	Channel() {};
	Channel(const char path[]) { strcpy(texture, path); };

	ChannelType type;
	int BufferId;
	char texture[50];
	FilterType filter;
	WrapType wrap;
};