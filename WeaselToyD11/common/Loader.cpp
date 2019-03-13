#include <fstream>
#include <windows.h>

#include "Loader.h"

#include "../lib/imgui.h"

#include "type/Channel.h"
#include "type/HashDefines.h"

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
				else if (strcmp(type, "buffer") == 0)
					ch.m_Type = Channels::ChannelType::E_Buffer;
				else
				{
					// only supporting three types suported
					assert(strcmp(type, "texture") == 0
						|| strcmp(type, "buffer") == 0
						|| strcmp(type, "none") == 0);
				}
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
				else if (strcmp(filter, "nearest") == 0)
					ch.m_Filter = Channels::FilterType::E_Nearest;
				else if (strcmp(filter, "linear") == 0)
					ch.m_Filter = Channels::FilterType::E_Linear;
				else
				{
					// only supporting three filter types
					assert(strcmp(filter, "mipmap") == 0
						|| strcmp(filter, "nearest") == 0
						|| strcmp(filter, "linear") == 0);
				}

				break;
			}
			case 'w':
			{
				// Initialize the wrapper setting
				char wrap[10];

				sscanf(line, "%s %s", &dummy, &wrap);

				if (strcmp(wrap, "clamp") == 0)
					ch.m_Wrap = Channels::WrapType::E_Clamp;
				else if (strcmp(wrap, "repeat") == 0)
					ch.m_Wrap = Channels::WrapType::E_Repeat;
				else
				{
					// only supporting two wrap types
					assert(strcmp(wrap, "clamp") == 0
						|| strcmp(wrap, "repeat") == 0);
				}

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
				else if (strcmp(bufferId, "b") == 0)
				{
					ch.m_BufferId = Channels::BufferId::E_BufferB;
				}
				else if (strcmp(bufferId, "c") == 0)
				{
					ch.m_BufferId = Channels::BufferId::E_BufferC;
				}
				else if (strcmp(bufferId, "d") == 0)
				{
					ch.m_BufferId = Channels::BufferId::E_BufferD;
				}
				else
				{
					// we only have 4 available buffers
					assert(strcmp(bufferId, "a") == 0
						|| strcmp(bufferId, "b") == 0
						|| strcmp(bufferId, "c") == 0
						|| strcmp(bufferId, "d") == 0);
				}
			}
			default:
				break;
		}
	}

	if(size != 0)
		channels[i-1] = ch;

	inputFile.close();

	return true;
}

////////////////////////////////////////////////
// Enum lookup functions
////////////////////////////////////////////////
const char* GetType(Channels::ChannelType type)
{
	switch (type)
	{
	case Channels::ChannelType::E_None:
		return "none";
	case Channels::ChannelType::E_Buffer:
		return "buffer";
	case Channels::ChannelType::E_Texture:
		return "texture";
	default:
		// there are only 3 possible types
		assert(type == Channels::ChannelType::E_None
			|| type == Channels::ChannelType::E_Buffer
			|| type == Channels::ChannelType::E_Texture);
		break;
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
	default:
		// there are only 3 possible filter types
		assert(type == Channels::FilterType::E_Linear
			|| type == Channels::FilterType::E_Mipmap
			|| type == Channels::FilterType::E_Nearest);
		break;
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
	default:
		// there are only 2 possible wrap types
		assert(Channels::WrapType::E_Clamp
			|| Channels::WrapType::E_Repeat);
		break;
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
		// there are only 4 different buffers
		assert(Channels::BufferId::E_BufferA
			|| Channels::BufferId::E_BufferB
			|| Channels::BufferId::E_BufferC
			|| Channels::BufferId::E_BufferD);
		break;
	}

	return "";
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
	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
	{
		if (channels[i].m_strTexture[0] == 't')
			size++;
	}

	fprintf(file, "s %i\n\n", size);

	for (int i = 0; i < MAX_RESORCESCHANNELS; ++i)
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

bool WriteIni(const char* strProj, const char* strEditor, ImVec4 vColor, RECT vAppSize, DWORD shaderFlags, int iEditor, bool autoReload, bool bRenderdoc)
{
	FILE* file;

	// open file
	file = fopen("settings.ini", "w");

	fprintf(file, "project: %s\n", strProj);
	fprintf(file, "editor: %s\n", strEditor);
	fprintf(file, "editorEnum: %d\n", iEditor);
	fprintf(file, "autoReload: %d\n", autoReload);
	fprintf(file, "bgColor: %f %f %f %f\n", vColor.x, vColor.y, vColor.w, vColor.z);
	fprintf(file, "appSize: %d %d %d %d\n", vAppSize.left, vAppSize.right, vAppSize.top, vAppSize.bottom);
	fprintf(file, "shaderFlags: %ld\n", shaderFlags);
	fprintf(file, "renderdocFlag: %d\n", bRenderdoc);

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

bool ReadIni(std::string& strProj, std::string& strEditor, ImVec4& vColor, RECT& vAppSize, DWORD& shaderFlags, int& iEditor, bool& autoReload, bool& bRenderdoc)
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
	int autRld = 1;
	int useRenderdoc = 0;

	while (inputFile.getline(line, MAX_PATH))
	{
		sscanf(line, "%s", &dummy);

		if (strcmp(dummy, "project:") == 0)
			sscanf(line, "%s %s\n", &dummy, &proj);

		else if (strcmp(dummy, "editor:") == 0)
			GetEditorHelper(line, strEditor);

		else if (strcmp(dummy, "editorEnum:") == 0)
			sscanf(line, "%s %i\n", &dummy, &iEditor);

		else if (strcmp(dummy, "autoReload:") == 0)
			sscanf(line, "%s %i\n", &dummy, &autRld);

		else if (strcmp(dummy, "bgColor:") == 0)
			sscanf(line, "%s %f %f %f %f\n", &dummy, &vColor.x, &vColor.y, &vColor.w, &vColor.z);

		else if (strcmp(dummy, "appSize:") == 0)
			sscanf(line, "%s %d %d %d %d\n", &dummy, &vAppSize.left, &vAppSize.right, &vAppSize.top, &vAppSize.bottom);

		else if (strcmp(dummy, "shaderFlags:") == 0)
			sscanf(line, "%s %ld", &dummy, &shaderFlags);

		else if (strcmp(dummy, "renderdocFlag:") == 0)
			sscanf(line, "%s %i", &dummy, &useRenderdoc);
	}

	strProj = std::string(proj);
	autoReload = autRld == 1 ? true : false;
	bRenderdoc = useRenderdoc == 1 ? true : false;

	inputFile.close();

	return true;
}

bool RenderdocCheck(std::string dllLocation)
{
	int substringEnd = dllLocation.find("renderdoc.dll");

	std::string path = dllLocation.substr(0, substringEnd);

	path += std::string("renderdoc_app.h");

	std::ifstream inputFile(path, std::ios::in);

	char line[256];

	while (inputFile.getline(line, 256))
	{
		std::string check = std::string(line);

		int isFound = check.find("API_1_3");
		if (isFound >= 0)
		{
			inputFile.close();
			return true;
		}
	}

	inputFile.close();
	return false;
}