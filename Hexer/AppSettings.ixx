module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "resource.h"
#include "HexCtrl.h"
#include <afxwin.h>
#include <algorithm>
#include <cassert>
#include <bitset>
#include <format>
#include <ranges>
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
	void RemoveFromRFL(std::wstring_view wsvPath);
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

void CAppSettingsRFL::RemoveFromRFL(std::wstring_view wsvPath)
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	std::erase(m_vecRFL, wsvPath);
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

export class CAppSettings final {
public:
	CAppSettings() = default;
	CAppSettings(const CAppSettings&) = delete;
	CAppSettings(CAppSettings&&) = delete;
	void operator=(const CAppSettings&) = delete;
	~CAppSettings() = default;

	struct PANESTATUS {
		bool fIsVisible : 1{};
		bool fIsActive : 1{};
	};
	struct HEXCTRLDATA {
		LOGFONTW stLogFont { .lfHeight { -MulDiv(11, Ut::GetHiDPIInfo().iLOGPIXELSY, 72) },
			.lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } }; //HexCtrl default font.
		HEXCTRL::HEXCOLORS stClrs; //HexCtrl default colors.
		DWORD dwCapacity { 16UL };
		DWORD dwDateFormat { 0xFFFFFFFFUL };
		DWORD dwGroupSize { 1UL };
		DWORD dwPageSize { 0U };
		DWORD dwCharsExtraSpace { 0UL };
		float flScrollRatio { 3.0F };
		wchar_t wchUnprintable { L'.' }; //Replacement char for unprintable characters.
		wchar_t wchDateSepar { L'/' };   //Date separator.
		bool fOffsetHex { true };
		bool fScrollLines { true };
		bool fInfoBar { true };
	};

	[[nodiscard]] auto GetHexCtrlData() -> HEXCTRLDATA&;
	[[nodiscard]] auto GetPaneData(UINT uPaneID)const->std::uint64_t;
	[[nodiscard]] auto GetPaneStatus(UINT uPaneID)const->PANESTATUS;
	void LoadSettings(std::wstring_view wsvKeyName);
	void RFLAddToList(std::wstring_view wsvPath, bool fBeginning = true);
	void RFLClear();
	[[nodiscard]] auto RFLGetPathFromID(UINT uID)const->std::wstring;
	void RFLInitialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPDisk, int iMaxEntry = 20);
	void RFLRemoveFromList(std::wstring_view wsvPath);
	void SetPaneData(UINT uPaneID, std::uint64_t ullData);
	void SetPaneStatus(UINT uPaneID, bool fShow, bool fActive);
	void SaveSettings(std::wstring_view wsvKeyName);
	[[nodiscard]] static constexpr auto PaneStatus2DWORD(PANESTATUS ps) -> DWORD;
	[[nodiscard]] static constexpr auto DWORD2PaneStatus(DWORD dw) -> PANESTATUS;
private:
	[[nodiscard]] auto RFLGetData()const->const std::vector<std::wstring>&;
private:
	CAppSettingsRFL m_stRFL;
	std::uint64_t m_ullPaneDataFileInfo { };   //Pane data for the "File Info".
	std::uint64_t m_ullPaneDataBkmMgr { };     //Pane data for the "Bokmark Manager".
	std::uint64_t m_ullPaneDataDataInterp { }; //Pane data for the "Template Manager".
	std::uint64_t m_ullPaneDataTemplMgr { };   //Pane data for the "Data Interpreter".
	PANESTATUS m_stPSFileInfo { };             //Pane status for the "File Properties".
	PANESTATUS m_stPSBkmMgr { };               //Pane status for the "Bokmark Manager".
	PANESTATUS m_stPSDataInterp { };           //Pane status for the "Data Interpreter".
	PANESTATUS m_stPSTemplMgr { };             //Pane status for the "Template Manager".
	PANESTATUS m_stPSLogInfo { };              //Pane status for the "Log Information".
	HEXCTRLDATA m_stHexCtrlData;               //HexCtrl settings data.
};


auto CAppSettings::GetHexCtrlData()->HEXCTRLDATA&
{
	return m_stHexCtrlData;
}

