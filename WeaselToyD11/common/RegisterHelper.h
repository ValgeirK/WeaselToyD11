#pragma once

#include <string>

bool GetStringRegKey(
	const int			id,
	const std::wstring& strValueName,
	std::wstring&		strValue,
	const std::wstring& strStrDefaultValue
);

bool GetRenderDocLoc(std::wstring& strValue);