/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
module;
#include <SDKDDKVer.h>
#include "resource.h"
#include <afxwin.h>
#include <Shlobj.h>
#include <d2d1_3.h>
#include <winioctl.h>
#include "HexCtrl.h"
#include <cassert>
#include <compare>
#include <chrono>
#include <expected>
#include <memory>
#include <string>
export module Utility;

#pragma comment(lib, "d2d1")

export import StrToNum;
export import ListEx;
export namespace lex = LISTEX;
HWND g_hWndMain { };

export namespace ut {
	constexpr auto HEXER_VERSION_MAJOR = 1;
	constexpr auto HEXER_VERSION_MINOR = 4;
	constexpr auto HEXER_VERSION_PATCH = 0;

	constexpr UINT g_arrPanes[] { IDC_PANE_DATAINFO, IDC_PANE_BKMMGR, IDC_PANE_DATAINTERP, IDC_PANE_TEMPLMGR, IDC_PANE_LOGGER };

	template<typename TCom> requires requires(TCom* pTCom) { pTCom->AddRef(); pTCom->Release(); }
	class comptr {
	public:
		comptr() = default;
		comptr(TCom* pTCom) : m_pTCom(pTCom) { }
		comptr(const comptr<TCom>& rhs) : m_pTCom(rhs.get()) { safe_addref(); }
		~comptr() { safe_release(); }
		operator TCom*()const { return get(); }
		operator TCom**() { return get_addr(); }
		operator IUnknown**() { return reinterpret_cast<IUnknown**>(get_addr()); }
		operator void**() { return reinterpret_cast<void**>(get_addr()); }
		auto operator->()const->TCom* { return get(); }
		auto operator=(const comptr<TCom>& rhs)->comptr& {
			if (this != &rhs) {
				safe_release();	m_pTCom = rhs.get(); safe_addref();
			}
			return *this;
		}
		auto operator=(TCom* pRHS)->comptr& {
			if (get() != pRHS) {
				if (get() != nullptr) { get()->Release(); }
				m_pTCom = pRHS;
			}
			return *this;
		}
		[[nodiscard]] bool operator==(const comptr<TCom>& rhs)const { return get() == rhs.get(); }
		[[nodiscard]] bool operator==(const TCom* pRHS)const { return get() == pRHS; }
		[[nodiscard]] explicit operator bool() { return get() != nullptr; }
		[[nodiscard]] explicit operator bool()const { return get() != nullptr; }
		[[nodiscard]] auto get()const -> TCom* { return m_pTCom; }
		[[nodiscard]] auto get_addr() -> TCom** { return &m_pTCom; }
		void safe_release() { if (get() != nullptr) { get()->Release(); m_pTCom = nullptr; } }
		void safe_addref() { if (get() != nullptr) { get()->AddRef(); } }
	private:
		TCom* m_pTCom { };
	};

