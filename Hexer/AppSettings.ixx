module;
/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "resource.h"
#include "sqlite3.h"
#include <afxwin.h>
#include "HexCtrl.h"
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

#ifdef _DEBUG
#define SQLITE_LIBNAME "dep\\sqlite\\sqlite3d.lib"
#else
#define SQLITE_LIBNAME "dep\\sqlite\\sqlite3.lib"
#endif
#pragma comment (lib, SQLITE_LIBNAME)

import Utility;

using VecDataOpen = std::vector<ut::DATAOPEN>;

//CAppSettingsRFL.
class CAppSettingsRFL final //Recent Files List.
{
public:
	CAppSettingsRFL() = default;
	void AddToList(const ut::DATAOPEN& dos);
	[[nodiscard]] auto GetDataFromMenuID(UINT uID)const -> ut::DATAOPEN;
	void Initialize(HMENU hMenu, int iIDMenuFirst, int iMaxEntry, HBITMAP hBMPFile, HBITMAP hBMPDevice,
		HBITMAP hBMPProcess, const std::wstring& wstrRFLRegPath);
	[[nodiscard]] bool IsInitialized()const;
	void RemoveFromList(const ut::DATAOPEN& dos);
	void SetRFLSize(DWORD dwRFLSize);
	void SaveSettings()const;
	void UpdateMenuIcons(HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess);
	[[nodiscard]] static void AddToListBeginning(const ut::DATAOPEN& dos, VecDataOpen& vecData);
	[[nodiscard]] static auto ReadRegData(const wchar_t* pwszRegPath) -> VecDataOpen;
	[[nodiscard]] static void SaveRegData(const wchar_t* pwszRegPath, const VecDataOpen& vecData);
private:
	[[nodiscard]] auto GetRegRFLPath()const -> const wchar_t*;
	void RebuildRFLMenu();
	void UpdateMenuIcons();
private:
	VecDataOpen m_vecRFL;      //Recent Files List data.
	const wchar_t* m_pwszRFLRegPath { };
	HMENU m_hMenu { };
	HBITMAP m_hBMPFile { };    //Bitmap for file icon.
	HBITMAP m_hBMPDevice { };  //Bitmap for device icon.
	HBITMAP m_hBMPProcess { }; //Bitmap for process icon.
	DWORD m_dwRFLSize { };     //RFL maximum size.
	int m_iIDMenuFirst { };
	bool m_fInit { };
};

void CAppSettingsRFL::AddToList(const ut::DATAOPEN& dos)
{
	AddToListBeginning(dos, m_vecRFL);
	if (m_vecRFL.size() > m_dwRFLSize) {
		m_vecRFL.resize(m_dwRFLSize);
	}

	RebuildRFLMenu();
}

auto CAppSettingsRFL::GetDataFromMenuID(UINT uID)const->ut::DATAOPEN
{
	assert(m_fInit);
	if (!m_fInit)
		return { };

	const auto uIndex = uID - m_iIDMenuFirst;
	if (uIndex >= m_vecRFL.size())
		return { };

	return m_vecRFL[uIndex];
}

void CAppSettingsRFL::Initialize(HMENU hMenu, int iIDMenuFirst, int iMaxEntry,
	HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess, const std::wstring& wstrRFLRegPath) {
	assert(IsMenu(hMenu));
	if (!IsMenu(hMenu)) {
		return;
	}

	m_hMenu = hMenu;
	m_iIDMenuFirst = iIDMenuFirst;
	m_dwRFLSize = std::clamp(iMaxEntry, 0, 20);
	m_hBMPFile = hBMPFile;
	m_hBMPDevice = hBMPDevice;
	m_hBMPProcess = hBMPProcess;
	m_pwszRFLRegPath = wstrRFLRegPath.data();
	m_fInit = true;
	m_vecRFL = ReadRegData(GetRegRFLPath());
	if (m_vecRFL.size() > m_dwRFLSize) {
		m_vecRFL.resize(m_dwRFLSize);
	}

	RebuildRFLMenu();
}

bool CAppSettingsRFL::IsInitialized()const {
	return m_fInit;
}

void CAppSettingsRFL::RemoveFromList(const ut::DATAOPEN& dos)
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	std::erase_if(m_vecRFL, [&dos](const ut::DATAOPEN& refData) { return refData == dos; });
	RebuildRFLMenu();
}

void CAppSettingsRFL::SetRFLSize(DWORD dwRFLSize)
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	m_dwRFLSize = dwRFLSize;
	if (m_vecRFL.size() > m_dwRFLSize) {
		m_vecRFL.resize(m_dwRFLSize);
	}
}

void CAppSettingsRFL::SaveSettings()const
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	SaveRegData(GetRegRFLPath(), m_vecRFL);
}

void CAppSettingsRFL::UpdateMenuIcons(HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess) {
	m_hBMPFile = hBMPFile;
	m_hBMPDevice = hBMPDevice;
	m_hBMPProcess = hBMPProcess;
	UpdateMenuIcons();
}

auto CAppSettingsRFL::GetRegRFLPath()const->const wchar_t*
{
	return m_pwszRFLRegPath;
}

void CAppSettingsRFL::AddToListBeginning(const ut::DATAOPEN& dos, VecDataOpen& vecData)
{
	using enum ut::EOpenMode;
	//Remove any duplicates. Processes can have same names but different IDs.
	std::erase_if(vecData, [&dos](const ut::DATAOPEN& refData) { return refData == dos; });
	const auto eOpenMode = dos.eOpenMode == NEW_FILE ? OPEN_FILE : dos.eOpenMode;
	vecData.emplace(vecData.begin(), ut::DATAOPEN {
		.wstrDataPath { dos.wstrDataPath }, .wstrFriendlyName { dos.wstrFriendlyName },
		.dwProcID { dos.dwProcID }, .eOpenMode { eOpenMode } });
}

