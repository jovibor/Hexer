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

using uptr_stmt = std::unique_ptr < sqlite3_stmt, decltype([](sqlite3_stmt* pSTMT) {
	auto res = sqlite3_finalize(pSTMT);
	assert(res == SQLITE_OK); }) > ;
using VecDataOpen = std::vector<ut::DATAOPEN>;
using SpnDataOpen = std::span<const ut::DATAOPEN>;

//CAppSettingsRFL.
class CAppSettingsRFL final //Recent Files List.
{
public:
	CAppSettingsRFL() = default;
	void AddToList(const ut::DATAOPEN& dos);
	void ClearList();
	[[nodiscard]] auto GetDataFromMenuID(UINT uID)const -> ut::DATAOPEN;
	void Initialize(sqlite3* pDB, HMENU hMenu, int iIDMenuFirst, int iMaxEntry,
		HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess);
	[[nodiscard]] bool IsEmpty()const;
	[[nodiscard]] bool IsInitialized()const;
	void RemoveFromList(const ut::DATAOPEN& dos);
	void SetRFLSize(DWORD dwRFLSize);
	void SaveSettings(sqlite3* pDB)const;
	void UpdateMenuIcons(HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess);
	[[nodiscard]] static auto DBLoadRecentFiles(sqlite3* pDB) -> VecDataOpen;
	static void DBSaveRecentFiles(sqlite3* pDB, SpnDataOpen spnDataOpen);
private:
	void RebuildRFLMenu();
	void UpdateMenuIcons();
private:
	VecDataOpen m_vecRFL;      //Recent Files List data.
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
	assert(m_fInit);
	if (!m_fInit)
		return;

	using enum ut::EOpenMode;

	//Remove any duplicates. Processes can have same names but different IDs.
	std::erase_if(m_vecRFL, [&dos](const ut::DATAOPEN& refData) { return refData == dos; });
	const auto eOpenMode = dos.eOpenMode == NEW_FILE ? OPEN_FILE : dos.eOpenMode;
	m_vecRFL.emplace(m_vecRFL.begin(), ut::DATAOPEN {
		.wstrDataPath { dos.wstrDataPath }, .wstrFriendlyName { dos.wstrFriendlyName },
		.dwProcID { dos.dwProcID }, .eOpenMode { eOpenMode } });

	if (m_vecRFL.size() > m_dwRFLSize) {
		m_vecRFL.resize(m_dwRFLSize);
	}

	RebuildRFLMenu();
}

void CAppSettingsRFL::ClearList()
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	m_vecRFL.clear();
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

void CAppSettingsRFL::Initialize(sqlite3* pDB, HMENU hMenu, int iIDMenuFirst, int iMaxEntry,
	HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess) {
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
	m_fInit = true;
	m_vecRFL = DBLoadRecentFiles(pDB);

	if (m_vecRFL.size() > m_dwRFLSize) {
		m_vecRFL.resize(m_dwRFLSize);
	}

	RebuildRFLMenu();
}

bool CAppSettingsRFL::IsEmpty()const
{
	assert(m_fInit);
	if (!m_fInit)
		return true;

	return m_vecRFL.empty();
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

	RebuildRFLMenu();
}

void CAppSettingsRFL::SaveSettings(sqlite3* pDB)const
{
	assert(m_fInit);
	if (!m_fInit)
		return;

	DBSaveRecentFiles(pDB, m_vecRFL);
}

void CAppSettingsRFL::UpdateMenuIcons(HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess) {
	assert(m_fInit);
	if (!m_fInit)
		return;

	m_hBMPFile = hBMPFile;
	m_hBMPDevice = hBMPDevice;
	m_hBMPProcess = hBMPProcess;
	UpdateMenuIcons();
}

