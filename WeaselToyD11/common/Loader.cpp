#include <fstream>
#include <windows.h>

#include "Loader.h"

#include "../lib/imgui.h"

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

////////////////////////////////////////////////
// Enum lookup functions
////////////////////////////////////////////////
const char* GetType(Channels::ChannelType type)
{
	switch (type)
	{
	case Channels::ChannelType::E_Buffer:
		return "buffer";
	case Channels::ChannelType::E_Texture:
		return "texture";
	}

	return "";
}

const char* GetFilter(Channels::FilterType type)
{
	switch (type)
	{
	case Channels::FilterType::E_Linear:
		return "linear";
	case Channels::FilterType::E_Mipmap:
		return "mipmap";
	case Channels::FilterType::E_Nearest:
		return "nearest";
	}

	return "";
}

const char* GetWrap(Channels::WrapType type)
{
	switch (type)
	{
	case Channels::WrapType::E_Clamp:
		return "clamp";
	case Channels::WrapType::E_Repeat:
		return "repeat";
	}

	return "";
}

const char* GetBuffer(Channels::BufferId id)
{
	switch (id)
	{
	case Channels::BufferId::E_BufferA:
		return "a";
	case Channels::BufferId::E_BufferB:
		return "b";
	case Channels::BufferId::E_BufferC:
		return "c";
	case Channels::BufferId::E_BufferD:
		return "d";
	default:
		return "a";
	}
}

////////////////////////////////////////////////
// Write Channel Settings File
////////////////////////////////////////////////

bool WriteChannel(const char* channelPath, Channel* channels)
{
	FILE* file;

	// open file
	file = fopen(std::string(channelPath).c_str(), "w");

	fprintf(file, "########## Texture Controls #############\n");

	int size = 0;
	for (int i = 0; i < 4; ++i)
	{
		if (channels[i].m_strTexture[0] == 't')
			size++;
	}

	fprintf(file, "s %i\n\n", size);

	for (int i = 0; i < size; ++i)
	{
		if (channels[i].m_Type >= 0)
		{
			fprintf(file, "iC %s\n", channels[i].m_strTexture);
			fprintf(file, "t %s\n", GetType(channels[i].m_Type));
			fprintf(file, "f %s\n", GetFilter(channels[i].m_Filter));
			fprintf(file, "w %s\n", GetWrap(channels[i].m_Wrap));
			fprintf(file, "b %s\n\n", GetBuffer(channels[i].m_BufferId));
		}
	}

	fclose(file);

	return true;
}

////////////////////////////////////////////////
// Write settings.ini file
////////////////////////////////////////////////

bool WriteIni(const char* strProj, const char* strEditor, ImVec4 vColor, RECT vAppSize, int iEditor, bool autoReload)
{
	FILE* file;

	// open file
	file = fopen("settings.ini", "w");

	fprintf(file, "project: %s\n", strProj);
	fprintf(file, "editor: %s\n", strEditor);
	fprintf(file, "editorEnum: %d\n", iEditor);
	fprintf(file, "autoReload: %d\n", autoReload);
	fprintf(file, "bgColor: %f %f %f %f\n", vColor.x, vColor.y, vColor.w, vColor.z);
	fprintf(file, "appSize: %d %d\n", vAppSize.right, vAppSize.bottom);

	fclose(file);

	return true;
}

////////////////////////////////////////////////
// Read settings.ini file
////////////////////////////////////////////////

void GetEditorHelper(const std::string line, std::string& editor)
{
	std::size_t index = line.find(' ');
	editor = line.substr(index + 1);
}

bool ReadIni(std::string& strProj, std::string& strEditor, ImVec4& vColor, RECT& vAppSize, int& iEditor, bool& autoReload)
{
	// Open file
	std::ifstream inputFile("settings.ini", std::ios::in);

	// Make sure the file was opened
	if (!inputFile)
	{
		return false;
	}

	// Initialize variables
	char line[MAX_PATH] = "";

	char dummy[MAX_PATH] = "";
	char proj[MAX_PATH] = "";
	char editor[MAX_PATH] = "";
	int enumer = 1;

	inputFile.getline(line, MAX_PATH);
	sscanf(line, "%s %s\n", &dummy, &proj);

	inputFile.getline(line, MAX_PATH);
	GetEditorHelper(line, strEditor);

	inputFile.getline(line, MAX_PATH);
	sscanf(line, "%s %i\n", &dummy, &iEditor);

	inputFile.getline(line, MAX_PATH);
	sscanf(line, "%s %i\n", &dummy, &enumer);

	inputFile.getline(line, MAX_PATH);
	sscanf(line, "%s %f %f %f %f\n", &dummy, &vColor.x, &vColor.y, &vColor.w, &vColor.z);

	inputFile.getline(line, MAX_PATH);
	sscanf(line, "%s %d %d\n", &dummy, &vAppSize.right, &vAppSize.bottom);

	strProj = std::string(proj);
	autoReload = enumer == 1 ? true : false;

	return true;
}