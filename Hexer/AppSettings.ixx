module;
/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
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
#include <filesystem>
#include <format>
#include <ranges>
#include <string>
#include <utility>
#include <vector>
export module AppSettings;

import Utility;


//CAppSettingsRFL.
class CAppSettingsRFL final //Recent Files List.
{
public:
	struct RFLDATA {
		std::wstring wstrDataPath;
		Ut::EOpenMode eMode { Ut::EOpenMode::OPEN_FILE };
	};
	CAppSettingsRFL() = default;
	void Initialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPDisk, int iMaxEntry);
	void AddToRFL(std::wstring_view wsvPath, bool fBeginning);
	void RemoveFromRFL(std::wstring_view wsvPath);
	void ClearRFL();
	[[nodiscard]] auto GetDataFromMenuID(UINT uID)const->RFLDATA;
	[[nodiscard]] auto GetRFL()const->const std::vector<RFLDATA>&;
private:
	void RebuildRFLMenu();
private:
	std::vector<RFLDATA> m_vecRFL; //Recent Files List data.
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
	m_iMaxEntry = std::clamp(iMaxEntry, 0, 20);
	m_fInit = true;

	RebuildRFLMenu();
}

void CAppSettingsRFL::AddToRFL(std::wstring_view wsvPath, bool fBeginning)
{
	//Remove any duplicates.
	std::erase_if(m_vecRFL, [wsvPath](const RFLDATA& refData) { return refData.wstrDataPath == wsvPath; });

	using enum Ut::EOpenMode;
	const auto eMode = wsvPath.starts_with(L"\\\\") ? OPEN_DEVICE : OPEN_FILE;

	if (fBeginning) {
		m_vecRFL.emplace(m_vecRFL.begin(), std::wstring { wsvPath }, eMode);
	}
	else {
		m_vecRFL.emplace_back(std::wstring { wsvPath }, eMode);
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

	std::erase_if(m_vecRFL, [wsvPath](const RFLDATA& refData) { return refData.wstrDataPath == wsvPath; });
	RebuildRFLMenu();
}

void CAppSettingsRFL::ClearRFL()
{
	m_vecRFL.clear();
}

auto CAppSettingsRFL::GetDataFromMenuID(UINT uID)const->RFLDATA
{
	assert(m_fInit);
	if (!m_fInit)
		return { };

	const auto uIndex = uID - m_iIDMenuFirst;
	if (uIndex >= m_vecRFL.size())
		return { };

	return m_vecRFL[uIndex];
}

auto CAppSettingsRFL::GetRFL()const->const std::vector<RFLDATA>&
{
	return m_vecRFL;
}


//CAppSettingsRFL private methods.

void CAppSettingsRFL::RebuildRFLMenu()
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	while (GetMenuItemCount(m_hMenu) > 0) {
		DeleteMenu(m_hMenu, 0, MF_BYPOSITION); //Removing all RFL menu items.
	}

	auto iIndex { 0 };
	for (const auto& refData : m_vecRFL) {
		if (iIndex >= m_iMaxEntry) //Adding not more than m_iMaxEntry.
			break;

		const auto fDevice = refData.eMode == Ut::EOpenMode::OPEN_DEVICE;
		const auto uIndexMenu = iIndex + 1;
		const auto wstrMenu = std::vformat(fDevice ? L"{} Device: {}" : L"{} {}",
			std::make_wformat_args(uIndexMenu, refData.wstrDataPath));
		const auto iMenuID = m_iIDMenuFirst + iIndex;
		AppendMenuW(m_hMenu, MF_STRING, iMenuID, wstrMenu.data());

		if (fDevice) {
			MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { m_hBMPDisk } };
			SetMenuItemInfoW(m_hMenu, iMenuID, FALSE, &mii);
		}

		++iIndex;
	}
}