auto CAppSettingsRFL::DBLoadRecentFiles(sqlite3* pDB)->VecDataOpen
{
	const auto sqlSelect = L"Select * FROM RecentFiles";
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlSelect, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);

	VecDataOpen vecDataOpen;
	while (sqlite3_step(pSTMT.get()) == SQLITE_ROW) {
		const auto pwszFilePath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(pSTMT.get(), 0)); //FilePath.
		const auto dwProcID = sqlite3_column_int(pSTMT.get(), 1);  //ProcID.
		const auto eOpenMode = sqlite3_column_int(pSTMT.get(), 2); //EOpenMode.
		vecDataOpen.emplace_back(ut::DATAOPEN { .wstrDataPath { pwszFilePath },
			.dwProcID { static_cast<DWORD>(dwProcID) }, .eOpenMode { static_cast<ut::EOpenMode>(eOpenMode) } });
	}

	return vecDataOpen;
}

void CAppSettingsRFL::DBSaveRecentFiles(sqlite3* pDB, SpnDataOpen spnDataOpen)
{
	//Remove all from RecentFiles.
	const auto sqlDelete = L"DELETE FROM RecentFiles;";
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlDelete, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	//Save spnDataOpen to DB.
	for (const auto& data : spnDataOpen) {
		const auto sqlInsert = std::format(L"INSERT INTO RecentFiles (FilePath, ProcID, EOpenMode)"
			L"VALUES ('{}','{}','{}');", data.wstrDataPath, data.dwProcID, std::to_underlying(data.eOpenMode));
		res = sqlite3_prepare16_v2(pDB, sqlInsert.data(), -1, std::out_ptr(pSTMT), nullptr);
		assert(res == SQLITE_OK);
		res = sqlite3_step(pSTMT.get());
		assert(res == SQLITE_DONE);
	}
}


//CAppSettingsRFL private methods.

void CAppSettingsRFL::RebuildRFLMenu()
{
	using enum ut::EOpenMode;
	assert(m_fInit);
	if (!m_fInit)
		return;

	while (::GetMenuItemCount(m_hMenu) > 0) {
		::DeleteMenu(m_hMenu, 0, MF_BYPOSITION); //Removing all RFL menu items.
	}

	for (const auto& [idx, vec] : m_vecRFL | std::views::enumerate) {
		if (idx >= m_dwRFLSize) //Adding no more than m_dwRFLSize.
			break;

		std::wstring wstrMenu;
		if (vec.eOpenMode == OPEN_PROC) {
			wstrMenu = std::format(L"{} {}: {} (ID: {})", idx + 1, ut::GetWstrEOpenMode(vec.eOpenMode), vec.wstrDataPath,
				vec.dwProcID);
		}
		else {
			wstrMenu = std::format(L"{} {}: {}", idx + 1, ut::GetWstrEOpenMode(vec.eOpenMode), vec.wstrDataPath);
		}

		const auto uMenuID = static_cast<UINT>(m_iIDMenuFirst + idx);
		::AppendMenuW(m_hMenu, MF_STRING, uMenuID, wstrMenu.data());
	}

	::AppendMenuW(m_hMenu, MF_SEPARATOR, 0, nullptr);
	::AppendMenuW(m_hMenu, MF_STRING, IDM_FILE_RFL_CLEARLIST, L"Clear List");
	UpdateMenuIcons();
}

