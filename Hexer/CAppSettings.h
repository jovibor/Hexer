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
	struct PANESTATUS {
		bool fIsVisible : 1{};
		bool fIsActive : 1{};
	};
public:
	CAppSettings() = default;
	CAppSettings(const CAppSettings&) = delete;
	CAppSettings(CAppSettings&&) = delete;
	void operator=(const CAppSettings&) = delete;
	~CAppSettings() = default;

	void LoadSettings(std::wstring_view wsvKeyName);
	void SaveSettings(std::wstring_view wsvKeyName);
	[[nodiscard]] auto GetPaneData(UINT uPaneID)const->std::uint64_t;
	[[nodiscard]] auto GetPaneStatus(UINT uPaneID)const->PANESTATUS;
	[[nodiscard]] auto GetRFL() -> std::vector<std::wstring>&;
	void SetPaneData(UINT uPaneID, std::uint64_t ullData);
	void SetPaneStatus(UINT uPaneID, bool fShow, bool fActive);
	[[nodiscard]] static constexpr auto PaneStatus2DWORD(PANESTATUS ps) -> DWORD;
	[[nodiscard]] static constexpr auto DWORD2PaneStatus(DWORD dw) -> PANESTATUS;
private:
	std::vector<std::wstring> m_vecRFL;        //Recent File List vector.
	std::uint64_t m_ullPaneDataFileProps { };  //Pane data for the "File Properties".
	std::uint64_t m_ullPaneDataDataInterp { }; //Pane data for the "Template Manager".
	std::uint64_t m_ullPaneDataTemplMgr { };   //Pane data for the "Data Interpreter".
	PANESTATUS m_stPSFileProps { };            //Pane status for the "File Properties".
	PANESTATUS m_stPSDataInterp { };           //Pane status for the "Data Interpreter".
	PANESTATUS m_stPSTemplMgr { };             //Pane status for the "Template Manager".
};