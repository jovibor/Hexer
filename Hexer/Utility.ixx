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
#include <chrono>
#include <optional>
#include <string>
export module Utility;

export namespace Ut {
	constexpr auto HEXER_VERSION_MAJOR = 0;
	constexpr auto HEXER_VERSION_MINOR = 9;
	constexpr auto HEXER_VERSION_PATCH = 5;

	constexpr UINT g_arrPanes[] { IDC_PANE_FILEINFO, IDC_PANE_BKMMGR, IDC_PANE_DATAINTERP, IDC_PANE_TEMPLMGR, IDC_PANE_LOGINFO };

	[[nodiscard]] constexpr auto GetPaneIDFromMenuID(UINT uMenuID) -> UINT {
		switch (uMenuID) {
		case IDM_VIEW_FILEPROPS:
			return IDC_PANE_FILEINFO;
		case IDM_VIEW_BKMMGR:
			return IDC_PANE_BKMMGR;
		case IDM_VIEW_DATAINTERP:
			return IDC_PANE_DATAINTERP;
		case IDM_VIEW_TEMPLMGR:
			return IDC_PANE_TEMPLMGR;
		case IDM_VIEW_LOGINFO:
			return IDC_PANE_LOGINFO;
		default:
			return { };
		};
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

	struct HIDPIINFO {
		int   iLOGPIXELSY { };
		float flDPIScale { };
	};

	struct FILEOPEN {
		std::wstring  wstrFilePath;
		std::uint64_t ullFileSize { };
		bool          fNewFile { };
	};

	struct FILEINFO {
		std::wstring_view wsvFilePath { };
		std::wstring_view wsvFileName { };
		std::uint64_t     ullFileSize { };
		std::uint32_t     dwPageSize { };
		bool              fMutable { };
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

	[[nodiscard]] auto GetAppName() -> const std::wstring& {
		static const std::wstring wstrAppName { [] {
			CStringW str;
			str.LoadStringW(IDR_HEXER_FRAME);
			return str; }() };
		return wstrAppName;
	}

	constexpr auto WM_ADDLOGENTRY { WM_APP + 1 }; //Custom message.

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
			AfxGetMainWnd()->SendMessageW(WM_ADDLOGENTRY, 0, reinterpret_cast<WPARAM>(&li));
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