//CAppSettings.
export class CAppSettings final {
public:
	using VecTemplates = std::vector<std::unique_ptr<HEXCTRL::HEXTEMPLATE>>;
	struct PANESTATUS {
		bool fIsVisible : 1{};
		bool fIsActive : 1{};
	};
	struct PANESETTINGS {
		std::uint64_t ullPaneDataFileInfo { };   //Pane data for the "File Info".
		std::uint64_t ullPaneDataBkmMgr { };     //Pane data for the "Bokmark Manager".
		std::uint64_t ullPaneDataDataInterp { }; //Pane data for the "Template Manager".
		std::uint64_t ullPaneDataTemplMgr { };   //Pane data for the "Data Interpreter".
		PANESTATUS stPSFileInfo { };             //Pane status for the "File Properties".
		PANESTATUS stPSBkmMgr { };               //Pane status for the "Bokmark Manager".
		PANESTATUS stPSDataInterp { };           //Pane status for the "Data Interpreter".
		PANESTATUS stPSTemplMgr { };             //Pane status for the "Template Manager".
		PANESTATUS stPSLogInfo { };              //Pane status for the "Log Information".
	};
	enum class EStartup :std::uint8_t {
		DO_NOTHING, RESTORE_LAST_OPENED, SHOW_FOD
	};
	struct GENERALSETTINGS {
		bool fMultipleInst { }; //0-Single, 1-Multiple.
		DWORD dwRFLSize { };
		EStartup eStartup { };
		bool fWindowsMenu { }; //1-Show, 0-Don't show.
	};
	struct HEXCTRLSETTINGS {
		LOGFONTW stLogFont { };
		HEXCTRL::HEXCOLORS stClrs;
		DWORD dwCapacity { };
		DWORD dwDateFormat { };
		DWORD dwGroupSize { };
		DWORD dwPageSize { };
		DWORD dwCharsExtraSpace { };
		float flScrollRatio { };
		wchar_t wchUnprintable { }; //Replacement char for unprintable characters.
		wchar_t wchDateSepar { };   //Date separator.
		bool fOffsetHex { };
		bool fScrollLines { };
		bool fInfoBar { };
	};
	CAppSettings() = default;
	CAppSettings(const CAppSettings&) = delete;
	CAppSettings(CAppSettings&&) = delete;
	CAppSettings& operator=(const CAppSettings&) = delete;
	~CAppSettings() = default;
	void AddToLastOpened(std::wstring_view wsvPath);
	[[nodiscard]] auto GetGeneralSettings() -> GENERALSETTINGS&;
	[[nodiscard]] auto GetHexCtrlSettings() -> HEXCTRLSETTINGS&;
	[[nodiscard]] auto GetHexCtrlTemplates()const->const VecTemplates&;
	[[nodiscard]] auto GetLastOpenedFromReg()const->std::vector<std::wstring>;
	[[nodiscard]] auto GetPaneData(UINT uPaneID)const->std::uint64_t;
	[[nodiscard]] auto GetPaneStatus(UINT uPaneID)const->PANESTATUS;
	void LoadSettings(std::wstring_view wsvKeyName);
	void OnSettingsChanged();
	void RemoveFromLastOpened(std::wstring_view wsvPath);
	void RFLAddToList(std::wstring_view wsvPath, bool fBeginning = true);
	void RFLClear();
	[[nodiscard]] auto RFLGetDataFromMenuID(UINT uID)const->CAppSettingsRFL::RFLDATA;
	void RFLInitialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPDisk, int iMaxEntry = 20);
	void RFLRemoveFromList(std::wstring_view wsvPath);
	void SaveSettings(std::wstring_view wsvKeyName);
	void SetPaneData(UINT uPaneID, std::uint64_t ullData);
	void SetPaneStatus(UINT uPaneID, bool fShow, bool fActive);
	[[nodiscard]] static auto GetGeneralDefs() -> const GENERALSETTINGS&;
	[[nodiscard]] static auto GetHexCtrlDefs() -> const HEXCTRLSETTINGS&;
private:
	[[nodiscard]] auto GetPanesSettings() -> PANESETTINGS&;
	[[nodiscard]] auto GetPanesSettings()const->const PANESETTINGS&;
	void LoadHexCtrlTemplates();
	[[nodiscard]] auto RFLGetData()const->const std::vector<CAppSettingsRFL::RFLDATA>&;
	void ShowInWindowsContextMenu(bool fShow);
	[[nodiscard]] static auto DWORD2PaneStatus(DWORD dw) -> PANESTATUS;
	[[nodiscard]] static auto PaneStatus2DWORD(PANESTATUS ps) -> DWORD;
