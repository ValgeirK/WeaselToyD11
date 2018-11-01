#include <fstream>
#include <windows.h>

#include "Loader.h"

#include "type/Channel.h"

bool LoadChannels(const char* channelPath, Channel* channels, int& size)
{
	// Open file
	std::ifstream inputFile(channelPath, std::ios::in);

	// Make sure the file was opened
	if (!inputFile)
	{
		MessageBox(nullptr,
			(LPCSTR)"Could not open channel file.", (LPCSTR)"Error", MB_OK);

		size = 0;
		return false;
	}

	// Initialize variables
	char line[256];
	Channel ch;
	int i = 0;

	while (inputFile.getline(line, 256))
	{
		switch (line[0])
		{
			// Initilize dummy variable
			char dummy[10];

			case 't':
			{
				// Initialize the type
				char type[10];

				sscanf(line, "%s %s", &dummy, &type);
				
				if (strcmp(type, "texture") == 0)
					ch.m_Type = Channels::ChannelType::E_Texture;
				if (strcmp(type, "buffer") == 0)
					ch.m_Type = Channels::ChannelType::E_Buffer;
			}
			case 's':
			{
				sscanf(line, "%s %i", &dummy, &size);
				break;
			}
			case 'i':
			{
				if (i != 0)
					channels[i-1] = ch;

				// Initialize variable for the texture path
				char texturePath[50];

				sscanf(line, "%s %s", &dummy, &texturePath);

				ch = Channel();
				strncpy(ch.m_strTexture, texturePath, strlen(texturePath) + 1);
				i++;

				break;
			}
			case 'f':
			{
				// Initialize the filter setting
				char filter[10];

				sscanf(line, "%s %s", &dummy, &filter);
				
				if(strcmp(filter, "mipmap") == 0)
					ch.m_Filter = Channels::FilterType::E_Mipmap;
				if (strcmp(filter, "nearest") == 0)
					ch.m_Filter = Channels::FilterType::E_Nearest;
				if (strcmp(filter, "linear") == 0)
					ch.m_Filter = Channels::FilterType::E_Linear;

				break;
			}
			case 'w':
			{
				// Initialize the wrapper setting
				char wrap[10];

				sscanf(line, "%s %s", &dummy, &wrap);

				if (strcmp(wrap, "clamp") == 0)
					ch.m_Wrap = Channels::WrapType::E_Clamp;
				if (strcmp(wrap, "repeat") == 0)
					ch.m_Wrap = Channels::WrapType::E_Repeat;

				break;
			}
			case 'b':
			{
				// initialize the buffer id
				char bufferId[2];

				sscanf(line, "%s %s", &dummy, &bufferId);

				if (strcmp(bufferId, "a") == 0)
				{
					ch.m_BufferId = Channels::BufferId::E_BufferA;
				}
				if (strcmp(bufferId, "b") == 0)
				{
					ch.m_BufferId = Channels::BufferId::E_BufferB;
				}
				if (strcmp(bufferId, "c") == 0)
				{
					ch.m_BufferId = Channels::BufferId::E_BufferC;
				}
				if (strcmp(bufferId, "d") == 0)
				{
					ch.m_BufferId = Channels::BufferId::E_BufferD;
				}
			}
			default:
				break;
		}
	}

	channels[i-1] = ch;

	return true;
}