module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "resource.h"
#include <afxwin.h>
#include <algorithm>
#include <cassert>
#include <bitset>
#include <format>
#include <string>
#include <vector>
export module AppSettings;

import Utility;


//CAppSettingsRFL.

class CAppSettingsRFL final //Recent File List.
{
public:
	CAppSettingsRFL() = default;
	void Initialize(HMENU hMenu, int iMenuFirstID, HBITMAP hBMPDisk, int iMaxEntry);
	void AddToRFL(std::wstring_view wsvPath, bool fBeginning);
	void ClearRFL();
	[[nodiscard]] auto GetPathFromRFL(UINT uID)const->std::wstring;
	[[nodiscard]] auto GetRFL()const->const std::vector<std::wstring>&;
private:
	void RebuildRFLMenu();
private:
	std::vector<std::wstring> m_vecRFL; //Recent File List data.
	HMENU m_hMenu { };
	HBITMAP m_hBMPDisk { }; //Bitmap for disk icon.
	int m_iMaxEntry { };
	int m_iIDMenuFirst { };
	bool m_fInit { };
};

void CAppSettingsRFL::Initialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPDisk, int iMaxEntry)
{
	assert(IsMenu(hMenu));
	if (!IsMenu(hMenu)) {
		return;
	}

	m_hMenu = hMenu;
	m_iIDMenuFirst = iIDMenuFirst;
	m_hBMPDisk = hBMPDisk;
	m_iMaxEntry = iMaxEntry;
	m_fInit = true;

	RebuildRFLMenu();
}

void CAppSettingsRFL::AddToRFL(std::wstring_view wsvPath, bool fBeginning)
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	std::erase(m_vecRFL, wsvPath); //Remove any duplicates.

	if (fBeginning) {
		m_vecRFL.emplace(m_vecRFL.begin(), wsvPath);
	}
	else {
		m_vecRFL.emplace_back(wsvPath);
	}

	if (m_vecRFL.size() > m_iMaxEntry) {
		m_vecRFL.resize(m_iMaxEntry);
	}

	RebuildRFLMenu();
}

void CAppSettingsRFL::ClearRFL()
{
	m_vecRFL.clear();
}

auto CAppSettingsRFL::GetPathFromRFL(UINT uID)const->std::wstring
{
	assert(m_fInit);
	if (!m_fInit)
		return { };

	const auto uIndex = uID - m_iIDMenuFirst;
	if (uIndex >= m_vecRFL.size())
		return { };

	return m_vecRFL.at(uIndex);
}

auto CAppSettingsRFL::GetRFL()const->const std::vector<std::wstring>&
{
	return m_vecRFL;
}


//CAppSettingsRFL Private methods.

void CAppSettingsRFL::RebuildRFLMenu()
{
	while (GetMenuItemCount(m_hMenu) > 0) {
		DeleteMenu(m_hMenu, 0, MF_BYPOSITION); //Removing all RFL menu items.
	}

	auto uIndex { 0 };
	for (const auto& wstr : m_vecRFL) {
		if (uIndex >= m_iMaxEntry) //Adding not more than m_iMaxEntry.
			break;

		const auto fDevice = wstr.starts_with(L"\\\\");
		const auto wstrMenu = std::vformat(fDevice ? L"{} Device: {}" : L"{} {}",
			std::make_wformat_args(uIndex + 1, wstr));
		const auto iMenuID = m_iIDMenuFirst + uIndex;
		AppendMenuW(m_hMenu, MF_STRING, iMenuID, wstrMenu.data());

		if (fDevice) {
			MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { m_hBMPDisk } };
			SetMenuItemInfoW(m_hMenu, iMenuID, FALSE, &mii);
		}

		++uIndex;
	}
}


//CAppSettings.

export class CAppSettings final
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

	[[nodiscard]] auto GetPaneData(UINT uPaneID)const->std::uint64_t;
	[[nodiscard]] auto GetPaneStatus(UINT uPaneID)const->PANESTATUS;
	void LoadSettings(std::wstring_view wsvKeyName);
	void RFLAddToList(std::wstring_view wsvPath, bool fBeginning = true);
	void RFLClear();
	[[nodiscard]] auto RFLGetPathFromID(UINT uID)const->std::wstring;
	void RFLInitialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPDisk, int iMaxEntry = 20);
	void SaveSettings(std::wstring_view wsvKeyName);
	void SetPaneData(UINT uPaneID, std::uint64_t ullData);
	void SetPaneStatus(UINT uPaneID, bool fShow, bool fActive);
	[[nodiscard]] static constexpr auto PaneStatus2DWORD(PANESTATUS ps) -> DWORD;
	[[nodiscard]] static constexpr auto DWORD2PaneStatus(DWORD dw) -> PANESTATUS;
