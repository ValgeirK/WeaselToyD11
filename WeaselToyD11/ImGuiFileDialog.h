#ifndef __IMGUI_FILE_DIALOG_H_
#define __IMGUI_FILE_DIALOG_H_

#include <d3d11.h>

#include <vector>
#include <string>

#include "common/TextureLib.h"

#define MAX_FILE_DIALOG_NAME_BUFFER 1024

struct FileInfoStruct
{
	char type;
	std::string filePath;
	std::string fileName;
	std::string ext;
};

class ImGuiFileDialog
{
private:
	std::vector<FileInfoStruct> m_FileList;
	std::string m_SelectedFileName;
	std::string m_OldFileName = "";
	std::string m_CurrentPath;
	std::vector<std::string> m_CurrentPath_Decomposition;
	std::string m_CurrentFilterExt;
	int m_CurrentPadding = -1;

	int						   peakWidth = 256;
	int						   peakHeight = 256;

	TextureLib*				   pTextureLib;

	ID3D11Texture2D*		   mRenderTargetTexture;
	ID3D11ShaderResourceView*  mShaderResourceView;

public:
	static char FileNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER];
	static int FilterIndex;
	bool IsOk;

public:
	static ImGuiFileDialog* Instance(TextureLib* texLib)
	{
		static ImGuiFileDialog *_instance = new ImGuiFileDialog(texLib);
		return _instance;
	}

public:
	ImGuiFileDialog(TextureLib*);
	~ImGuiFileDialog();

	bool FileDialog(const char* vName, std::string& vSelected, int iPadding, const char* vFilters = 0, std::string vPath = ".", std::string vDefaultFileName = "");
	bool FileDialog(const char* vName, const char* vFilters = 0, std::string vPath = ".", std::string vDefaultFileName = "");
	void Clear();
	std::string GetFilepathName();
	std::string GetCurrentPath();
	std::string GetCurrentFileName();
	std::string GetCurrentFilter();
	std::string GetSelectedFile();

private:
	void ScanDir(std::string vPath);
	void SetCurrentDir(std::string vPath);
	void ComposeNewPath(std::vector<std::string>::iterator vIter);
};


#endif // __IMGUI_FILE_DIALOG_H_