private:
	CAppSettingsRFL m_stRFL;
	PANESETTINGS m_stPaneSettings;   //"Panes" settings data.
	GENERALSETTINGS m_stGeneralData; //"General" settings data.
	HEXCTRLSETTINGS m_stHexCtrlData; //"HexCtrl" settings data.
	std::wstring m_wstrKeyName;      //Registry Key name.
	std::vector<std::wstring> m_vecLastOpened; //Last Opened files list.
	VecTemplates m_vecHexCtrlTemplates; //HexCtrl loaded templates.
	bool m_fLoaded { false };        //LoadSettings has succeeded.
};


void CAppSettings::AddToLastOpened(std::wstring_view wsvPath)
{
	if (m_vecLastOpened.size() < 20) {
		m_vecLastOpened.emplace_back(wsvPath);
	}
}

auto CAppSettings::GetGeneralSettings()->GENERALSETTINGS&
{
	return m_stGeneralData;
}

auto CAppSettings::GetHexCtrlSettings()->HEXCTRLSETTINGS&
{
	return m_stHexCtrlData;
}

auto CAppSettings::GetHexCtrlTemplates()const->const VecTemplates&
{
	return m_vecHexCtrlTemplates;
}

auto CAppSettings::GetLastOpenedFromReg()const->std::vector<std::wstring>
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	//Last Opened List.
	std::vector<std::wstring> vec;
	const std::wstring wstrKeyRFL = std::wstring { L"SOFTWARE\\" } + m_wstrKeyName + L"\\Last Opened List";
	if (CRegKey regRFL; regRFL.Open(HKEY_CURRENT_USER, wstrKeyRFL.data()) == ERROR_SUCCESS) {
		int iCode { };
		wchar_t buffName[32];
		wchar_t buffData[MAX_PATH];
		DWORD dwIndex = 0;
		while (iCode != ERROR_NO_MORE_ITEMS && dwIndex < 20) { //Maximum 20 files.
			DWORD dwNameSize { sizeof(buffName) / sizeof(wchar_t) };
			DWORD dwDataType { };
			DWORD dwDataSize { MAX_PATH * sizeof(wchar_t) };
			iCode = RegEnumValueW(regRFL, dwIndex, buffName, &dwNameSize, nullptr, &dwDataType,
				reinterpret_cast<LPBYTE>(&buffData), &dwDataSize);
			if (iCode == ERROR_SUCCESS && dwDataType == REG_SZ) {
				vec.emplace_back(buffData);
			}
			++dwIndex;
		}
	}

	return vec;
}

auto CAppSettings::GetPaneData(UINT uPaneID)const->std::uint64_t
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	const auto& refPanes = GetPanesSettings();
	switch (uPaneID) {
	case IDC_PANE_DATAINFO:
		return refPanes.ullPaneDataFileInfo;
	case IDC_PANE_BKMMGR:
		return refPanes.ullPaneDataBkmMgr;
	case IDC_PANE_DATAINTERP:
		return refPanes.ullPaneDataDataInterp;
	case IDC_PANE_TEMPLMGR:
		return refPanes.ullPaneDataTemplMgr;
	default:
		return { };
	}
}

auto CAppSettings::GetPaneStatus(UINT uPaneID)const->PANESTATUS
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	const auto& refPanes = GetPanesSettings();
	switch (uPaneID) {
	case IDC_PANE_DATAINFO:
		return refPanes.stPSFileInfo;
	case IDC_PANE_BKMMGR:
		return refPanes.stPSBkmMgr;
	case IDC_PANE_DATAINTERP:
		return refPanes.stPSDataInterp;
	case IDC_PANE_TEMPLMGR:
		return refPanes.stPSTemplMgr;
	case IDC_PANE_LOGGER:
		return refPanes.stPSLogInfo;
	default:
		return { };
	}
}