private:
	[[nodiscard]] auto RFLGetData()const->const std::vector<std::wstring>&;
private:
	CAppSettingsRFL m_stRFL;
	std::uint64_t m_ullPaneDataFileProps { };  //Pane data for the "File Properties".
	std::uint64_t m_ullPaneDataDataInterp { }; //Pane data for the "Template Manager".
	std::uint64_t m_ullPaneDataTemplMgr { };   //Pane data for the "Data Interpreter".
	PANESTATUS m_stPSFileProps { };            //Pane status for the "File Properties".
	PANESTATUS m_stPSDataInterp { };           //Pane status for the "Data Interpreter".
	PANESTATUS m_stPSTemplMgr { };             //Pane status for the "Template Manager".
	PANESTATUS m_stPSLogInfo { };              //Pane status for the "Log Information".
};

auto CAppSettings::GetPaneData(UINT uPaneID)const->std::uint64_t
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		return m_ullPaneDataFileProps;
	case IDC_PANE_DATAINTERP:
		return m_ullPaneDataDataInterp;
	case IDC_PANE_TEMPLMGR:
		return m_ullPaneDataTemplMgr;
	default:
		return { };
	}
}

auto CAppSettings::GetPaneStatus(UINT uPaneID)const->PANESTATUS
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		return m_stPSFileProps;
	case IDC_PANE_DATAINTERP:
		return m_stPSDataInterp;
	case IDC_PANE_TEMPLMGR:
		return m_stPSTemplMgr;
	case IDC_PANE_LOGINFO:
		return m_stPSLogInfo;
	default:
		return { };
	}
}

void CAppSettings::LoadSettings(std::wstring_view wsvKeyName)
{
	//Settings.
	const std::wstring wstrKeySettings = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Settings";
	if (CRegKey regSettings; regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) == ERROR_SUCCESS) {

		//PaneStatus.
		DWORD dwPaneStatusFileProps { };
		regSettings.QueryDWORDValue(L"PaneStatusFileProps", dwPaneStatusFileProps);
		m_stPSFileProps = DWORD2PaneStatus(dwPaneStatusFileProps);
		DWORD dwPaneStatusDataInterp { };
		regSettings.QueryDWORDValue(L"PaneStatusDataInterp", dwPaneStatusDataInterp);
		m_stPSDataInterp = DWORD2PaneStatus(dwPaneStatusDataInterp);
		DWORD dwPaneStatusTemplMgr { };
		regSettings.QueryDWORDValue(L"PaneStatusTemplMgr", dwPaneStatusTemplMgr);
		m_stPSTemplMgr = DWORD2PaneStatus(dwPaneStatusTemplMgr);
		DWORD dwPaneStatusLogInfo { };
		regSettings.QueryDWORDValue(L"PaneStatusLogInfo", dwPaneStatusLogInfo);
		m_stPSLogInfo = DWORD2PaneStatus(dwPaneStatusLogInfo);

		//PaneData.
		QWORD ullPaneDataTemplMgr { };
		regSettings.QueryQWORDValue(L"PaneDataTemplMgr", ullPaneDataTemplMgr);
		SetPaneData(IDC_PANE_TEMPLMGR, ullPaneDataTemplMgr);
		QWORD ullPaneDataDataInterp { };
		regSettings.QueryQWORDValue(L"PaneDataDataInterp", ullPaneDataDataInterp);
		SetPaneData(IDC_PANE_DATAINTERP, ullPaneDataDataInterp);
	}

	//Recent File List.
	const std::wstring wstrKeyRFL = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Recent File List";
	if (CRegKey regRFL; regRFL.Open(HKEY_CURRENT_USER, wstrKeyRFL.data()) == ERROR_SUCCESS) {
		int iCode { };
		wchar_t buffName[32];
		wchar_t buffData[MAX_PATH];
		DWORD dwIndex = 0;
		RFLClear();
		while (iCode != ERROR_NO_MORE_ITEMS) {
			DWORD dwNameSize { sizeof(buffName) / sizeof(wchar_t) };
			DWORD dwDataType { };
			DWORD dwDataSize { MAX_PATH * sizeof(wchar_t) };
			iCode = RegEnumValueW(regRFL, dwIndex, buffName, &dwNameSize, nullptr, &dwDataType,
				reinterpret_cast<LPBYTE>(&buffData), &dwDataSize);
			if (iCode == ERROR_SUCCESS && dwDataType == REG_SZ) {
				RFLAddToList(buffData, false);
			}
			++dwIndex;
		}
	}
}