auto CAppSettingsRFL::ReadRegData(const wchar_t* pwszRegPath)->VecDataOpen
{
	CRegKey reg;
	if (reg.Open(HKEY_CURRENT_USER, pwszRegPath) != ERROR_SUCCESS) {
		return { };
	}

	VecDataOpen vecRet;
	int iCode { };
	wchar_t buffName[32];
	wchar_t buffData[MAX_PATH];
	DWORD dwIndex = 0;
	while (iCode != ERROR_NO_MORE_ITEMS) {
		DWORD dwNameSize { sizeof(buffName) / sizeof(wchar_t) };
		DWORD dwDataType { };
		DWORD dwDataSize { MAX_PATH * sizeof(wchar_t) };
		iCode = RegEnumValueW(reg, dwIndex++, buffName, &dwNameSize, nullptr, &dwDataType,
			reinterpret_cast<LPBYTE>(&buffData), &dwDataSize);

		if (iCode == ERROR_SUCCESS && dwDataType == REG_SZ) {
			const auto wsvName = std::wstring_view { buffName };

			using enum ut::EOpenMode;
			ut::EOpenMode eOpenMode { };
			if (const auto it = wsvName.find(L':'); it != std::wstring_view::npos) {
				const auto wsvMode = wsvName.substr(it + 1);
				const auto optEOpenMode = ut::GetEOpenModeWstr(wsvMode);
				if (!optEOpenMode)
					continue;

				eOpenMode = *optEOpenMode;
			}

			ut::DATAOPEN stDOS { .eOpenMode { eOpenMode } };
			const auto wsvData = std::wstring_view { buffData };
			if (eOpenMode == OPEN_PROC) {
				const auto nID = wsvData.find(L"ID:");
				if (nID == std::wstring_view::npos) {
					continue;
				}

				const auto nIDStart = nID + 3;
				const auto nIDEnd = wsvData.find(L';', nIDStart);
				if (nIDEnd == std::wstring_view::npos) {
					continue;
				}

				const auto optID = stn::StrToUInt32(wsvData.substr(nIDStart, nIDEnd - nIDStart));
				if (!optID) {
					continue;
				}

				stDOS.dwProcID = *optID; //Process ID.

				const auto nName = wsvData.find(L"Name:", nIDEnd + 1);
				if (nName == std::wstring_view::npos) {
					continue;
				}

				const auto nNameStart = nName + 5;
				const auto nNameEnd = wsvData.find(L';', nNameStart);
				if (nNameEnd == std::wstring_view::npos) {
					continue;
				}

				stDOS.wstrDataPath = wsvData.substr(nNameStart, nNameEnd - nNameStart); //Process name.
			}
			else if (eOpenMode == OPEN_DRIVE) {
				const auto nPath = wsvData.find(L"Path:");
				if (nPath == std::wstring_view::npos) {
					continue;
				}

				const auto nPathStart = nPath + 5;
				const auto nPathEnd = wsvData.find(L';', nPathStart);
				if (nPathEnd == std::wstring_view::npos) {
					continue;
				}

				stDOS.wstrDataPath = wsvData.substr(nPathStart, nPathEnd - nPathStart); //Path.

				const auto nName = wsvData.find(L"FriendlyName:", nPathEnd + 1);
				if (nName == std::wstring_view::npos) {
					continue;
				}

				const auto nNameStart = nName + 13 /*length of "FriendlyName:"*/;
				const auto nNameEnd = wsvData.find(L';', nNameStart);
				if (nNameEnd == std::wstring_view::npos) {
					continue;
				}

				stDOS.wstrFriendlyName = wsvData.substr(nNameStart, nNameEnd - nNameStart); //Friendly name.
			}
			else {
				stDOS.wstrDataPath = wsvData;
			}

			vecRet.emplace_back(stDOS);
		}

		if (dwIndex > 50) { //Sentinel.
			break;
		}
	}

	return vecRet;
}

void CAppSettingsRFL::SaveRegData(const wchar_t* pwszRegPath, const VecDataOpen& vecData)
{
	using enum ut::EOpenMode;
	const auto wsv = std::wstring_view { pwszRegPath };
	const auto wsvRegKeyName = wsv.substr(wsv.find_last_of(L'\\') + 1);
	const auto wstrRegPathWithoutKey = std::wstring { wsv.substr(0, wsv.find_last_of(L'\\')) };

	CRegKey reg;
	if (reg.Open(HKEY_CURRENT_USER, wstrRegPathWithoutKey.data()) == ERROR_SUCCESS) {
		reg.RecurseDeleteKey(wsvRegKeyName.data()); //Remove all data in the key.
	}

	reg.Create(HKEY_CURRENT_USER, pwszRegPath);
	for (const auto& [idx, ref] : vecData | std::views::enumerate) {
		if (ref.eOpenMode == OPEN_PROC) {
			reg.SetStringValue(std::format(L"{:02d}:{}", idx, ut::GetWstrEOpenMode(ref.eOpenMode)).data(),
				std::format(L"ID:{};Name:{};", ref.dwProcID, ref.wstrDataPath).data());
		}
		else if (ref.eOpenMode == OPEN_DRIVE) {
			reg.SetStringValue(std::format(L"{:02d}:{}", idx, ut::GetWstrEOpenMode(ref.eOpenMode)).data(),
				std::format(L"Path:{};FriendlyName:{};", ref.wstrDataPath, ref.wstrFriendlyName).data());
		}
		else {
			reg.SetStringValue(std::format(L"{:02d}:{}", idx, ut::GetWstrEOpenMode(ref.eOpenMode)).data(),
				ref.wstrDataPath.data());
		}
	}
}


