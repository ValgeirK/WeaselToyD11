#include <windows.h>
#include <string>

#include "RegisterHelper.h"

LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
{
	strValue = strDefaultValue;
	WCHAR szBuffer[512];
	DWORD dwBufferSize = sizeof(szBuffer);
	ULONG nError;
	nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
	if (ERROR_SUCCESS == nError)
	{
		strValue = szBuffer;
	}
	return nError;
}

bool GetStringRegKey(
	const int id, 
	const std::wstring& strValueName, 
	std::wstring&		strValue, 
	const std::wstring& strStrDefaultValue)
{
	HKEY hKey;
	const char* path = "";

	switch (id)
	{
	case 2:
		path = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\notepad++.exe";
		break;
	}

	LONG lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey);
	bool bExistsAndSuccess(lRes == ERROR_SUCCESS);
	bool bDoesNotExistsSpecifically(lRes == ERROR_FILE_NOT_FOUND);

	if (bExistsAndSuccess && !bDoesNotExistsSpecifically)
	{
		GetStringRegKey(hKey, strValueName, strValue, strStrDefaultValue);
		RegCloseKey(hKey);
	}
	

	return bExistsAndSuccess;
}

bool GetRenderDocLoc(std::wstring& strValue)
{
	HKEY hKey;
	const char* path = "SOFTWARE\\Classes\\CLSID\\{5D6BF029-A6BA-417A-8523-120492B1DCE3}\\InprocServer32";

	const std::wstring		strValueName = L"";
	const std::wstring		strStrDefaultValue = L"";

	LONG lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey);
	bool bExistsAndSuccess(lRes == ERROR_SUCCESS);
	bool bDoesNotExistsSpecifically(lRes == ERROR_FILE_NOT_FOUND);

	if (bExistsAndSuccess && !bDoesNotExistsSpecifically)
	{
		GetStringRegKey(hKey, strValueName, strValue, strStrDefaultValue);
		RegCloseKey(hKey);

		return true;
	}

	return false;
}