void CAppSettings::LoadSettings(std::wstring_view wsvKeyName)
{
	m_wstrKeyName = wsvKeyName;

	//Settings.
	const std::wstring wstrKeySettings = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Settings";
	if (CRegKey regSettings; regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) == ERROR_SUCCESS) {
		//PaneStatus.
		auto& refPanes = GetPanesSettings();
		DWORD dwPaneStatusFileInfo { };
		regSettings.QueryDWORDValue(L"PaneStatusFileInfo", dwPaneStatusFileInfo);
		refPanes.stPSFileInfo = DWORD2PaneStatus(dwPaneStatusFileInfo);
		DWORD dwPaneStatusBkmMgr { };
		regSettings.QueryDWORDValue(L"PaneStatusBkmMgr", dwPaneStatusBkmMgr);
		refPanes.stPSBkmMgr = DWORD2PaneStatus(dwPaneStatusBkmMgr);
		DWORD dwPaneStatusDataInterp { };
		regSettings.QueryDWORDValue(L"PaneStatusDataInterp", dwPaneStatusDataInterp);
		refPanes.stPSDataInterp = DWORD2PaneStatus(dwPaneStatusDataInterp);
		DWORD dwPaneStatusTemplMgr { };
		regSettings.QueryDWORDValue(L"PaneStatusTemplMgr", dwPaneStatusTemplMgr);
		refPanes.stPSTemplMgr = DWORD2PaneStatus(dwPaneStatusTemplMgr);
		DWORD dwPaneStatusLogInfo { };
		regSettings.QueryDWORDValue(L"PaneStatusLogInfo", dwPaneStatusLogInfo);
		refPanes.stPSLogInfo = DWORD2PaneStatus(dwPaneStatusLogInfo);

		//PaneData.
		QWORD ullPaneDataBkmMgr { };
		regSettings.QueryQWORDValue(L"PaneDataBkmMgr", ullPaneDataBkmMgr);
		SetPaneData(IDC_PANE_BKMMGR, ullPaneDataBkmMgr);
		QWORD ullPaneDataDataInterp { };
		regSettings.QueryQWORDValue(L"PaneDataDataInterp", ullPaneDataDataInterp);
		SetPaneData(IDC_PANE_DATAINTERP, ullPaneDataDataInterp);
		QWORD ullPaneDataTemplMgr { };
		regSettings.QueryQWORDValue(L"PaneDataTemplMgr", ullPaneDataTemplMgr);
		SetPaneData(IDC_PANE_TEMPLMGR, ullPaneDataTemplMgr);

		//General settings.
		auto& refGeneral = GetGeneralSettings();
		DWORD dwMultipleInst { };
		regSettings.QueryDWORDValue(L"GeneralInstances", dwMultipleInst);
		refGeneral.fMultipleInst = dwMultipleInst;
		regSettings.QueryDWORDValue(L"GeneralRFLSize", refGeneral.dwRFLSize);
		DWORD dwStartup { };
		regSettings.QueryDWORDValue(L"GeneralStartup", dwStartup);
		refGeneral.eStartup = static_cast<EStartup>(dwStartup);
		DWORD dwWindowsMenu { };
		regSettings.QueryDWORDValue(L"GeneralWindowsMenu", dwWindowsMenu);
		refGeneral.fWindowsMenu = dwWindowsMenu;

		//HexCtrl settings.
		const std::wstring wstrKeyHexCtrl = wstrKeySettings + L"\\HexCtrl";
		if (CRegKey regHexCtrl; regHexCtrl.Open(HKEY_CURRENT_USER, wstrKeyHexCtrl.data()) == ERROR_SUCCESS) {
			//HexCtrl data.
			auto& refSett = GetHexCtrlSettings();
			regHexCtrl.QueryDWORDValue(L"HexCtrlCapacity", refSett.dwCapacity);
			regHexCtrl.QueryDWORDValue(L"HexCtrlGroupSize", refSett.dwGroupSize);
			regHexCtrl.QueryDWORDValue(L"HexCtrlPageSize", refSett.dwPageSize);
			DWORD dwUnprintable { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlUnprintable", dwUnprintable);
			refSett.wchUnprintable = static_cast<wchar_t>(dwUnprintable);
			DWORD dwDateSepar { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlDateSepar", dwDateSepar);
			refSett.wchDateSepar = static_cast<wchar_t>(dwDateSepar);
			regHexCtrl.QueryDWORDValue(L"HexCtrlDateFormat", refSett.dwDateFormat);
			DWORD dwScrollLines { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlIsScrollLines", dwScrollLines);
			refSett.fScrollLines = dwScrollLines;
			DWORD dwScrollRatio { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlScrollSize", dwScrollRatio);
			refSett.flScrollRatio = std::bit_cast<float>(dwScrollRatio);
			DWORD dwInfoBar { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlIsInfoBar", dwInfoBar);
			refSett.fInfoBar = dwInfoBar;
			DWORD dwOffsetHex { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlIsOffsetHex", dwOffsetHex);
			refSett.fOffsetHex = dwOffsetHex;
			regHexCtrl.QueryDWORDValue(L"HexCtrlCharsExtraSpace", refSett.dwCharsExtraSpace);

			//HexCtrl font.
			auto& lf = refSett.stLogFont;
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
			auto& refClrs = refSett.stClrs;
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
		else { GetHexCtrlSettings() = GetHexCtrlDefs(); }
	}
	else {
		GetGeneralSettings() = GetGeneralDefs();
		GetHexCtrlSettings() = GetHexCtrlDefs();
	}

	LoadHexCtrlTemplates();

	m_fLoaded = true;
}

void CAppSettings::OnSettingsChanged()
{
	ShowInWindowsContextMenu(m_stGeneralData.fWindowsMenu);
}

void CAppSettings::RemoveFromLastOpened(std::wstring_view wsvPath)
{
	std::erase(m_vecLastOpened, wsvPath);
}

void CAppSettings::RFLAddToList(std::wstring_view wsvPath, bool fBeginning)
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return;

	m_stRFL.AddToRFL(wsvPath, fBeginning);
}

void CAppSettings::RFLClear()
{
	m_stRFL.ClearRFL();
}

auto CAppSettings::RFLGetDataFromMenuID(UINT uID)const->CAppSettingsRFL::RFLDATA
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	return m_stRFL.GetDataFromMenuID(uID);
}

void CAppSettings::RFLInitialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPDisk, int iMaxEntry)
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return;

	m_stRFL.Initialize(hMenu, iIDMenuFirst, hBMPDisk, iMaxEntry);

	//Recent Files List.
	const std::wstring wstrKeyRFL = std::wstring { L"SOFTWARE\\" } + m_wstrKeyName + L"\\Recent Files List";
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

void CAppSettings::RFLRemoveFromList(std::wstring_view wsvPath)
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return;

	m_stRFL.RemoveFromRFL(wsvPath);
}

void CAppSettings::SaveSettings(std::wstring_view wsvKeyName)
{
	const std::wstring wstrAppKey = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName };

	//Recent Files List.
	CRegKey regRFL;
	regRFL.Open(HKEY_CURRENT_USER, wstrAppKey.data());
	regRFL.RecurseDeleteKey(L"Recent Files List"); //Remove all data to set it below.
	const std::wstring wstrKeyRFL = wstrAppKey + L"\\Recent Files List";
	regRFL.Create(HKEY_CURRENT_USER, wstrKeyRFL.data());
	for (const auto [idx, refData] : RFLGetData() | std::views::enumerate) {
		regRFL.SetStringValue(std::format(L"File{:02d}", idx).data(), refData.wstrDataPath.data());
	}

	//Last Opened List.
	CRegKey regLOL;
	regLOL.Open(HKEY_CURRENT_USER, wstrAppKey.data());
	regLOL.RecurseDeleteKey(L"Last Opened List"); //Remove all data to set it below.
	const std::wstring wstrKeyLOL = wstrAppKey + L"\\Last Opened List";
	regLOL.Create(HKEY_CURRENT_USER, wstrKeyLOL.data());
	for (const auto [idx, wstr] : m_vecLastOpened | std::views::enumerate) {
		regLOL.SetStringValue(std::format(L"File{:02d}", idx).data(), wstr.data());
	}

	//Settings.
	CRegKey regSettings;
	const std::wstring wstrKeySettings = wstrAppKey + L"\\Settings";
	if (regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) != ERROR_SUCCESS) {
		regSettings.Create(HKEY_CURRENT_USER, wstrKeySettings.data());
	}

	//PaneStatus.
	regSettings.SetDWORDValue(L"PaneStatusFileInfo", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_DATAINFO)));
	regSettings.SetDWORDValue(L"PaneStatusBkmMgr", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_BKMMGR)));
	regSettings.SetDWORDValue(L"PaneStatusDataInterp", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_DATAINTERP)));
	regSettings.SetDWORDValue(L"PaneStatusTemplMgr", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_TEMPLMGR)));
	regSettings.SetDWORDValue(L"PaneStatusLogInfo", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_LOGGER)));

	//SetPaneData
	regSettings.SetQWORDValue(L"PaneDataBkmMgr", GetPaneData(IDC_PANE_BKMMGR));
	regSettings.SetQWORDValue(L"PaneDataDataInterp", GetPaneData(IDC_PANE_DATAINTERP));
	regSettings.SetQWORDValue(L"PaneDataTemplMgr", GetPaneData(IDC_PANE_TEMPLMGR));

	//General settings.
	const auto& refGeneral = GetGeneralSettings();
	regSettings.SetDWORDValue(L"GeneralInstances", refGeneral.fMultipleInst);
	regSettings.SetDWORDValue(L"GeneralRFLSize", refGeneral.dwRFLSize);
	regSettings.SetDWORDValue(L"GeneralStartup", std::to_underlying(refGeneral.eStartup));
	regSettings.SetDWORDValue(L"GeneralWindowsMenu", refGeneral.fWindowsMenu);

	//HexCtrl settings.
	CRegKey regHexCtrl;
	const std::wstring wstrKeyHexCtrl = wstrKeySettings + L"\\HexCtrl";
	if (regHexCtrl.Open(HKEY_CURRENT_USER, wstrKeyHexCtrl.data()) != ERROR_SUCCESS) {
		regHexCtrl.Create(HKEY_CURRENT_USER, wstrKeyHexCtrl.data());
	}

	//HexCtrl data.
	const auto& refSett = GetHexCtrlSettings();
	regHexCtrl.SetDWORDValue(L"HexCtrlCapacity", refSett.dwCapacity);
	regHexCtrl.SetDWORDValue(L"HexCtrlGroupSize", refSett.dwGroupSize);
	regHexCtrl.SetDWORDValue(L"HexCtrlPageSize", refSett.dwPageSize);
	regHexCtrl.SetDWORDValue(L"HexCtrlUnprintable", refSett.wchUnprintable);
	regHexCtrl.SetDWORDValue(L"HexCtrlDateFormat", refSett.dwDateFormat);
	regHexCtrl.SetDWORDValue(L"HexCtrlDateSepar", refSett.wchDateSepar);
	regHexCtrl.SetDWORDValue(L"HexCtrlIsScrollLines", refSett.fScrollLines);
	regHexCtrl.SetDWORDValue(L"HexCtrlScrollSize", std::bit_cast<DWORD>(refSett.flScrollRatio));
	regHexCtrl.SetDWORDValue(L"HexCtrlIsInfoBar", refSett.fInfoBar);
	regHexCtrl.SetDWORDValue(L"HexCtrlIsOffsetHex", refSett.fOffsetHex);
	regHexCtrl.SetDWORDValue(L"HexCtrlCharsExtraSpace", refSett.dwCharsExtraSpace);

	//HexCtrl font.
	const auto& refLF = refSett.stLogFont;
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
	const auto& refClrs = refSett.stClrs;
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
	auto& refPanes = GetPanesSettings();
	switch (uPaneID) {
	case IDC_PANE_DATAINFO:
		refPanes.ullPaneDataFileInfo = ullData;
		break;
	case IDC_PANE_BKMMGR:
		refPanes.ullPaneDataBkmMgr = ullData;
		break;
	case IDC_PANE_DATAINTERP:
		refPanes.ullPaneDataDataInterp = ullData;
		break;
	case IDC_PANE_TEMPLMGR:
		refPanes.ullPaneDataTemplMgr = ullData;
		break;
	default:
		break;
	}
}