void CAppSettings::RFLAddToList(std::wstring_view wsvPath, bool fBeginning)
{
	m_stRFL.AddToRFL(wsvPath, fBeginning);
}

void CAppSettings::RFLClear()
{
	m_stRFL.ClearRFL();
}

auto CAppSettings::RFLGetPathFromID(UINT uID)const->std::wstring
{
	return m_stRFL.GetPathFromRFL(uID);
}

void CAppSettings::RFLInitialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPDisk, int iMaxEntry)
{
	m_stRFL.Initialize(hMenu, iIDMenuFirst, hBMPDisk, iMaxEntry);
}

void CAppSettings::SaveSettings(std::wstring_view wsvKeyName)
{
	const std::wstring wstrAppKey = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName };

	//Settings.
	CRegKey regSettings;
	const std::wstring wstrKeySettings = wstrAppKey + L"\\Settings";
	if (regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) != ERROR_SUCCESS) {
		regSettings.Create(HKEY_CURRENT_USER, wstrKeySettings.data());
	}

	//PaneStatus.
	regSettings.SetDWORDValue(L"PaneStatusFileProps", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_FILEPROPS)));
	regSettings.SetDWORDValue(L"PaneStatusDataInterp", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_DATAINTERP)));
	regSettings.SetDWORDValue(L"PaneStatusTemplMgr", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_TEMPLMGR)));
	regSettings.SetDWORDValue(L"PaneStatusLogInfo", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_LOGINFO)));

	//SetPaneData
	regSettings.SetQWORDValue(L"PaneDataDataInterp", GetPaneData(IDC_PANE_DATAINTERP));
	regSettings.SetQWORDValue(L"PaneDataTemplMgr", GetPaneData(IDC_PANE_TEMPLMGR));

	//Recent File List.
	CRegKey regRFL;
	regRFL.Open(HKEY_CURRENT_USER, wstrAppKey.data());
	regRFL.RecurseDeleteKey(L"Recent File List"); //Remove all data to set it below.
	const std::wstring wstrKeyRFL = wstrAppKey + L"\\Recent File List";
	regRFL.Create(HKEY_CURRENT_USER, wstrKeyRFL.data());
	int iIndex { 1 };
	for (const auto& wstr : RFLGetData()) {
		regRFL.SetStringValue(std::format(L"File{:02d}", iIndex++).data(), wstr.data());
	}
}

void CAppSettings::SetPaneData(UINT uPaneID, std::uint64_t ullData)
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		m_ullPaneDataFileProps = ullData;
		break;
	case IDC_PANE_DATAINTERP:
		m_ullPaneDataDataInterp = ullData;
		break;
	case IDC_PANE_TEMPLMGR:
		m_ullPaneDataTemplMgr = ullData;
		break;
	default:
		return;
	}
}

void CAppSettings::SetPaneStatus(UINT uPaneID, bool fShow, bool fActive)
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		m_stPSFileProps = { fShow, fActive };
	case IDC_PANE_DATAINTERP:
		m_stPSDataInterp = { fShow, fActive };
	case IDC_PANE_TEMPLMGR:
		m_stPSTemplMgr = { fShow, fActive };
	case IDC_PANE_LOGINFO:
		m_stPSLogInfo = { fShow, fActive };
	default:
		return;
	}
}


//CAppSettings Private methods.

auto CAppSettings::RFLGetData()const->const std::vector<std::wstring>&
{
	return m_stRFL.GetRFL();
}

constexpr auto CAppSettings::PaneStatus2DWORD(PANESTATUS ps)->DWORD
{
	std::bitset<32> bs;
	bs[0] = ps.fIsVisible;
	bs[1] = ps.fIsActive;

	return bs.to_ulong();
}

constexpr auto CAppSettings::DWORD2PaneStatus(DWORD dw)->PANESTATUS
{
	const std::bitset<32> bsPS(dw);
	return { bsPS.test(0), bsPS.test(1) };
}