//CAppSettingsRFL private methods.

void CAppSettingsRFL::RebuildRFLMenu()
{
	using enum ut::EOpenMode;
	assert(m_fInit);
	if (!m_fInit)
		return;

	while (GetMenuItemCount(m_hMenu) > 0) {
		DeleteMenu(m_hMenu, 0, MF_BYPOSITION); //Removing all RFL menu items.
	}

	for (const auto& [idx, refData] : m_vecRFL | std::views::enumerate) {
		if (idx >= m_dwRFLSize) //Adding no more than m_dwRFLSize.
			break;

		std::wstring wstrMenu;
		if (refData.eOpenMode == OPEN_PROC) {
			wstrMenu = std::format(L"{} {}: {} (ID: {})", idx + 1, ut::GetWstrEOpenMode(refData.eOpenMode), refData.wstrDataPath,
				refData.dwProcID);
		}
		else {
			wstrMenu = std::format(L"{} {}: {}", idx + 1, ut::GetWstrEOpenMode(refData.eOpenMode), refData.wstrDataPath);
		}

		const auto uMenuID = static_cast<UINT>(m_iIDMenuFirst + idx);
		AppendMenuW(m_hMenu, MF_STRING, uMenuID, wstrMenu.data());
	}

	UpdateMenuIcons();
}

void CAppSettingsRFL::UpdateMenuIcons() {
	for (const auto& [idx, refData] : m_vecRFL | std::views::enumerate) {
		if (idx >= m_dwRFLSize) //Adding not more than m_dwRFLSize.
			break;

		HBITMAP hBmp { };
		switch (refData.eOpenMode) {
		case OPEN_FILE:
			hBmp = m_hBMPFile;
			break;
		case OPEN_DRIVE:
		case OPEN_VOLUME:
		case OPEN_PATH:
			hBmp = m_hBMPDevice;
			break;
		case OPEN_PROC:
			hBmp = m_hBMPProcess;
			break;
		default:
			break;
		}

		const MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { hBmp } };
		const auto uMenuID = static_cast<UINT>(m_iIDMenuFirst + idx);
		::SetMenuItemInfoW(m_hMenu, uMenuID, FALSE, &mii);
	}
}


//CAppSettings.
export class CAppSettings final {
public:
	using VecTemplates = std::vector<std::unique_ptr<HEXCTRL::HEXTEMPLATE>>;
	using VecBkm = std::vector<HEXCTRL::HEXBKM>;
	using SpnBkm = std::span<HEXCTRL::HEXBKM>;
	using VecSpan = std::vector<HEXCTRL::HEXSPAN>;
	using uptr_sqlite = std::unique_ptr < sqlite3, decltype([](sqlite3* pDB) {
		auto res = sqlite3_close(pDB);
		assert(res == SQLITE_OK); }) > ;
	using uptr_stmt = std::unique_ptr < sqlite3_stmt, decltype([](sqlite3_stmt* pSTMT) {
		auto res = sqlite3_finalize(pSTMT);
		assert(res == SQLITE_OK); }) > ;
	struct PANESTATUS {
		bool fVisible : 1{};
		bool fActive : 1{};
	};
	struct PANESETTINGS {
		std::uint64_t ullPaneDataFileInfo { };   //Pane data for the "File Info".
		std::uint64_t ullPaneDataBkmMgr { };     //Pane data for the "Bokmark Manager".
		std::uint64_t ullPaneDataDataInterp { }; //Pane data for the "Template Manager".
		std::uint64_t ullPaneDataTemplMgr { };   //Pane data for the "Data Interpreter".
		PANESTATUS    stPSFileInfo { };          //Pane status for the "File Properties".
		PANESTATUS    stPSBkmMgr { };            //Pane status for the "Bokmark Manager".
		PANESTATUS    stPSDataInterp { };        //Pane status for the "Data Interpreter".
		PANESTATUS    stPSTemplMgr { };          //Pane status for the "Template Manager".
		PANESTATUS    stPSLogInfo { };           //Pane status for the "Log Information".
	};
	enum class EOnStartup :std::uint8_t {
		DO_NOTHING, RESTORE_LAST_OPENED, SHOW_NEW_FILE, SHOW_OPEN_FILE, SHOW_OPEN_DEVICE, SHOW_OPEN_PROC
	};
	struct GENERALSETTINGS {
		bool            fMultipleInst { }; //0-Single, 1-Multiple.
		DWORD           dwRFLSize { };
		EOnStartup      eOnStartup { };
		ut::DATAACCESS  stDAC { };
		ut::EDataIOMode eDataIOMode { };
		bool            fWindowsMenu { }; //1-Show, 0-Don't show.
	};
	struct HEXCTRLSETTINGS {
		LOGFONTW stLogFont { };
		HEXCTRL::HEXCOLORS stClrs;
		DWORD    dwCapacity { };
		DWORD    dwDateFormat { };
		DWORD    dwGroupSize { };
		DWORD    dwPageSize { };
		DWORD    dwCharsExtraSpace { };
		float    flScrollRatio { };
		wchar_t  wchUnprintable { }; //Replacement char for unprintable characters.
		wchar_t  wchDateSepar { };   //Date separator.
		bool     fOffsetHex { };
		bool     fHexCharsCaseUpper { };
		bool     fScrollLines { };
		bool     fInfoBar { };
	};
	struct ICONDATA {
		UINT uIDCmd { };
		UINT uIDSVG { };
		HBITMAP hBmp { };
		HICON hIcon { };
		void Clear()const { ::DeleteObject(hBmp); ::DestroyIcon(hIcon); }
	};
	CAppSettings() = default;
	CAppSettings(const CAppSettings&) = delete;
	CAppSettings(CAppSettings&&) = delete;
	CAppSettings& operator=(const CAppSettings&) = delete;
	~CAppSettings() = default;
	[[nodiscard]] auto GetGeneralSettings() -> GENERALSETTINGS&;
	[[nodiscard]] auto GetHexCtrlSettings() -> HEXCTRLSETTINGS&;
	[[nodiscard]] auto GetHexCtrlTemplates()const -> const VecTemplates&;
	[[nodiscard]] auto GetIconDataForCmd(UINT uCMD)const -> const ICONDATA*;
	[[nodiscard]] auto GetLastOpenedList()const -> VecDataOpen;
	[[nodiscard]] auto GetPaneData(UINT uPaneID)const -> std::uint64_t;
	[[nodiscard]] auto GetPaneStatus(UINT uPaneID)const -> PANESTATUS;
	[[nodiscard]] auto GetSavedBkms(std::wstring_view wsvDataName)const -> VecBkm;
	[[nodiscard]] bool IsRFLInitialized()const;
	void LoadSettings(std::wstring_view wsvAppName);
	void LOLAddToList(const ut::DATAOPEN& dos);      //Last Opened List add.
	void LOLRemoveFromList(const ut::DATAOPEN& dos); //Last Opened List remove.
	void OnSettingsChanged();
	void RecreateCMDIcons(int iWidth, int iHeight); //All menu and toolbar icons.
	void RFLAddToList(const ut::DATAOPEN& dos);
	[[nodiscard]] auto RFLGetDataFromMenuID(UINT uID)const -> ut::DATAOPEN;
	void RFLInitialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess);
	void RFLRemoveFromList(const ut::DATAOPEN& dos);
	void RFLUpdateMenuIcons(HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess);
	void SaveBkms(std::wstring_view wsvDataName, SpnBkm spnBkm);
	void SaveSettings();
	void SetPaneData(UINT uPaneID, std::uint64_t ullData);
	void SetPaneStatus(UINT uPaneID, bool fVisible, bool fActive);
	[[nodiscard]] static auto GetGeneralDefs() -> const GENERALSETTINGS&;
	[[nodiscard]] static auto GetHexCtrlDefs() -> const HEXCTRLSETTINGS&;