void CAppSettings::SetPaneStatus(UINT uPaneID, bool fShow, bool fActive)
{
	auto& refPanes = GetPanesSettings();
	switch (uPaneID) {
	case IDC_PANE_DATAINFO:
		refPanes.stPSFileInfo = { fShow, fActive };
		break;
	case IDC_PANE_BKMMGR:
		refPanes.stPSBkmMgr = { fShow, fActive };
		break;
	case IDC_PANE_DATAINTERP:
		refPanes.stPSDataInterp = { fShow, fActive };
		break;
	case IDC_PANE_TEMPLMGR:
		refPanes.stPSTemplMgr = { fShow, fActive };
		break;
	case IDC_PANE_LOGGER:
		refPanes.stPSLogInfo = { fShow, fActive };
		break;
	default:
		break;
	}
}



//CAppSettings Private methods.

auto CAppSettings::GetPanesSettings()->PANESETTINGS&
{
	return m_stPaneSettings;
}

auto CAppSettings::GetPanesSettings()const->const PANESETTINGS&
{
	return m_stPaneSettings;
}

void CAppSettings::LoadHexCtrlTemplates()
{
	wchar_t buff[MAX_PATH];
	GetModuleFileNameW(nullptr, buff, MAX_PATH);
	std::wstring wstrPath = buff;
	wstrPath = wstrPath.substr(0, wstrPath.find_last_of(L'\\'));
	wstrPath += L"\\Templates\\";
	if (const std::filesystem::path pathTemplates { wstrPath }; std::filesystem::exists(pathTemplates)) {
		for (const auto& entry : std::filesystem::directory_iterator { pathTemplates }) {
			const std::wstring_view wsvFile = entry.path().c_str();
			if (const auto npos = wsvFile.find_last_of(L'.'); npos != std::wstring_view::npos
				&& wsvFile.substr(npos + 1) == L"json") { //Check json extension of templates.
				if (auto p = HEXCTRL::IHexTemplates::LoadFromFile(wsvFile.data()); p != nullptr) {
					m_vecHexCtrlTemplates.emplace_back(std::move(p));
				}
			}
		}
	}
}

