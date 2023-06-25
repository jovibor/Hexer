/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <string>
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
	[[nodiscard]] auto GetPaneData(UINT uPaneID)const->std::uint64_t;
	[[nodiscard]] bool GetPaneActive(UINT uPaneID)const;
	[[nodiscard]] bool GetShowPane(UINT uPaneID)const;
	[[nodiscard]] auto GetRFL() -> std::vector<std::wstring>&;
	void SetPaneData(UINT uPaneID, std::uint64_t ullData);
	void SetPaneActive(UINT uPaneID, bool fActive);
	void SetShowPane(UINT uPaneID, bool fShow);
private:
	std::vector<std::wstring> m_vecRFL;
	std::uint64_t m_ullPaneDataFileProps { };
	std::uint64_t m_ullPaneDataTemplMgr { };
	std::uint64_t m_ullPaneDataDataInterp { };
	bool m_fShowPaneFileProps { };    //Show "File Properties" pane on the first frame opened?
	bool m_fShowPaneDataInterp { };   //Show "Data Interpreter" pane on the first frame opened?
	bool m_fShowPaneTemplMgr { };     //Show "Template Manager" pane on the first frame opened?
	bool m_fPaneActiveFileProps { };  //Is Pane active "File Properties"?
	bool m_fPaneActiveDataInterp { }; //Is Pane active "Data Interpreter"?
	bool m_fPaneActiveTemplMgr { };   //Is Pane active "Template Manager"?
};