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
#include <chrono>
#include <optional>
#include <string>
export module Utility;

export import StrToNum;
export namespace stn = HEXCTRL::stn;
export import ListEx;
export namespace lex = HEXCTRL::LISTEX;
static HWND g_hWndMain { };

export namespace Ut {
	constexpr auto HEXER_VERSION_MAJOR = 1;
	constexpr auto HEXER_VERSION_MINOR = 3;
	constexpr auto HEXER_VERSION_PATCH = 0;

	constexpr UINT g_arrPanes[] { IDC_PANE_DATAINFO, IDC_PANE_BKMMGR, IDC_PANE_DATAINTERP, IDC_PANE_TEMPLMGR, IDC_PANE_LOGGER };

	//AfxGetMainWnd() doesn't return correct handle in some cases.
	//For instance, when open process and trying filling its whole memory with any data.
	void SetMainWnd(HWND hWnd) {
		g_hWndMain = hWnd;
	}

	[[nodiscard]] auto GetMainWnd() -> HWND {
		return g_hWndMain;
	}

	[[nodiscard]] auto GetAppName() -> const std::wstring& {
		static const std::wstring wstrAppName { [] {
			CStringW strw;
			strw.LoadStringW(IDR_HEXER_FRAME);
			return strw; }() };
		return wstrAppName;
	}

	[[nodiscard]] constexpr auto GetEHexWndFromPaneID(UINT uPaneID) -> std::optional<HEXCTRL::EHexWnd> {
		switch (uPaneID) {
		case IDC_PANE_BKMMGR:
			return HEXCTRL::EHexWnd::DLG_BKMMGR;
		case IDC_PANE_DATAINTERP:
			return HEXCTRL::EHexWnd::DLG_DATAINTERP;
		case IDC_PANE_TEMPLMGR:
			return HEXCTRL::EHexWnd::DLG_TEMPLMGR;
		default:
			return std::nullopt;
		}
	}

	[[nodiscard]] constexpr auto GetPaneIDFromMenuID(UINT uMenuID) -> UINT {
		switch (uMenuID) {
		case IDM_VIEW_DATAINFO:
			return IDC_PANE_DATAINFO;
		case IDM_VIEW_BKMMGR:
			return IDC_PANE_BKMMGR;
		case IDM_VIEW_DATAINTERP:
			return IDC_PANE_DATAINTERP;
		case IDM_VIEW_TEMPLMGR:
			return IDC_PANE_TEMPLMGR;
		case IDM_VIEW_LOGGER:
			return IDC_PANE_LOGGER;
		default:
			return { };
		};
	}