private:
	[[nodiscard]] auto GetAppName()const -> const std::wstring&;
	[[nodiscard]] auto GetPanesSettings() -> PANESETTINGS&;
	[[nodiscard]] auto GetPanesSettings()const -> const PANESETTINGS&;
	[[nodiscard]] auto GetRegBasePath()const -> const std::wstring&;
	[[nodiscard]] auto GetRegHexCtrlSettingsPath()const -> const std::wstring&;
	[[nodiscard]] auto GetRegLOLPath()const -> const std::wstring&; //Last Opened List.
	[[nodiscard]] auto GetRegRFLPath()const -> const std::wstring&; //Recent Files List.
	[[nodiscard]] auto GetRegSettingsPath()const -> const std::wstring&;
	[[nodiscard]] auto GetSQLiteDB()const -> sqlite3*;
	void LoadHexCtrlTemplates();
	void ShowInWindowsContextMenu(bool fShow);
	static void DBCreateTables(sqlite3* pDB);
	[[nodiscard]] static auto DBGetFileIDByName(sqlite3* pDB, std::wstring_view wsvFileName) -> std::int64_t;
	[[nodiscard]] static auto DBLoadBkmForFileID(sqlite3* pDB, std::int64_t i64FileID) -> VecBkm;
	[[nodiscard]] static auto DBOpenDB(const wchar_t* pwszPathDB) -> uptr_sqlite;
	static void DBSaveBkmForFileID(sqlite3* pDB, std::int64_t i64FileID, SpnBkm spnBkm);
	static void DBSaveHexCtrlSettings(sqlite3* pDB, const HEXCTRLSETTINGS& sett);
	[[nodiscard]] static auto DWORD2PaneStatus(DWORD dw) -> PANESTATUS;
	[[nodiscard]] static auto PaneStatus2DWORD(PANESTATUS ps) -> DWORD;
private:
	std::vector<ICONDATA> m_vecIconData {
		{ ICONDATA { .uIDCmd { IDM_FILE_NEWFILE }, .uIDSVG { IDR_SVG_FILE_NEW } } },
		{ ICONDATA { .uIDCmd { IDM_FILE_OPENFILE }, .uIDSVG { IDR_SVG_FILE_OPEN } } },
		{ ICONDATA { .uIDCmd { IDM_FILE_OPENDEVICE }, .uIDSVG { IDR_SVG_DEVICE_OPEN } } },
		{ ICONDATA { .uIDCmd { IDM_FILE_OPENPROCESS }, .uIDSVG { IDR_SVG_PROCESS_OPEN } } },
		{ ICONDATA { .uIDCmd { IDM_FILE_SAVE }, .uIDSVG { IDR_SVG_SAVE } } },
		{ ICONDATA { .uIDCmd { IDM_TOOLS_SETTINGS }, .uIDSVG { IDR_SVG_SETTINGS } } },
		{ ICONDATA { .uIDCmd { IDM_EDIT_COPYHEX }, .uIDSVG { IDR_SVG_CLPBRD_COPYHEX } } },
		{ ICONDATA { .uIDCmd { IDM_EDIT_PASTEHEX }, .uIDSVG { IDR_SVG_CLPBRD_PASTEHEX } } },
		{ ICONDATA { .uIDCmd { IDM_FIND_SEARCH }, .uIDSVG { IDR_SVG_SEARCH } } },
		{ ICONDATA { .uIDCmd { IDM_VIEW_BKMMGR }, .uIDSVG { IDR_SVG_BKM } } },
		{ ICONDATA { .uIDCmd { IDR_SVG_MODIFY }, .uIDSVG { IDR_SVG_MODIFY } } },
		{ ICONDATA { .uIDCmd { IDR_SVG_FONTCHOOSE }, .uIDSVG { IDR_SVG_FONTCHOOSE } } }
	};
	CAppSettingsRFL m_stRFL;
	PANESETTINGS m_stPaneSettings;      //"Panes" settings data.
	GENERALSETTINGS m_stGeneralData;    //"General" settings data.
	HEXCTRLSETTINGS m_stHexCtrlData;    //"HexCtrl" settings data.
	std::wstring m_wstrAppName;         //Application name for registry paths.
	VecDataOpen m_vecLOL;               //Last Opened Files list.
	VecTemplates m_vecHexCtrlTemplates; //HexCtrl loaded templates.
	uptr_sqlite m_upSQLiteDB;           //SQLite database pointer.
	bool m_fLoaded { false };           //LoadSettings has succeeded.
};


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