auto CAppSettings::GetPaneData(UINT uPaneID)const->std::uint64_t
{
	switch (uPaneID) {
	case IDC_PANE_FILEINFO:
		return m_ullPaneDataFileInfo;
	case IDC_PANE_BKMMGR:
		return m_ullPaneDataBkmMgr;
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
	case IDC_PANE_FILEINFO:
		return m_stPSFileInfo;
	case IDC_PANE_BKMMGR:
		return m_stPSBkmMgr;
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
		DWORD dwPaneStatusFileInfo { };
		regSettings.QueryDWORDValue(L"PaneStatusFileInfo", dwPaneStatusFileInfo);
		m_stPSFileInfo = DWORD2PaneStatus(dwPaneStatusFileInfo);
		DWORD dwPaneStatusBkmMgr { };
		regSettings.QueryDWORDValue(L"PaneStatusBkmMgr", dwPaneStatusBkmMgr);
		m_stPSBkmMgr = DWORD2PaneStatus(dwPaneStatusBkmMgr);
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
		QWORD ullPaneDataBkmMgr { };
		regSettings.QueryQWORDValue(L"PaneDataBkmMgr", ullPaneDataBkmMgr);
		SetPaneData(IDC_PANE_BKMMGR, ullPaneDataBkmMgr | HEXCTRL::HEXCTRL_FLAG_BKMMGR_NOESC);
		QWORD ullPaneDataDataInterp { };
		regSettings.QueryQWORDValue(L"PaneDataDataInterp", ullPaneDataDataInterp);
		SetPaneData(IDC_PANE_DATAINTERP, ullPaneDataDataInterp | HEXCTRL::HEXCTRL_FLAG_DATAINTERP_NOESC);
		QWORD ullPaneDataTemplMgr { };
		regSettings.QueryQWORDValue(L"PaneDataTemplMgr", ullPaneDataTemplMgr);
		SetPaneData(IDC_PANE_TEMPLMGR, ullPaneDataTemplMgr | HEXCTRL::HEXCTRL_FLAG_TEMPLMGR_NOESC);

		//HexCtrl settings.
		const std::wstring wstrKeyHexCtrl = wstrKeySettings + L"\\HexCtrl";
		if (CRegKey regHexCtrl; regHexCtrl.Open(HKEY_CURRENT_USER, wstrKeyHexCtrl.data()) == ERROR_SUCCESS) {

			//HexCtrl data.
			auto& refData = GetHexCtrlData();
			regHexCtrl.QueryDWORDValue(L"HexCtrlCapacity", refData.dwCapacity);
			regHexCtrl.QueryDWORDValue(L"HexCtrlGroupSize", refData.dwGroupSize);
			regHexCtrl.QueryDWORDValue(L"HexCtrlPageSize", refData.dwPageSize);
			DWORD dwUnprintable { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlUnprintable", dwUnprintable);
			DWORD dwDateSepar { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlDateSepar", dwDateSepar);
			refData.wchDateSepar = static_cast<wchar_t>(dwDateSepar);
			regHexCtrl.QueryDWORDValue(L"HexCtrlDateFormat", refData.dwDateFormat);
			DWORD dwScrollLines { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlIsScrollLines", dwScrollLines);
			refData.fScrollLines = dwScrollLines;
			DWORD dwScrollRatio { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlScrollSize", dwScrollRatio);
			refData.flScrollRatio = std::bit_cast<float>(dwScrollRatio);
			DWORD dwInfoBar { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlIsInfoBar", dwInfoBar);
			refData.fInfoBar = dwInfoBar;
			DWORD dwOffsetHex { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlIsOffsetHex", dwOffsetHex);
			refData.fOffsetHex = dwOffsetHex;
			regHexCtrl.QueryDWORDValue(L"HexCtrlCharsExtraSpace", refData.dwCharsExtraSpace);

			//HexCtrl font.
			auto& lf = GetHexCtrlData().stLogFont;
			DWORD dwChars { 32UL };
			regHexCtrl.QueryStringValue(L"HexCtrlFontFace", lf.lfFaceName, &dwChars);
			DWORD dwHeight { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlFontHeight", dwHeight);
			lf.lfHeight = dwHeight;
			DWORD dwWidth { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlFontWidth", dwWidth);
			lf.lfWidth = dwWidth;
			DWORD dwWeight { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlFontWeight", dwWeight);
			lf.lfWeight = dwWeight;
			DWORD dwItalic { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlFontItalic", dwItalic);
			lf.lfItalic = static_cast<BYTE>(dwItalic);
			DWORD dwUnderline { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlFontUnderline", dwUnderline);
			lf.lfUnderline = static_cast<BYTE>(dwUnderline);
			DWORD dwStrikeOut { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlFontStrikeout", dwStrikeOut);
			lf.lfStrikeOut = static_cast<BYTE>(dwStrikeOut);
			DWORD dwCharSet { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlFontCharSet", dwCharSet);
			lf.lfCharSet = static_cast<BYTE>(dwCharSet);
			DWORD dwPitchAndFamily { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlFontPitchAndFamily", dwPitchAndFamily);
			lf.lfPitchAndFamily = static_cast<BYTE>(dwPitchAndFamily);

			//HexCtrl colors.
			auto& refClrs = GetHexCtrlData().stClrs;
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontHex", refClrs.clrFontHex);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontText", refClrs.clrFontText);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontSel", refClrs.clrFontSel);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontDataInterp", refClrs.clrFontDataInterp);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontCaption", refClrs.clrFontCaption);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontInfoParam", refClrs.clrFontInfoParam);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontInfoData", refClrs.clrFontInfoData);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontCaret", refClrs.clrFontCaret);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBk", refClrs.clrBk);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkSel", refClrs.clrBkSel);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkDataInterp", refClrs.clrBkDataInterp);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkInfoBar", refClrs.clrBkInfoBar);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkCaret", refClrs.clrBkCaret);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkCaretSel", refClrs.clrBkCaretSel);
		}
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

void CAppSettings::RFLRemoveFromList(std::wstring_view wsvPath)
{
	m_stRFL.RemoveFromRFL(wsvPath);
}

void CAppSettings::SaveSettings(std::wstring_view wsvKeyName)
{
	const std::wstring wstrAppKey = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName };

	//Recent File List.
	CRegKey regRFL;
	regRFL.Open(HKEY_CURRENT_USER, wstrAppKey.data());
	regRFL.RecurseDeleteKey(L"Recent File List"); //Remove all data to set it below.
	const std::wstring wstrKeyRFL = wstrAppKey + L"\\Recent File List";
	regRFL.Create(HKEY_CURRENT_USER, wstrKeyRFL.data());
	for (const auto [idx, wstr] : RFLGetData() | std::views::enumerate) {
		regRFL.SetStringValue(std::format(L"File{:02d}", idx).data(), wstr.data());
	}

	//Settings.
	CRegKey regSettings;
	const std::wstring wstrKeySettings = wstrAppKey + L"\\Settings";
	if (regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) != ERROR_SUCCESS) {
		regSettings.Create(HKEY_CURRENT_USER, wstrKeySettings.data());
	}

	//PaneStatus.
	regSettings.SetDWORDValue(L"PaneStatusFileInfo", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_FILEINFO)));
	regSettings.SetDWORDValue(L"PaneStatusBkmMgr", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_BKMMGR)));
	regSettings.SetDWORDValue(L"PaneStatusDataInterp", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_DATAINTERP)));
	regSettings.SetDWORDValue(L"PaneStatusTemplMgr", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_TEMPLMGR)));
	regSettings.SetDWORDValue(L"PaneStatusLogInfo", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_LOGINFO)));

	//SetPaneData
	regSettings.SetQWORDValue(L"PaneDataBkmMgr", GetPaneData(IDC_PANE_BKMMGR));
	regSettings.SetQWORDValue(L"PaneDataDataInterp", GetPaneData(IDC_PANE_DATAINTERP));
	regSettings.SetQWORDValue(L"PaneDataTemplMgr", GetPaneData(IDC_PANE_TEMPLMGR));

	//HexCtrl settings.
	CRegKey regHexCtrl;
	const std::wstring wstrKeyHexCtrl = wstrKeySettings + L"\\HexCtrl";
	if (regHexCtrl.Open(HKEY_CURRENT_USER, wstrKeyHexCtrl.data()) != ERROR_SUCCESS) {
		regHexCtrl.Create(HKEY_CURRENT_USER, wstrKeyHexCtrl.data());
	}

	//HexCtrl data.
	const auto& refData = GetHexCtrlData();
	regHexCtrl.SetDWORDValue(L"HexCtrlCapacity", refData.dwCapacity);
	regHexCtrl.SetDWORDValue(L"HexCtrlGroupSize", refData.dwGroupSize);
	regHexCtrl.SetDWORDValue(L"HexCtrlPageSize", refData.dwPageSize);
	regHexCtrl.SetDWORDValue(L"HexCtrlUnprintable", refData.wchUnprintable);
	regHexCtrl.SetDWORDValue(L"HexCtrlDateFormat", refData.dwDateFormat);
	regHexCtrl.SetDWORDValue(L"HexCtrlDateSepar", refData.wchDateSepar);
	regHexCtrl.SetDWORDValue(L"HexCtrlIsScrollLines", refData.fScrollLines);
	regHexCtrl.SetDWORDValue(L"HexCtrlScrollSize", std::bit_cast<DWORD>(refData.flScrollRatio));
	regHexCtrl.SetDWORDValue(L"HexCtrlIsInfoBar", refData.fInfoBar);
	regHexCtrl.SetDWORDValue(L"HexCtrlIsOffsetHex", refData.fOffsetHex);
	regHexCtrl.SetDWORDValue(L"HexCtrlCharsExtraSpace", refData.dwCharsExtraSpace);

	//HexCtrl font.
	const auto& refLF = GetHexCtrlData().stLogFont;
	regHexCtrl.SetStringValue(L"HexCtrlFontFace", refLF.lfFaceName);
	regHexCtrl.SetDWORDValue(L"HexCtrlFontHeight", refLF.lfHeight);
	regHexCtrl.SetDWORDValue(L"HexCtrlFontWidth", refLF.lfWidth);
	regHexCtrl.SetDWORDValue(L"HexCtrlFontWeight", refLF.lfWeight);
	regHexCtrl.SetDWORDValue(L"HexCtrlFontItalic", refLF.lfItalic);
	regHexCtrl.SetDWORDValue(L"HexCtrlFontUnderline", refLF.lfUnderline);
	regHexCtrl.SetDWORDValue(L"HexCtrlFontStrikeout", refLF.lfStrikeOut);
	regHexCtrl.SetDWORDValue(L"HexCtrlFontCharSet", refLF.lfCharSet);
	regHexCtrl.SetDWORDValue(L"HexCtrlFontPitchAndFamily", refLF.lfPitchAndFamily);

	//HexCtrl colors.
	const auto& refClrs = GetHexCtrlData().stClrs;
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontHex", refClrs.clrFontHex);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontText", refClrs.clrFontText);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontSel", refClrs.clrFontSel);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontDataInterp", refClrs.clrFontDataInterp);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontCaption", refClrs.clrFontCaption);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontInfoParam", refClrs.clrFontInfoParam);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontInfoData", refClrs.clrFontInfoData);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontCaret", refClrs.clrFontCaret);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBk", refClrs.clrBk);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkSel", refClrs.clrBkSel);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkDataInterp", refClrs.clrBkDataInterp);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkInfoBar", refClrs.clrBkInfoBar);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkCaret", refClrs.clrBkCaret);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkCaretSel", refClrs.clrBkCaretSel);
}

void CAppSettings::SetPaneData(UINT uPaneID, std::uint64_t ullData)
{
	switch (uPaneID) {
	case IDC_PANE_FILEINFO:
		m_ullPaneDataFileInfo = ullData;
		break;
	case IDC_PANE_BKMMGR:
		m_ullPaneDataBkmMgr = ullData;
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
	case IDC_PANE_FILEINFO:
		m_stPSFileInfo = { fShow, fActive };
	case IDC_PANE_BKMMGR:
		m_stPSBkmMgr = { fShow, fActive };
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