	[[nodiscard]] auto SVGToBmp(IStream* pStream, int iWidth, int iHeight, ID2D1Factory* pD2DFactory = nullptr) -> HBITMAP {
		//The "height" and "width" svg root attributes <svg ...height="30" width="30"...> must be removed from the svg file,
		//to scale image correctly with the Direct2D.
		//Otherwise, ID2D1SvgDocument will use these attributes for scaling, not its own viewport size.

		if (pD2DFactory == nullptr) {
			static const comptr pD2DFactory1 = []() {
				ID2D1Factory1* pFactory1;
				::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1),
					reinterpret_cast<void**>(&pFactory1));
				assert(pFactory1 != nullptr);
				return pFactory1;
				}();
			pD2DFactory = pD2DFactory1;
		}

		const auto d2d1RTP = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0, 0, D2D1_RENDER_TARGET_USAGE_NONE,
			D2D1_FEATURE_LEVEL_DEFAULT);
		comptr<ID2D1DCRenderTarget> pDCRT;
		pD2DFactory->CreateDCRenderTarget(&d2d1RTP, pDCRT);
		assert(pDCRT != nullptr);
		if (pDCRT == nullptr)
			return { };

		std::unique_ptr < std::remove_pointer_t<HDC>, decltype([](HDC hDC) { ::DeleteDC(hDC); }) > pHDC { ::CreateCompatibleDC(nullptr) };
		assert(pHDC != nullptr);
		if (pHDC == nullptr)
			return { };

		const BITMAPINFO bi { .bmiHeader { .biSize { sizeof(BITMAPINFOHEADER) },
			.biWidth { iWidth }, .biHeight { iHeight }, .biPlanes { 1 }, .biBitCount { 32 }, .biCompression { BI_RGB } } };
		void* pBitsBmp;
		const auto hBmp = ::CreateDIBSection(pHDC.get(), &bi, DIB_RGB_COLORS, &pBitsBmp, nullptr, 0);
		assert(hBmp != nullptr);
		if (hBmp == nullptr)
			return { };

		::SelectObject(pHDC.get(), hBmp);
		const RECT rc { .right { iWidth }, .bottom { iHeight } };
		pDCRT->BindDC(pHDC.get(), &rc);

		comptr<ID2D1DeviceContext5> pD2DDC5;
		pDCRT->QueryInterface(__uuidof(ID2D1DeviceContext5), pD2DDC5);
		assert(pD2DDC5 != nullptr);
		if (pD2DDC5 == nullptr)
			return { };

		comptr<ID2D1SvgDocument> pSVGDoc;
		pD2DDC5->CreateSvgDocument(pStream, { .width { static_cast<float>(iWidth) },
			.height { static_cast<float>(iHeight) } }, pSVGDoc);
		assert(pSVGDoc != nullptr);
		if (pSVGDoc == nullptr)
			return { };

		pDCRT->BeginDraw();
		pD2DDC5->DrawSvgDocument(pSVGDoc);
		pDCRT->EndDraw();

		return hBmp;
	}

	[[nodiscard]] auto SVGToBmp(UINT uIDRes, int iWidth, int iHeight, HINSTANCE hInstRes = nullptr,
		LPCWSTR pwszTypeRes = L"SVG", ID2D1Factory * pD2DFactory = nullptr) -> HBITMAP {
		const auto hRCSVG = ::FindResourceW(hInstRes, MAKEINTRESOURCEW(uIDRes), pwszTypeRes);
		assert(hRCSVG != nullptr);
		if (hRCSVG == nullptr)
			return { };

		const auto hResData = ::LoadResource(hInstRes, hRCSVG);
		assert(hResData != nullptr);
		if (hResData == nullptr)
			return { };

		const auto pResData = static_cast<const BYTE*>(::LockResource(hResData));
		assert(pResData != nullptr);
		if (pResData == nullptr)
			return { };

		const auto dwSizeRes = ::SizeofResource(hInstRes, hRCSVG);

		comptr<IStream> pStream = ::SHCreateMemStream(pResData, dwSizeRes);
		assert(pStream != nullptr);
		if (pStream == nullptr)
			return { };

		return SVGToBmp(pStream, iWidth, iHeight, pD2DFactory);
	}

	[[nodiscard]] auto HICONFromHBITMAP(HBITMAP hBmp) -> HICON {
		BITMAP bmp;
		::GetObjectW(hBmp, sizeof(BITMAP), &bmp);
		const auto hDCScreen = ::GetDC(nullptr);
		const auto hbmMask = ::CreateCompatibleBitmap(hDCScreen, bmp.bmWidth, bmp.bmHeight);
		ICONINFO ii { .fIcon { TRUE }, .hbmMask { hbmMask }, .hbmColor { hBmp } };
		const auto hIcon = ::CreateIconIndirect(&ii);
		::DeleteObject(hbmMask);
		::ReleaseDC(nullptr, hDCScreen);

		return hIcon;
	}

	[[nodiscard]] bool IsElevated() { //Checking if the app runs elevated.
		static const bool fElevated = [] {
			HANDLE hToken { };
			::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken);
			TOKEN_ELEVATION te { };
			DWORD cbSize { sizeof(te) };
			::GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &cbSize);
			::CloseHandle(hToken);
			return te.TokenIsElevated > 0;
			}();
		return fElevated;
	}

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

	[[nodiscard]] auto GetModulePath(HMODULE hMod = nullptr) -> std::wstring {
		std::wstring wstrPath;
		wstrPath.resize_and_overwrite(MAX_PATH, [hMod](wchar_t* pData, std::size_t szSize) {
			return ::GetModuleFileNameW(hMod, pData, static_cast<DWORD>(szSize)); });
		return wstrPath;
	}

	[[nodiscard]] auto GetModuleDir(HMODULE hMod = nullptr) -> std::wstring {
		auto wstrDir = GetModulePath(hMod);
		wstrDir = wstrDir.substr(0, wstrDir.find_last_of(L'\\'));
		return wstrDir;
	}

	[[nodiscard]] auto GetAppDataDir() -> const std::wstring& {
		static const auto wstrADD = [] {
			PWSTR pwsz { };
			::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &pwsz);
			auto wstr = std::wstring { pwsz } + L"\\" + GetAppName();
			::CoTaskMemFree(pwsz);
			return wstr;
			}
		();
		return wstrADD;
	}

	[[nodiscard]] auto GetSQLiteDBPath() -> const std::wstring& {
		static const auto wstrDB = GetAppDataDir() + L"\\" + GetAppName() + L".cfg";
		return wstrDB;
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

	void ShowLastError(DWORD dwErr = 0) {
		::MessageBoxW(nullptr, ut::GetLastErrorWstr(dwErr).data(), nullptr, 0);
	}

	[[nodiscard]] auto GetDeviceSize(HANDLE hHandle) -> std::expected<std::uint64_t, int> {
		DISK_GEOMETRY stDG { };
		if (DeviceIoControl(hHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, nullptr, 0, &stDG, sizeof(stDG),
			nullptr, nullptr) == FALSE) {
			return std::unexpected(GetLastError());
		}

		switch (stDG.MediaType) {
		case MEDIA_TYPE::Unknown:
		case MEDIA_TYPE::RemovableMedia:
		case MEDIA_TYPE::FixedMedia:
		{
			GET_LENGTH_INFORMATION stLI { };
			if (DeviceIoControl(hHandle, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &stLI,
				sizeof(stLI), nullptr, nullptr) == FALSE) {
				return std::unexpected(GetLastError());
			}
			return stLI.Length.QuadPart;
		}
		default:
			return stDG.Cylinders.QuadPart * stDG.TracksPerCylinder *
				stDG.SectorsPerTrack * stDG.BytesPerSector;
		}
	}

	[[nodiscard]] auto GetDeviceSize(const wchar_t* pwszPath) -> std::expected<std::uint64_t, int> {
		//The GENERIC_READ flag is mandatory for the IOCTL_DISK_GET_LENGTH_INFO to work.
		//The IOCTL_DISK_GET_LENGTH_INFO also requires elevation (Admin access) for some drives/volumes.
		const auto hHandle = CreateFileW(pwszPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hHandle == INVALID_HANDLE_VALUE) {
			return std::unexpected(GetLastError());
		}

		const auto u64Size = GetDeviceSize(hHandle);
		CloseHandle(hHandle);

		return u64Size;
	}

	[[nodiscard]] auto GetDPIScaleForHWND(HWND hWnd) -> float {
		return static_cast<float>(::GetDpiForWindow(hWnd)) / USER_DEFAULT_SCREEN_DPI; //High-DPI scale factor for window.
	}

	//Get GDI font size in points from the size in pixels.
	[[nodiscard]] auto FontPointsFromPixels(long iSizePixels) -> float {
		constexpr auto flPointsInPixel = 72.F / USER_DEFAULT_SCREEN_DPI;
		return std::abs(iSizePixels) * flPointsInPixel;
	}

	//Get GDI font size in pixels from the size in points.
	[[nodiscard]] auto FontPixelsFromPoints(float flSizePoints) -> long {
		constexpr auto flPixelsInPoint = USER_DEFAULT_SCREEN_DPI / 72.F;
		return std::lround(flSizePoints * flPixelsInPoint);
	}

	enum class EOpenMode : std::uint8_t {
		OPEN_FILE = 0x1, OPEN_DRIVE, OPEN_VOLUME, OPEN_PATH, OPEN_PROC, NEW_FILE
	};

	enum class EDataAccessMode : std::uint8_t {
		ACCESS_SAFE = 0x1, ACCESS_INPLACE = 0x2
	};

	struct DATAACCESS {
		constexpr DATAACCESS() = default;
		constexpr DATAACCESS(DWORD dw) {
			switch (dw) {
			case 0:
				fMutable = false;
				break;
			case 1:
				fMutable = true;
				eDataAccessMode = EDataAccessMode::ACCESS_SAFE;
				break;
			case 2:
				fMutable = true;
				eDataAccessMode = EDataAccessMode::ACCESS_INPLACE;
				break;
			default:
				fMutable = false;
				break;
			}
		};
		EDataAccessMode eDataAccessMode { };
		bool fMutable { };
		constexpr auto operator<=>(const DATAACCESS&)const = default;
		constexpr operator DWORD()const { return fMutable ? std::to_underlying(eDataAccessMode) : 0; };
	};

	enum class EDataIOMode : std::uint8_t {
		DATA_MMAP, DATA_IOBUFF, DATA_IOIMMEDIATE
	};

	[[nodiscard]] constexpr auto GetWstrEOpenMode(EOpenMode eOpenMode) {
		using enum EOpenMode;
		switch (eOpenMode) {
		case OPEN_DRIVE:
			return L"Drive";
		case OPEN_VOLUME:
			return L"Volume";
		case OPEN_PATH:
			return L"Path";
		case OPEN_PROC:
			return L"Process";
		case OPEN_FILE:
		case NEW_FILE:
			return L"File";
		default:
			return L"";
		};
	}

	[[nodiscard]] constexpr auto GetEOpenModeWstr(std::wstring_view wsv) -> std::optional<EOpenMode> {
		using enum EOpenMode;
		if (wsv == L"Drive")
			return OPEN_DRIVE;
		if (wsv == L"Volume")
			return OPEN_VOLUME;
		if (wsv == L"Path")
			return OPEN_PATH;
		if (wsv == L"File")
			return OPEN_FILE;
		if (wsv == L"Process")
			return OPEN_PROC;

		return std::nullopt;
	}

	[[nodiscard]] constexpr auto GetWstrDATAACCESS(DATAACCESS stDAC) {
		using enum EDataAccessMode;
		if (!stDAC.fMutable) {
			return L"Read Only";
		}

		switch (stDAC.eDataAccessMode) {
		case ACCESS_SAFE:
			return L"Read/Write Safe";
		case ACCESS_INPLACE:
			return L"Read/Write In-Place";
		default:
			return L"";
		}
	}

	[[nodiscard]] constexpr auto GetWstrEDataIOMode(EDataIOMode eDataIOMode) {
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
		const std::wstring_view wsv = pwszPath;
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

	[[nodiscard]] auto LoadDIBitmap(int iResID, int iWidth, int iHeight) -> HBITMAP {
		return static_cast<HBITMAP>(::LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(iResID),
			IMAGE_BITMAP, iWidth, iHeight, LR_CREATEDIBSECTION));
	}

	[[nodiscard]] auto TimetToWstr(std::time_t time) -> std::wstring {
		std::wstring wstr;
		wstr.resize_and_overwrite(32, [=](wchar_t* pData, std::size_t sSize)noexcept {
			std::tm tm;
			localtime_s(&tm, &time);
			return std::wcsftime(pData, sSize, L"%H:%M:%S", &tm);
		});

		return wstr;
	}

	struct DATAOPEN { //Main data opening struct.
		std::wstring  wstrDataPath; //Or Process name.
		std::wstring  wstrFriendlyName; //Used mostly for devices' friendly name.
		std::uint64_t ullSizeNewFile { };
		DWORD         dwProcID { };
		EOpenMode     eOpenMode { EOpenMode::OPEN_FILE };
		[[nodiscard]] bool operator==(const DATAOPEN& rhs)const {
			return wstrDataPath == rhs.wstrDataPath && dwProcID == rhs.dwProcID;
		};
	};

	struct DATAINFO { //Data for the CDlgDataInfo dialog.
		std::wstring_view wsvDataPath;
		std::wstring_view wsvFileName;
		std::wstring_view wsvFriendlyName;
		std::uint64_t     ullDataSize { };
		std::uint32_t     dwPageSize { };
		EOpenMode         eOpenMode { };
		DATAACCESS        stDAC;
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
	constexpr auto HEXCTRL_FLAG_BKMMGR_HEX { 0x1ULL };     //Bookmark Manager Hex.
	constexpr auto HEXCTRL_FLAG_BKMMGR_TT { 0x2ULL };      //Bookmark Manager Tooltips.


	namespace Log {
		enum class EMsgType :std::int8_t { //Enum id-number is the icon's index in the image list.
			Unknown = -1, msg_error = 0, msg_warning = 1, msg_info = 2
		};

		struct LOGINFO { //Struct for providing and transferring log data.
			std::time_t       time { std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
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