auto CAppSettings::GetIconDataForCmd(UINT uCmd)const->const ICONDATA*
{
	const auto it = std::ranges::find_if(m_vecIconData, [=](const ICONDATA& data) { return data.uIDCmd == uCmd; });
	return it != std::ranges::end(m_vecIconData) ? &*it : nullptr;
}

auto CAppSettings::GetLastOpenedList()const->VecDataOpen
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	return CAppSettingsRFL::ReadRegData(GetRegLOLPath().data());
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

auto CAppSettings::GetSavedBkms(std::wstring_view wsvDataName)const->VecBkm
{
	const auto pDB = GetSQLiteDB();
	return DBLoadBkmForFileID(pDB, DBGetFileIDByName(pDB, wsvDataName));
}

bool CAppSettings::IsRFLInitialized()const {
	return m_stRFL.IsInitialized();
}

void CAppSettings::LoadSettings(std::wstring_view wsvAppName)
{
	assert(!wsvAppName.empty());
	if (wsvAppName.empty())
		return;

	if (m_upSQLiteDB == nullptr) {
		m_upSQLiteDB = DBOpenDB(ut::GetSQLiteDBName().data());
		DBCreateTables(m_upSQLiteDB.get());
	}

	m_wstrAppName = wsvAppName;

	//Filling all the default settings first, then load from the registry.
	GetGeneralSettings() = GetGeneralDefs();
	GetHexCtrlSettings() = GetHexCtrlDefs();

	//Settings.
	const auto& refSettPath = GetRegSettingsPath();
	if (CRegKey regSettings; regSettings.Open(HKEY_CURRENT_USER, refSettPath.data()) == ERROR_SUCCESS) {
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
		refGeneral.eOnStartup = static_cast<EOnStartup>(dwStartup);
		DWORD dwWindowsMenu { };
		regSettings.QueryDWORDValue(L"GeneralWindowsMenu", dwWindowsMenu);
		refGeneral.fWindowsMenu = dwWindowsMenu;
		DWORD dwDataAccessMode { };
		regSettings.QueryDWORDValue(L"GeneralDataAccessMode", dwDataAccessMode);
		refGeneral.stDAC = (std::min)(dwDataAccessMode, static_cast<DWORD>(ut::EDataAccessMode::ACCESS_INPLACE));
		DWORD dwDataIOMode { };
		regSettings.QueryDWORDValue(L"GeneralDataIOMode", dwDataIOMode);
		refGeneral.eDataIOMode = (std::min)(static_cast<ut::EDataIOMode>(dwDataIOMode), ut::EDataIOMode::DATA_IOIMMEDIATE);

		//HexCtrl settings.
		const auto& refHexCtrlSettPath = GetRegHexCtrlSettingsPath();
		if (CRegKey regHexCtrl; regHexCtrl.Open(HKEY_CURRENT_USER, refHexCtrlSettPath.data()) == ERROR_SUCCESS) {
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
			DWORD dwHexCharsCaseUpper { };
			regHexCtrl.QueryDWORDValue(L"HexCtrlIsHexCharsCaseUpper", dwHexCharsCaseUpper);
			refSett.fHexCharsCaseUpper = dwHexCharsCaseUpper;
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
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontCaption", refClrs.clrFontCaption);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontHex", refClrs.clrFontHex);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontText", refClrs.clrFontText);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontOffset", refClrs.clrFontOffset);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontSel", refClrs.clrFontSel);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontDataInterp", refClrs.clrFontDataInterp);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontInfoParam", refClrs.clrFontInfoParam);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontInfoData", refClrs.clrFontInfoData);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontCaret", refClrs.clrFontCaret);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrFontBkm", refClrs.clrFontBkm);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBk", refClrs.clrBk);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkHex", refClrs.clrBkHex);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkText", refClrs.clrBkText);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkOffset", refClrs.clrBkOffset);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkSel", refClrs.clrBkSel);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkDataInterp", refClrs.clrBkDataInterp);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkInfoBar", refClrs.clrBkInfoBar);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkCaret", refClrs.clrBkCaret);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkCaretSel", refClrs.clrBkCaretSel);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrBkBkm", refClrs.clrBkBkm);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrLinesMain", refClrs.clrLinesMain);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrLinesTempl", refClrs.clrLinesTempl);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrScrollBar", refClrs.clrScrollBar);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrScrollThumb", refClrs.clrScrollThumb);
			regHexCtrl.QueryDWORDValue(L"HexCtrlClrScrollArrow", refClrs.clrScrollArrow);
		}
	}

	LoadHexCtrlTemplates();

	m_fLoaded = true;
}