void CAppSettingsRFL::UpdateMenuIcons() {
	for (const auto& [idx, vec] : m_vecRFL | std::views::enumerate) {
		if (idx >= m_dwRFLSize) //Adding not more than m_dwRFLSize.
			break;

		HBITMAP hBmp { };
		switch (vec.eOpenMode) {
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
	struct DBTEMPLAPPLIED {
		ULONGLONG ullOffset { };
		std::wstring wstrTemplateName;
	};
	using VecTemplApplied = std::vector<DBTEMPLAPPLIED>;
	using uptr_sqlite = std::unique_ptr < sqlite3, decltype([](sqlite3* pDB) {
		auto res = sqlite3_close(pDB);
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
	struct ICONDATA;
	CAppSettings() = default;
	CAppSettings(const CAppSettings&) = delete;
	CAppSettings(CAppSettings&&) = delete;
	CAppSettings& operator=(const CAppSettings&) = delete;
	~CAppSettings() = default;
	[[nodiscard]] auto GetGeneralSettings() -> GENERALSETTINGS&;
	[[nodiscard]] auto GetHexCtrlSettings() -> HEXCTRLSETTINGS&;
	[[nodiscard]] auto GetHexCtrlTemplates()const -> const VecTemplates&;
	[[nodiscard]] auto GetIconDataForCmd(UINT uCMD)const -> const ICONDATA*;
	[[nodiscard]] auto GetLastOpenFiles()const -> VecDataOpen;
	[[nodiscard]] auto GetPaneData(UINT uPaneID)const -> std::uint64_t;
	[[nodiscard]] auto GetPaneStatus(UINT uPaneID)const -> PANESTATUS;
	[[nodiscard]] auto GetSavedBkms(std::wstring_view wsvFileName)const -> VecBkm;
	[[nodiscard]] auto GetSavedTemplApplied(std::wstring_view wsvFileName)const -> VecTemplApplied;
	[[nodiscard]] bool IsRFLInitialized()const;
	void LoadSettings(std::wstring_view wsvAppName);
	void OnSettingsChanged();
	void RecreateCMDIcons(int iWidth, int iHeight); //All menu and toolbar icons.
	void RFLAddToList(const ut::DATAOPEN& dos);
	void RFLClearList();
	[[nodiscard]] auto RFLGetDataFromMenuID(UINT uID)const -> ut::DATAOPEN;
	void RFLInitialize(HMENU hMenu, int iIDMenuFirst, HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess);
	[[nodiscard]] bool RFLIsEmpty()const;
	void RFLRemoveFromList(const ut::DATAOPEN& dos);
	void RFLUpdateMenuIcons(HBITMAP hBMPFile, HBITMAP hBMPDevice, HBITMAP hBMPProcess);
	void SaveBkms(std::wstring_view wsvFileName, HEXCTRL::SpanHexBkm spnBkm);
	void SaveLastOpenFiles(SpnDataOpen spnDataOpen);
	void SaveSettings();
	void SaveTemplApplied(std::wstring_view wsvFileName, HEXCTRL::SpanHexTemplApplied spnApplied);
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
	[[nodiscard]] auto GetRegSettingsPath()const -> const std::wstring&;
	[[nodiscard]] auto GetSQLiteDB()const -> sqlite3*;
	void LoadHexCtrlTemplates();
	void ShowInWindowsContextMenu(bool fShow);
	static void DBCreateTables(sqlite3* pDB);
	[[nodiscard]] static auto DBGetFileIDByName(sqlite3* pDB, std::wstring_view wsvFileName) -> std::int64_t;
	[[nodiscard]] static auto DBLoadBkmForFileID(sqlite3* pDB, std::int64_t i64FileID) -> VecBkm;
	[[nodiscard]] static auto DBLoadLastOpenFiles(sqlite3* pDB) -> VecDataOpen;
	[[nodiscard]] static auto DBLoadTemplApliedForFileID(sqlite3* pDB, std::int64_t i64FileID) -> VecTemplApplied;
	[[nodiscard]] static auto DBOpenDB(const wchar_t* pwszPathDB) -> uptr_sqlite;
	static void DBSaveBkmForFileID(sqlite3* pDB, std::int64_t i64FileID, HEXCTRL::SpanHexBkm spnHexBkm);
	static void DBSaveHexCtrlSettings(sqlite3* pDB, const HEXCTRLSETTINGS& sett);
	static void DBSaveLastOpenFiles(sqlite3* pDB, SpnDataOpen spnDataOpen);
	static void DBSaveTemplAppliedForFileID(sqlite3* pDB, std::int64_t i64FileID, HEXCTRL::SpanHexTemplApplied spnApplied);
	[[nodiscard]] static auto DWORD2PaneStatus(DWORD dw) -> PANESTATUS;
	[[nodiscard]] static auto PaneStatus2DWORD(PANESTATUS ps) -> DWORD;
private:
	static std::vector<ICONDATA> m_vecIconData;
	CAppSettingsRFL m_stRFL;
	PANESETTINGS m_stPaneSett;          //"Panes" settings data.
	GENERALSETTINGS m_stGeneralSett;    //"General" settings data.
	HEXCTRLSETTINGS m_stHexCtrlSett;    //"HexCtrl" settings data.
	VecTemplates m_vecHexCtrlTemplates; //HexCtrl loaded templates.
	std::wstring m_wstrAppName;         //Application name for registry paths.
	uptr_sqlite m_upSQLiteDB;           //SQLite database pointer.
	bool m_fLoaded { false };           //LoadSettings has succeeded.
};

struct CAppSettings::ICONDATA {
	UINT uIDCmd { };
	UINT uIDSVG { };
	HBITMAP hBmp { };
	HICON hIcon { };
	void Clear()const { ::DeleteObject(hBmp); ::DestroyIcon(hIcon); }
};

std::vector<CAppSettings::ICONDATA> CAppSettings::m_vecIconData {
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


auto CAppSettings::GetGeneralSettings()->GENERALSETTINGS&
{
	return m_stGeneralSett;
}

auto CAppSettings::GetHexCtrlSettings()->HEXCTRLSETTINGS&
{
	return m_stHexCtrlSett;
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

auto CAppSettings::GetLastOpenFiles()const->VecDataOpen
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	return DBLoadLastOpenFiles(GetSQLiteDB());
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

auto CAppSettings::GetSavedBkms(std::wstring_view wsvFileName)const->VecBkm
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	const auto pDB = GetSQLiteDB();
	return DBLoadBkmForFileID(pDB, DBGetFileIDByName(pDB, wsvFileName));
}

auto CAppSettings::GetSavedTemplApplied(std::wstring_view wsvFileName)const->VecTemplApplied
{
	assert(m_fLoaded);
	if (!m_fLoaded)
		return { };

	const auto pDB = GetSQLiteDB();
	return DBLoadTemplApliedForFileID(pDB, DBGetFileIDByName(pDB, wsvFileName));
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
		::CreateDirectoryW(ut::GetAppDataDir().data(), nullptr);
		m_upSQLiteDB = DBOpenDB(ut::GetSQLiteDBPath().data());
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

void CAppSettings::OnSettingsChanged()
{
	ShowInWindowsContextMenu(m_stGeneralSett.fWindowsMenu);
	m_stRFL.SetRFLSize(m_stGeneralSett.dwRFLSize);
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

void CAppSettings::RFLClearList()
{
	m_stRFL.ClearList();
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

	m_stRFL.Initialize(GetSQLiteDB(), hMenu, iIDMenuFirst, GetGeneralSettings().dwRFLSize,
		hBMPFile, hBMPDevice, hBMPProcess);
}

bool CAppSettings::RFLIsEmpty()const
{
	return m_stRFL.IsEmpty();
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

void CAppSettings::SaveBkms(std::wstring_view wsvFileName, HEXCTRL::SpanHexBkm spnBkm)
{
	const auto pDB = GetSQLiteDB();
	const auto i64FileID = DBGetFileIDByName(pDB, wsvFileName);
	DBSaveBkmForFileID(pDB, i64FileID, spnBkm);
}

void CAppSettings::SaveLastOpenFiles(SpnDataOpen spnDataOpen)
{
	const auto pDB = GetSQLiteDB();
	DBSaveLastOpenFiles(pDB, spnDataOpen);
}

void CAppSettings::SaveSettings()
{
	//Recent Files List.
	m_stRFL.SaveSettings(GetSQLiteDB());

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

void CAppSettings::SaveTemplApplied(std::wstring_view wsvFileName, HEXCTRL::SpanHexTemplApplied spnApplied)
{
	const auto pDB = GetSQLiteDB();
	const auto i64FileID = DBGetFileIDByName(pDB, wsvFileName);
	DBSaveTemplAppliedForFileID(pDB, i64FileID, spnApplied);
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
	return m_stPaneSett;
}

auto CAppSettings::GetPanesSettings()const->const PANESETTINGS& {
	return m_stPaneSett;
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
	const auto sqlModulesCreate = L"CREATE TABLE IF NOT EXISTS Files ("
		L"FileID INTEGER PRIMARY KEY,"
		L"Name   TEXT NOT NULL UNIQUE);";

	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlModulesCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	const auto sqlBkmsCreate = L"CREATE TABLE IF NOT EXISTS Bookmarks ("
		L"FileID   INTEGER NOT NULL,"
		L"VecSpan  TEXT NOT NULL,"
		L"Desc     TEXT,"
		L"HEXCOLOR INTEGER NOT NULL);";
	res = sqlite3_prepare16_v2(pDB, sqlBkmsCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	const auto sqlTemplAppliedCreate = L"CREATE TABLE IF NOT EXISTS TemplatesApplied ("
		L"FileID       INTEGER NOT NULL,"
		L"TemplateName TEXT NOT NULL,"
		L"Offset       INTEGER NOT NULL);";
	res = sqlite3_prepare16_v2(pDB, sqlTemplAppliedCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	const auto sqlSettGeneralCreate = L"CREATE TABLE IF NOT EXISTS SettingsGeneral ("
		L"Name TEXT PRIMARY KEY,"
		L"Data TEXT NOT NULL);";
	res = sqlite3_prepare16_v2(pDB, sqlSettGeneralCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	const auto sqlSettHexCtrlCreate = L"CREATE TABLE IF NOT EXISTS SettingsHexCtrl ("
		L"Name TEXT PRIMARY KEY,"
		L"Data TEXT NOT NULL);";
	res = sqlite3_prepare16_v2(pDB, sqlSettHexCtrlCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	const auto sqlLOFCreate = L"CREATE TABLE IF NOT EXISTS LastOpenFiles ("
		L"FilePath  TEXT NOT NULL,"
		L"ProcID    INTEGER,"
		L"EOpenMode INTEGER NOT NULL);"; //EOpenMode enum data.
	res = sqlite3_prepare16_v2(pDB, sqlLOFCreate, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	const auto sqlRFCreate = L"CREATE TABLE IF NOT EXISTS RecentFiles ("
		L"FilePath  TEXT NOT NULL,"
		L"ProcID    INTEGER,"
		L"EOpenMode INTEGER NOT NULL);"; //EOpenMode enum data.
	res = sqlite3_prepare16_v2(pDB, sqlRFCreate, -1, std::out_ptr(pSTMT), nullptr);
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
		HEXCTRL::VecHexSpan vecSpan;
		const std::wstring_view wsvVecSpan = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(pSTMT.get(), 1)); //VecHexSpan.
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
		const std::uint64_t u64Clr = sqlite3_column_int64(pSTMT.get(), 3);
		const HEXCTRL::HEXCOLOR clr { .clrBk { static_cast<DWORD>((u64Clr >> 32) & 0xFFFFFFFFU) },
			.clrText { static_cast<DWORD>(u64Clr & 0xFFFFFFFFU) } };
		vecBkm.emplace_back(HEXCTRL::HEXBKM { .vecSpan { std::move(vecSpan) }, .wstrDesc { pwszDesc }, .stClr { clr } });
	}

	return vecBkm;
}

auto CAppSettings::DBLoadLastOpenFiles(sqlite3* pDB)->VecDataOpen
{
	const auto sqlSelect = L"Select * FROM LastOpenFiles";
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlSelect, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);

	VecDataOpen vecDataOpen;
	while (sqlite3_step(pSTMT.get()) == SQLITE_ROW) {
		const auto pwszFilePath = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(pSTMT.get(), 0)); //FilePath.
		const auto dwProcID = sqlite3_column_int(pSTMT.get(), 1);  //ProcID.
		const auto eOpenMode = sqlite3_column_int(pSTMT.get(), 2); //EOpenMode.
		vecDataOpen.emplace_back(ut::DATAOPEN { .wstrDataPath { pwszFilePath },
			.dwProcID { static_cast<DWORD>(dwProcID) }, .eOpenMode { static_cast<ut::EOpenMode>(eOpenMode) } });
	}

	return vecDataOpen;
}

auto CAppSettings::DBLoadTemplApliedForFileID(sqlite3* pDB, std::int64_t i64FileID)->VecTemplApplied
{
	const auto sqlSelect = std::format(L"Select * FROM TemplatesApplied WHERE FileID = '{}'", i64FileID);
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlSelect.data(), -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);

	VecTemplApplied vecApplied;
	while (sqlite3_step(pSTMT.get()) == SQLITE_ROW) {
		const auto pwszTemplName = reinterpret_cast<const wchar_t*>(sqlite3_column_text16(pSTMT.get(), 1)); //Template name.
		const auto u64Offset = static_cast<std::uint64_t>(sqlite3_column_int64(pSTMT.get(), 2)); //Offset.
		vecApplied.emplace_back(DBTEMPLAPPLIED { .ullOffset { u64Offset }, .wstrTemplateName { pwszTemplName } });
	}

	return vecApplied;
}

auto CAppSettings::DBOpenDB(const wchar_t* pwszPathDB)->uptr_sqlite
{
	sqlite3* pDB;
	auto res = sqlite3_open16(pwszPathDB, &pDB);
	assert(res == SQLITE_OK);
	return uptr_sqlite(pDB);
}

void CAppSettings::DBSaveBkmForFileID(sqlite3* pDB, std::int64_t i64FileID, HEXCTRL::SpanHexBkm spnBkm)
{
	//Remove all existing records for given FileID.
	const auto sqlDelete = std::format(L"DELETE FROM Bookmarks WHERE FileID = '{}'", i64FileID);
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlDelete.data(), -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	for (const auto& bkm : spnBkm) {
		std::wstring wstrVecSpan;
		for (const auto& vecSpan : bkm.vecSpan) {
			wstrVecSpan += std::format(L"{},{},", vecSpan.ullOffset, vecSpan.ullSize);
		}

		//Combine two HEXCOLOR DWORDs into one std::uint64_t, for storing.
		const std::uint64_t u64Clr = static_cast<std::uint64_t>(bkm.stClr.clrBk) << 32 | bkm.stClr.clrText;
		const auto sqlInsert = std::format(L"INSERT INTO Bookmarks (FileID, VecSpan, Desc, HEXCOLOR)"
			L"VALUES ('{}','{}','{}','{}');", i64FileID, wstrVecSpan, bkm.wstrDesc, u64Clr);
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

void CAppSettings::DBSaveLastOpenFiles(sqlite3* pDB, SpnDataOpen spnDataOpen)
{
	//Remove all from LastOpenFiles.
	const auto sqlDelete = L"DELETE FROM LastOpenFiles;";
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlDelete, -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	//Save spnDataOpen to DB.
	for (const auto& data : spnDataOpen) {
		const auto sqlInsert = std::format(L"INSERT INTO LastOpenFiles (FilePath, ProcID, EOpenMode)"
			L"VALUES ('{}','{}','{}');", data.wstrDataPath, data.dwProcID, std::to_underlying(data.eOpenMode));
		res = sqlite3_prepare16_v2(pDB, sqlInsert.data(), -1, std::out_ptr(pSTMT), nullptr);
		assert(res == SQLITE_OK);
		res = sqlite3_step(pSTMT.get());
		assert(res == SQLITE_DONE);
	}
}

void CAppSettings::DBSaveTemplAppliedForFileID(sqlite3* pDB, std::int64_t i64FileID, HEXCTRL::SpanHexTemplApplied spnApplied)
{
	//Remove all existing records for given FileID.
	const auto sqlDelete = std::format(L"DELETE FROM TemplatesApplied WHERE FileID = '{}'", i64FileID);
	uptr_stmt pSTMT;
	auto res = sqlite3_prepare16_v2(pDB, sqlDelete.data(), -1, std::out_ptr(pSTMT), nullptr);
	assert(res == SQLITE_OK);
	res = sqlite3_step(pSTMT.get());
	assert(res == SQLITE_DONE);

	for (const auto& applied : spnApplied) {
		const auto sqlInsert = std::format(L"INSERT INTO TemplatesApplied (FileID, TemplateName, Offset)"
			L"VALUES ('{}','{}','{}');", i64FileID, applied.pTemplate->wstrName, applied.ullOffset);
		res = sqlite3_prepare16_v2(pDB, sqlInsert.data(), -1, std::out_ptr(pSTMT), nullptr);
		assert(res == SQLITE_OK);
		res = sqlite3_step(pSTMT.get());
		assert(res == SQLITE_DONE);
	}
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