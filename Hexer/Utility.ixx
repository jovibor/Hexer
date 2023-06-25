module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "resource.h"
#include <SDKDDKVer.h>
#include "HexCtrl.h"
#include <afxwin.h>
#include <optional>
#include <string>
#include <unordered_map>
export module Utility;

export namespace Utility
{
	constexpr auto HEXER_VERSION_MAJOR = 0;
	constexpr auto HEXER_VERSION_MINOR = 9;
	constexpr auto HEXER_VERSION_PATCH = 1;
	constexpr wchar_t g_wstrAppName[] { L"Hexer" };
	constexpr UINT g_arrPanes[] { IDC_PANE_FILEPROPS, IDC_PANE_DATAINTERP, IDC_PANE_TEMPLMGR };

	[[nodiscard]] constexpr auto PaneIDToEHexWnd(UINT uPaneID) -> std::optional<HEXCTRL::EHexWnd> {
		switch (uPaneID) {
		case IDC_PANE_DATAINTERP:
			return HEXCTRL::EHexWnd::DLG_DATAINTERP;
		case IDC_PANE_TEMPLMGR:
			return HEXCTRL::EHexWnd::DLG_TEMPLMGR;
		default:
			return std::nullopt;
		}
	}

	struct FILEPROPS {
		std::wstring_view wsvFilePath { };
		std::wstring_view wsvFileName { };
		std::uint64_t     ullFileSize { };
		std::uint32_t     dwPageSize { };
		bool              fWritable { };
	};

	enum class EBusType :std::uint16_t {
		TYPE_Unknown = 0, TYPE_SCSI = 1, TYPE_ATAPI = 2, TYPE_ATA = 3, TYPE_1394 = 4, TYPE_SSA = 5, TYPE_Fibre_Channel = 6,
		TYPE_USB = 7, RAID = 8, TYPE_iSCSI = 9, TYPE_SAS = 10, TYPE_SATA = 11, TYPE_SD = 12, TYPE_MMC = 13,
		TYPE_Virtual = 14, TYPE_File_Backed_Virtual = 15, TYPE_Storage_Spaces = 16, TYPE_NVMe = 17, TYPE_Reserved = 18
	};
	const std::unordered_map<EBusType, std::wstring_view> g_mapBusType{
		{ EBusType::TYPE_Unknown, L"Unknown"}, { EBusType::TYPE_SCSI, L"SCSI" },
		{ EBusType::TYPE_ATAPI, L"ATAPI" }, { EBusType::TYPE_ATA, L"ATA" },
		{ EBusType::TYPE_1394, L"1394" }, { EBusType::TYPE_SSA, L"SSA" },
		{ EBusType::TYPE_Fibre_Channel, L"Fibre Channel" }, { EBusType::TYPE_USB, L"USB" },
		{ EBusType::RAID, L"RAID" }, { EBusType::TYPE_iSCSI, L"iSCSI" },
		{ EBusType::TYPE_SAS, L"SAS" }, { EBusType::TYPE_SATA, L"SATA" },
		{ EBusType::TYPE_SD, L"SD" }, { EBusType::TYPE_MMC, L"MMC" },
		{ EBusType::TYPE_Virtual, L"Virtual" }, { EBusType::TYPE_File_Backed_Virtual, L"File Backed Virtual" },
		{ EBusType::TYPE_Storage_Spaces, L"Storage Spaces" }, { EBusType::TYPE_NVMe, L"NVMe" },
		{ EBusType::TYPE_Reserved, L"Reserved" }
	};

	enum class EMediaType :std::uint16_t {
		TYPE_Unspecified = 0, TYPE_HDD = 3, TYPE_SSD = 4, TYPE_SCM = 5
	};
	const std::unordered_map<EMediaType, std::wstring_view> g_mapMediaType {
		{ EMediaType::TYPE_Unspecified, L"Unspecified"}, { EMediaType::TYPE_HDD, L"HDD" },
		{ EMediaType::TYPE_SSD, L"SSD" }, { EMediaType::TYPE_SCM, L"SCM" }
	};

	struct PHYSICALDISK {
		std::wstring  wstrFriendlyName;
		std::wstring  wstrPath;
		std::uint64_t ullSize { };
		EBusType      eBusType { };
		EMediaType    eMediaType { };
	};

	struct VOLUME {
		std::wstring  wstrDriveLetter;
		std::wstring  wstrPath;
		std::wstring  wstrFileSystem;
		std::wstring  wstrFileSystemLabel;
		std::wstring  wstrDriveType;
		std::uint64_t ullSize { };
	};

	struct HIDPIINFO {
		int   iLOGPIXELSY { };
		float flDPIScale { };
	};

	struct FILEOPEN {
		std::wstring  wstrFilePath;
		std::uint64_t ullFileSize { };
		bool          fNewFile { };
	};

	[[nodiscard]] auto GetHiDPIInfo() -> HIDPIINFO {
		static const HIDPIINFO ret { []()->HIDPIINFO {
			const auto hDC = ::GetDC(nullptr);
			const auto iLOGPIXELSY = GetDeviceCaps(hDC, LOGPIXELSY);
			const auto flScale = iLOGPIXELSY / 96.0F;
			::ReleaseDC(nullptr, hDC);
			return { .iLOGPIXELSY { iLOGPIXELSY }, .flDPIScale{ flScale } };
		}() };

		return ret;
	}
}