void CAppSettings::LOLAddToList(const ut::DATAOPEN& dos)
{
	CAppSettingsRFL::AddToListBeginning(dos, m_vecLOL);

	if (m_vecLOL.size() > 20) { //Sentinel.
		m_vecLOL.resize(20);
	}
}

void CAppSettings::LOLRemoveFromList(const ut::DATAOPEN& dos)
{
	std::erase_if(m_vecLOL, [&dos](const ut::DATAOPEN& refData) { return refData == dos; });
}

void CAppSettings::OnSettingsChanged()
{
	ShowInWindowsContextMenu(m_stGeneralData.fWindowsMenu);
	m_stRFL.SetRFLSize(m_stGeneralData.dwRFLSize);
}

void CAppSettings::RecreateCMDIcons(int iWidth, int iHeight) {
	for (auto& data : m_vecIconData) {
		data.Clear();
		data.hBmp = ut::SVGToBmp(data.uIDSVG, iWidth, iHeight);
		data.hIcon = ut::HICONFromHBITMAP(data.hBmp);
	}
}

void CAppSettings::RFLAddToList(const ut::DATAOPEN& dos)
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return;

	m_stRFL.AddToList(dos);
}

auto CAppSettings::RFLGetDataFromMenuID(UINT uID)const->ut::DATAOPEN
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	return m_stRFL.GetDataFromMenuID(uID);
}

void CAppSettings::RFLInitialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess)
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return;

	m_stRFL.Initialize(hMenu, iIDMenuFirst, GetGeneralSettings().dwRFLSize,
		hBMPFile, hBMPDevice, hBMPProcess, GetRegRFLPath());
}

void CAppSettings::RFLRemoveFromList(const ut::DATAOPEN& dos)
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return;

	m_stRFL.RemoveFromList(dos);
}

void CAppSettings::RFLUpdateMenuIcons(HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess) {
	m_stRFL.UpdateMenuIcons(hBMPFile, hBMPDevice, hBMPProcess);
}

void CAppSettings::SaveBkms(std::wstring_view wsvDataName, SpnBkm spnBkm)
{
	const auto pDB = GetSQLiteDB();
	const auto i64FileID = DBGetFileIDByName(pDB, wsvDataName);
	DBSaveBkmForFileID(pDB, i64FileID, spnBkm);
}

void CAppSettings::SaveSettings()
{
	//Recent Files List.
	m_stRFL.SaveSettings();

	//Last Opened List.
	CAppSettingsRFL::SaveRegData(GetRegLOLPath().data(), m_vecLOL);

	//Settings.
	CRegKey regSettings;
	const auto& refSettPath = GetRegSettingsPath();
	if (regSettings.Open(HKEY_CURRENT_USER, refSettPath.data()) != ERROR_SUCCESS) {
		regSettings.Create(HKEY_CURRENT_USER, refSettPath.data());
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
	regSettings.SetDWORDValue(L"GeneralStartup", std::to_underlying(refGeneral.eOnStartup));
	regSettings.SetDWORDValue(L"GeneralWindowsMenu", refGeneral.fWindowsMenu);
	regSettings.SetDWORDValue(L"GeneralDataAccessMode", refGeneral.stDAC);
	regSettings.SetDWORDValue(L"GeneralDataIOMode", std::to_underlying(refGeneral.eDataIOMode));

	//HexCtrl settings.
	CRegKey regHexCtrl;
	const auto& refHexCtrlSettPath = GetRegHexCtrlSettingsPath();
	if (regHexCtrl.Open(HKEY_CURRENT_USER, refHexCtrlSettPath.data()) != ERROR_SUCCESS) {
		regHexCtrl.Create(HKEY_CURRENT_USER, refHexCtrlSettPath.data());
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
	regHexCtrl.SetDWORDValue(L"HexCtrlIsHexCharsCaseUpper", refSett.fHexCharsCaseUpper);
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
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontCaption", refClrs.clrFontCaption);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontHex", refClrs.clrFontHex);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontText", refClrs.clrFontText);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontOffset", refClrs.clrFontOffset);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontSel", refClrs.clrFontSel);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontDataInterp", refClrs.clrFontDataInterp);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontInfoParam", refClrs.clrFontInfoParam);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontInfoData", refClrs.clrFontInfoData);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontCaret", refClrs.clrFontCaret);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrFontBkm", refClrs.clrFontBkm);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBk", refClrs.clrBk);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkHex", refClrs.clrBkHex);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkText", refClrs.clrBkText);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkOffset", refClrs.clrBkOffset);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkSel", refClrs.clrBkSel);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkDataInterp", refClrs.clrBkDataInterp);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkInfoBar", refClrs.clrBkInfoBar);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkCaret", refClrs.clrBkCaret);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkCaretSel", refClrs.clrBkCaretSel);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrBkBkm", refClrs.clrBkBkm);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrLinesMain", refClrs.clrLinesMain);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrLinesTempl", refClrs.clrLinesTempl);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrScrollBar", refClrs.clrScrollBar);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrScrollThumb", refClrs.clrScrollThumb);
	regHexCtrl.SetDWORDValue(L"HexCtrlClrScrollArrow", refClrs.clrScrollArrow);
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

