/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <string_view>
#include <vector>

class CAppSettings final
{
public:
	CAppSettings() = default;
	CAppSettings(const CAppSettings&) = delete;
	CAppSettings(CAppSettings&&) = delete;
	void operator=(const CAppSettings&) = delete;
	~CAppSettings() = default;
	void LoadSettings(std::wstring_view wsvKeyName);
	void SaveSettings(std::wstring_view wsvKeyName);
	[[nodiscard]] bool GetShowPaneFileProps()const;
	[[nodiscard]] auto GetRFL() -> std::vector<std::wstring>&;
	void SetShowPaneFileProps(bool fShow);
private:
	std::vector<std::wstring> m_vecRFL;
	bool m_fShowPaneFileProps { }; //Show "File Properties" pane on the first file opened?
};