	[[nodiscard]] auto GetLastErrorWstr(DWORD dwErr = 0) -> std::wstring {
		std::wstring wstr;
		wstr.resize_and_overwrite(MAX_PATH, [=](wchar_t* pData, std::size_t sSize)noexcept {
			return FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
				dwErr > 0 ? dwErr : GetLastError(), 0, pData, static_cast<DWORD>(sSize), nullptr);
			});
		return wstr;
	}

	struct HIDPIINFO {
		int   iLOGPIXELSY { };
		float flDPIScale { };
	};

	[[nodiscard]] auto GetHiDPIInfo() -> HIDPIINFO {
		static const HIDPIINFO ret { []()->HIDPIINFO {
			const auto hDC = ::GetDC(nullptr);
			const auto iLOGPIXELSY = GetDeviceCaps(hDC, LOGPIXELSY);
			const auto flScale = iLOGPIXELSY / 96.0F;
			::ReleaseDC(nullptr, hDC);
			return { .iLOGPIXELSY { iLOGPIXELSY }, .flDPIScale { flScale } };
			}() };

		return ret;
	}

	enum class EOpenMode : std::uint8_t {
		OPEN_FILE, OPEN_DEVICE, OPEN_PROC, NEW_FILE
	};

	enum class EDataAccessMode : std::uint8_t {
		DA_RO, DA_RW_DEFAULT, DA_RW_INPLACE
	};

	enum class EDataIOMode : std::uint8_t {
		DATA_MMAP, DATA_IOBUFF, DATA_IOIMMEDIATE
	};

	[[nodiscard]] auto GetWstrEOpenMode(EOpenMode eOpenMode) -> std::wstring_view {
		using enum EOpenMode;
		switch (eOpenMode) {
		case OPEN_DEVICE:
			return L"Device";
		case OPEN_PROC:
			return L"Process";
		case OPEN_FILE:
		case NEW_FILE:
			return L"File";
		default:
			return L"";
		};
	}

	[[nodiscard]] auto GetWstrEDataAccessMode(EDataAccessMode eDAMode) -> std::wstring_view {
		using enum EDataAccessMode;
		switch (eDAMode) {
		case DA_RO:
			return L"Read Only";
		case DA_RW_DEFAULT:
			return L"Read/Write Default";
		case DA_RW_INPLACE:
			return L"Read/Write In-Place";
		default:
			return L"";
		}
	}

	[[nodiscard]] auto GetWstrEDataIOMode(EDataIOMode eDataIOMode) -> std::wstring_view {
		using enum EDataIOMode;
		switch (eDataIOMode) {
		case DATA_MMAP:
			return L"Memory Mapped File";
		case DATA_IOBUFF:
			return L"Data IO Buffered";
		case DATA_IOIMMEDIATE:
			return L"Data IO Immediate";
		default:
			return L"";
		}
	}

	[[nodiscard]] auto ResolveLNK(const wchar_t* pwszPath) -> std::wstring {
		std::wstring_view wsv = pwszPath;
		if (!wsv.ends_with(L".lnk") && !wsv.ends_with(L".LNK")) {
			return pwszPath;
		}

		CComPtr<IShellLinkW> pIShellLinkW;
		pIShellLinkW.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
		CComPtr<IPersistFile> pIPersistFile;
		pIShellLinkW->QueryInterface(IID_PPV_ARGS(&pIPersistFile));
		pIPersistFile->Load(pwszPath, STGM_READ);

		std::wstring wstrPath;
		wstrPath.resize_and_overwrite(MAX_PATH, [pIShellLinkW = pIShellLinkW](wchar_t* pData, std::size_t sSize)noexcept {
			pIShellLinkW->GetPath(pData, static_cast<int>(sSize), nullptr, 0);
			return sSize; });
		wstrPath.resize(wstrPath.find_first_of(L'\0')); //Resize to the actual data size.

		return wstrPath;
	};

	[[nodiscard]] auto GetHBITMAP(int iResID) -> HBITMAP {
		const auto iSizeIcon = static_cast<int>(16 * Ut::GetHiDPIInfo().flDPIScale);
		return static_cast<HBITMAP>(LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(iResID),
			IMAGE_BITMAP, iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));
	}

	[[nodiscard]] auto HICONfromHBITMAP(HBITMAP hbmp) -> HICON {
		BITMAP stBmp;
		if (!GetObjectW(hbmp, sizeof(BITMAP), &stBmp))
			return { };

		const auto hbmpMask = CreateCompatibleBitmap(::GetDC(nullptr), stBmp.bmWidth, stBmp.bmHeight);
		ICONINFO ii { .fIcon { TRUE }, .hbmMask { hbmpMask }, .hbmColor { hbmp } };
		auto hICO = CreateIconIndirect(&ii);
		DeleteObject(hbmpMask);

		return hICO;
	}

	struct DATAOPEN { //Main data opening struct.
		std::wstring  wstrDataPath; //Or Process name.
		std::uint64_t ullSizeNewFile { };
		DWORD         dwProcID { };
		EOpenMode     eOpenMode { EOpenMode::OPEN_FILE };
		[[nodiscard]] bool operator==(const DATAOPEN& rhs)const {
			return wstrDataPath == rhs.wstrDataPath && dwProcID == rhs.dwProcID;
		};
	};

	struct DATAINFO { //Data for the CDlgLogger dialog.
		std::wstring_view wsvDataPath { };
		std::wstring_view wsvFileName { };
		std::uint64_t     ullDataSize { };
		std::uint32_t     dwPageSize { };
		EOpenMode         eOpenMode { };
		EDataAccessMode   eDataAccessMode { };
		EDataIOMode       eDataIOMode { };
	};

	//Custom messages.
	constexpr auto WM_ADD_LOG_ENTRY { WM_APP + 1 };
	constexpr auto WM_APP_SETTINGS_CHANGED { WM_APP + 2 };

	//Flags for the HexCtrl internal dialogs.
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_MIN { 0x1ULL };   //Template Manager.
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_HEX { 0x2ULL };
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_TT { 0x4ULL };
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_HGL { 0x8ULL };
	constexpr auto HEXCTRL_FLAG_TEMPLMGR_SWAP { 0x10ULL };
	constexpr auto HEXCTRL_FLAG_DATAINTERP_HEX { 0x1ULL }; //Data Interpreter.
	constexpr auto HEXCTRL_FLAG_DATAINTERP_BE { 0x2ULL };
	constexpr auto HEXCTRL_FLAG_BKMMGR_HEX { 0x1ULL };	   //Bookmark Manager.


	namespace Log {
		enum class EMsgType :std::int8_t { //Enum id-number is the icon's index in the image list.
			Unknown = -1, msg_error = 0, msg_warning = 1, msg_info = 2
		};

		using local_time = std::chrono::local_time<std::chrono::system_clock::duration>;
		struct LOGINFO { //Struct for providing and transferring log data.
			local_time        tmloc { std::chrono::current_zone()->to_local(std::chrono::system_clock::now()) };
			std::wstring_view wsvMsg;
			EMsgType          eType { };
		};

		void AddLogEntry(const LOGINFO& li) {
			SendMessageW(GetMainWnd(), WM_ADD_LOG_ENTRY, 0, reinterpret_cast<WPARAM>(&li));
		}

		void AddLogEntryError(std::wstring_view wsvMsg) {
			AddLogEntry({ .wsvMsg = wsvMsg, .eType = EMsgType::msg_error });
		}

		void AddLogEntryWarning(std::wstring_view wsvMsg) {
			AddLogEntry({ .wsvMsg = wsvMsg, .eType = EMsgType::msg_warning });
		}

		void AddLogEntryInfo(std::wstring_view wsvMsg) {
			AddLogEntry({ .wsvMsg = wsvMsg, .eType = EMsgType::msg_info });
		}
	}
}