void CAppSettings::SetPaneStatus(UINT uPaneID, bool fVisible, bool fActive)
{
	auto& refPanes = GetPanesSettings();
	switch (uPaneID) {
	case IDC_PANE_DATAINFO:
		refPanes.stPSFileInfo = { .fVisible { fVisible }, .fActive { fActive } };
		break;
	case IDC_PANE_BKMMGR:
		refPanes.stPSBkmMgr = { .fVisible { fVisible }, .fActive { fActive } };
		break;
	case IDC_PANE_DATAINTERP:
		refPanes.stPSDataInterp = { .fVisible { fVisible }, .fActive { fActive } };
		break;
	case IDC_PANE_TEMPLMGR:
		refPanes.stPSTemplMgr = { .fVisible { fVisible }, .fActive { fActive } };
		break;
	case IDC_PANE_LOGGER:
		refPanes.stPSLogInfo = { .fVisible { fVisible }, .fActive { fActive } };
		break;
	default:
		break;
	}
}

auto CAppSettings::GetGeneralDefs()->const GENERALSETTINGS&
{
	static const GENERALSETTINGS defs { .fMultipleInst { false }, .dwRFLSize { 20 }, .eOnStartup { EOnStartup::DO_NOTHING },
		.stDAC { 0 }, .eDataIOMode { ut::EDataIOMode::DATA_MMAP }, .fWindowsMenu { false } };
	return defs;
}

auto CAppSettings::GetHexCtrlDefs()->const HEXCTRLSETTINGS&
{
	static const HEXCTRLSETTINGS defs { .stLogFont { .lfHeight { -ut::FontPixelsFromPoints(11) },
		.lfPitchAndFamily { FIXED_PITCH }, .lfFaceName { L"Consolas" } }, //HexCtrl default font.
		.dwCapacity { 16UL }, .dwDateFormat { 0xFFFFFFFFUL }, .dwGroupSize { 1UL }, .dwPageSize { 0UL },
		.dwCharsExtraSpace { 0UL }, .flScrollRatio { 3.0F }, .wchUnprintable { L'.' }, .wchDateSepar { L'/' },
		.fOffsetHex { true }, .fHexCharsCaseUpper { true }, .fScrollLines { true }, .fInfoBar { true } };
	return defs;
}


//CAppSettings Private methods.

auto CAppSettings::GetAppName()const->const std::wstring& {
	return m_wstrAppName;
}

auto CAppSettings::GetPanesSettings()->PANESETTINGS& {
	return m_stPaneSettings;
}

auto CAppSettings::GetPanesSettings()const->const PANESETTINGS& {
	return m_stPaneSettings;
}

auto CAppSettings::GetRegBasePath()const->const std::wstring&
{
	static const auto wstrBase = L"Software\\" + GetAppName();
	return wstrBase;
}

auto CAppSettings::GetRegHexCtrlSettingsPath()const->const std::wstring&
{
	static const auto wstrHexCtrlSett = GetRegSettingsPath() + L"\\HexCtrl";
	return wstrHexCtrlSett;
}

auto CAppSettings::GetRegLOLPath()const->const std::wstring&
{
	static const auto wstrLOL = GetRegBasePath() + L"\\Last Opened List";
	return wstrLOL;
}

auto CAppSettings::GetRegRFLPath()const->const std::wstring&
{
	static const auto wstrRFL = GetRegBasePath() + L"\\Recent Files List";
	return wstrRFL;
}

auto CAppSettings::GetRegSettingsPath()const->const std::wstring&
{
	static const auto wstrSettings = GetRegBasePath() + L"\\Settings";
	return wstrSettings;
}

auto CAppSettings::GetSQLiteDB()const->sqlite3*
{
	return m_upSQLiteDB.get();
}