auto CAppSettings::RFLGetData()const->const std::vector<CAppSettingsRFL::RFLDATA>&
{
	return m_stRFL.GetRFL();
}

void CAppSettings::ShowInWindowsContextMenu(bool fShow)
{
	wchar_t buffPath[MAX_PATH];
	GetModuleFileNameW(nullptr, buffPath, MAX_PATH);
	const std::wstring wstrAppKey = L"*\\shell\\The Hexer";

	CRegKey regCR;
	if (const auto fOpen = regCR.Open(HKEY_CLASSES_ROOT, wstrAppKey.data()) == ERROR_SUCCESS; fOpen == fShow)
		return;

	if (fShow) {
		regCR.Create(HKEY_CLASSES_ROOT, wstrAppKey.data());
		regCR.SetStringValue(nullptr, L"Open in Hexer");
		regCR.SetStringValue(L"Icon", buffPath);
		const auto wstrCommand = wstrAppKey + L"\\command";
		regCR.Create(HKEY_CLASSES_ROOT, wstrCommand.data());
		const auto wstrPath = std::format(L"\"{}\" \"%1\"", buffPath);
		regCR.SetStringValue(nullptr, wstrPath.data());
	}
	else {
		regCR.Close();
		CRegKey regShell;
		regShell.Open(HKEY_CLASSES_ROOT, L"*\\shell");
		regShell.RecurseDeleteKey(L"The Hexer");
	}
}

