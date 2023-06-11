/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxwin.h>
#include <vector>
#include <string>

class CHexerRFL final //Recent File List.
{
public:
	CHexerRFL() = default;
	void Initialize(HMENU hMenu, int iMenuFirstID, std::vector<std::wstring>* pVecData, int iMaxEntry = 20);
	void AddToRFL(std::wstring_view wsvPath);
	[[nodiscard]] auto GetPathFromRFL(UINT uID)const->std::wstring;
private:
	void RebuildRFLMenu();
private:
	std::vector<std::wstring>* m_pVecData { };
	HMENU m_hMenu { };
	int m_iMaxEntry { };
	int m_iIDMenuFirst { };
	bool m_fInit { };
};