void CAppSettings::LoadHexCtrlTemplates()
{
	auto wstrDir = ut::GetModuleDir();
	wstrDir += L"\\Templates\\";
	if (const std::filesystem::path pathTemplates { wstrDir }; std::filesystem::exists(pathTemplates)) {
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

void CAppSettings::ShowInWindowsContextMenu(bool fShow)
{
	const std::wstring wstrAppKey = L"Software\\Classes\\*\\shell\\The Hexer";
	CRegKey regCR;
	if (const auto fOpen = regCR.Open(HKEY_CURRENT_USER, wstrAppKey.data()) == ERROR_SUCCESS; fOpen == fShow)
		return;

	if (fShow) {
		if (regCR.Create(HKEY_CURRENT_USER, wstrAppKey.data()) != ERROR_SUCCESS) {
			assert(true);
			return;
		}

		const auto wstrPath = ut::GetModulePath();
		regCR.SetStringValue(nullptr, L"Open in Hexer");
		regCR.SetStringValue(L"Icon", wstrPath.data());
		const auto wstrCommand = wstrAppKey + L"\\command";
		regCR.Create(HKEY_CURRENT_USER, wstrCommand.data());
		const auto wstrPathReg = std::format(L"\"{}\" \"%1\"", wstrPath);
		regCR.SetStringValue(nullptr, wstrPathReg.data());
	}
	else {
		regCR.Close();
		CRegKey regShell;
		regShell.Open(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell");
		regShell.RecurseDeleteKey(L"The Hexer");
	}
}

void CAppSettings::DBCreateTables(sqlite3* pDB)
{
	const wchar_t* sqlModulesCreate = L"CREATE TABLE IF NOT EXISTS Files ("
		L"FileID INTEGER PRIMARY KEY,"
		L"Name   TEXT NOT NULL UNIQUE);";

	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlModulesCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	const wchar_t* sqlBkmsCreate = L"CREATE TABLE IF NOT EXISTS Bookmarks ("
		L"FileID   INTEGER NOT NULL,"
		L"VecSpan  TEXT NOT NULL,"
		L"Desc     TEXT,"
		L"HEXCOLOR TEXT);";
	res = sqlite3_prepare16_v2(pDB, sqlBkmsCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	const wchar_t* sqlSettHexCtrlCreate = L"CREATE TABLE IF NOT EXISTS Settings_HexCtrl ("
		L"Name TEXT PRIMARY KEY,"
		L"Data TEXT NOT NULL);";
	res = sqlite3_prepare16_v2(pDB, sqlSettHexCtrlCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);
}

auto CAppSettings::DBGetFileIDByName(sqlite3* pDB, std::wstring_view wsvFileName)->std::int64_t
{
	const auto sqlSelect = std::format(L"Select FileID FROM Files WHERE Name = '{}'", wsvFileName);
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlSelect.data(), -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);

	if (sqlite3_step(pSTMT.get()) == SQLITE_ROW) {
		return sqlite3_column_int64(pSTMT.get(), 0);
	}

	const auto sqlInsert = std::format(L"INSERT INTO Files (Name) VALUES ('{}');", wsvFileName);
	res = sqlite3_prepare16_v2(pDB, sqlInsert.data(), -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);
	return sqlite3_last_insert_rowid(pDB);
}

auto CAppSettings::DBLoadBkmForFileID(sqlite3* pDB, std::int64_t i64FileID)->VecBkm
{
	const auto sqlSelect = std::format(L"Select * FROM Bookmarks WHERE FileID = '{}'", i64FileID);
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlSelect.data(), -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);

	VecBkm vecBkm;
	while (sqlite3_step(pSTMT.get()) == SQLITE_ROW) {
		VecSpan vecSpan;
		const std::wstring_view wsvVecSpan = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(pSTMT.get(), 1)); //VecSpan.
		std::uint64_t ullOffset { };
		for (const auto [index, digits] : wsvVecSpan | std::views::split(L',') | std::views::enumerate) {
			const auto ullCurr = stn::StrToUInt64(std::wstring_view { digits }).value_or(0ULL);
			if (index % 2 == 1) {
				vecSpan.emplace_back(HEXCTRL::HEXSPAN { .ullOffset { ullOffset }, .ullSize { ullCurr } });
			}
			else {
				ullOffset = ullCurr;
			}
		}

		const auto pwszDesc = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(pSTMT.get(), 2)); //Desc.
		const std::wstring_view wsvHEXCOLOR = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(pSTMT.get(), 3)); //HEXCOLOR.
		auto rngClr = wsvHEXCOLOR | std::views::split(L',');
		HEXCTRL::HEXCOLOR clr;
		if (auto it = rngClr.begin(); it != rngClr.end()) {
			clr.clrBk = stn::StrToUInt32(std::wstring_view { *it }).value_or(0UL);
			if (++it != rngClr.end()) {
				clr.clrText = stn::StrToUInt32(std::wstring_view { *it }).value_or(0UL);
			}
		}
		vecBkm.emplace_back(HEXCTRL::HEXBKM { .vecSpan { std::move(vecSpan) }, .wstrDesc { pwszDesc }, .stClr { clr } });
	}

	return vecBkm;
}

auto CAppSettings::DBOpenDB(const wchar_t* pwszPathDB)->uptr_sqlite
{
	sqlite3* pDB;
	auto res = sqlite3_open16(pwszPathDB, &pDB);
	assert(res == SQLITE_OK);
	return uptr_sqlite(pDB);
}

void CAppSettings::DBSaveBkmForFileID(sqlite3* pDB, std::int64_t i64FileID, SpnBkm spnBkm)
{
	//Remove all existing bookmarks for given FileID.
	const auto sqlDelete = std::format(L"DELETE FROM Bookmarks WHERE FileID = '{}'", i64FileID);
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlDelete.data(), -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	//Save current bookmarks.
	for (const auto bkm : spnBkm) {
		std::wstring wstrVecSpan;
		for (const auto vecSpan : bkm.vecSpan) {
			wstrVecSpan += std::format(L"{},{},", vecSpan.ullOffset, vecSpan.ullSize);
		}
		const auto wstrHEXCOLOR = std::format(L"{},{}", bkm.stClr.clrBk, bkm.stClr.clrText);
		const auto sqlInsert = std::format(L"INSERT INTO Bookmarks (FileID, VecSpan, Desc, HEXCOLOR)"
			L"VALUES ('{}','{}','{}','{}');", i64FileID, wstrVecSpan, bkm.wstrDesc, wstrHEXCOLOR);
		res = sqlite3_prepare16_v2(pDB, sqlInsert.data(), -1, std::out_ptr(pSTMT), nullptr);
		assert(res == SQLITE_OK);
		res = sqlite3_step(pSTMT.get());
		assert(res == SQLITE_DONE);
	}
}

void CAppSettings::DBSaveHexCtrlSettings(sqlite3* pDB, const HEXCTRLSETTINGS& sett)
{
	(void)sett.dwCapacity;
	const auto sqlHexSettInsert = L"INSERT INTO Settings_HexCtrl(Name, Data)"
		L"VALUES({HexCtrlSettName}, {SettData})"
		L"ON CONFLICT(Name)"
		L"DO UPDATE SET Data = excluded.Data";

	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlHexSettInsert, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);
}

auto CAppSettings::DWORD2PaneStatus(DWORD dw)->PANESTATUS
{
	const std::bitset<32> bsPS(dw);
	return { .fVisible { bsPS.test(0) }, .fActive { bsPS.test(1) } };
}

auto CAppSettings::PaneStatus2DWORD(PANESTATUS ps)->DWORD
{
	std::bitset<32> bs;
	bs[0] = ps.fVisible;
	bs[1] = ps.fActive;

	return bs.to_ulong();
}