auto CAppSettings::DWORD2PaneStatus(DWORD dw)->PANESTATUS
{
	const std::bitset<32> bsPS(dw);
	return { bsPS.test(0), bsPS.test(1) };
}

auto CAppSettings::GetGeneralDefs()->const GENERALSETTINGS&
{
	static const GENERALSETTINGS defs { .fMultipleInst { 0 }, .dwRFLSize { 20 }, .eStartup { EStartup::DO_NOTHING },
		.fWindowsMenu { false } };
	return defs;
}

auto CAppSettings::GetHexCtrlDefs()->const HEXCTRLSETTINGS&
{
	static const HEXCTRLSETTINGS defs { .stLogFont { .lfHeight { -MulDiv(11, Ut::GetHiDPIInfo().iLOGPIXELSY, 72) },
		.lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } }, //HexCtrl default font.
		.dwCapacity { 16UL }, .dwDateFormat { 0xFFFFFFFFUL }, .dwGroupSize { 1UL }, .dwPageSize { 0UL },
		.dwCharsExtraSpace { 0UL }, .flScrollRatio { 3.0F }, .wchUnprintable { L'.' }, .wchDateSepar { L'/' },
		.fOffsetHex { true }, .fScrollLines { true }, .fInfoBar { true } };
	return defs;
}

auto CAppSettings::PaneStatus2DWORD(PANESTATUS ps)->DWORD
{
	std::bitset<32> bs;
	bs[0] = ps.fIsVisible;
	bs[1] = ps.fIsActive;

	return bs.to_ulong();
}