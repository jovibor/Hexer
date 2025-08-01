module;
/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "resource.h"
#include <afxdialogex.h>
#include <algorithm>
#include <format>
#include <psapi.h>
#include <string>
#include <vector>
#include <wtsapi32.h>
export module DlgOpenProcess;

#pragma comment(lib, "Wtsapi32.lib")
import Utility;

export class CDlgOpenProcess final : public CDialogEx {
public:
	struct PROCDATA;
	struct MODULEDATA;
	CDlgOpenProcess(CWnd* pParent = nullptr) : CDialogEx(IDD_OPENPROCESS, pParent) { }
	[[nodiscard]] auto GetOpenData()const -> const std::vector<ut::DATAOPEN>&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	void EnableDynamicLayoutHelper(bool fEnable);
	afx_msg void OnBnClickedRefresh();
	afx_msg auto OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) -> HBRUSH;
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	BOOL OnInitDialog()override;
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnListModulesColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListModulesGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListModulesDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListProcsColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListProcsItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListProcsDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListProcsGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListProcsGetTooltip(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
	void OnOK()override;
	void OnProcReady(bool fReady);
	void RefreshProcs();
	DECLARE_MESSAGE_MAP();
private:
	lex::CListEx m_ListProcs;
	lex::CListEx m_ListModules;
	std::vector<PROCDATA> m_vecProcs;
	std::vector<MODULEDATA> m_vecModules;
	std::vector<ut::DATAOPEN> m_vecOpenData;
	std::wstring m_wstrTTProcPath; //Tooltip for process image full path.
	std::locale m_locale;
	CButton m_btnOpen;
	CButton m_statInfo;
	bool m_fCurInSplitter { }; //Indicates that mouse cursor is in the splitter area.
	bool m_fLMDownResize { };  //Left mouse pressed in the splitter area to resize.
	bool m_fProcReady { false };
};

struct CDlgOpenProcess::PROCDATA {
	std::wstring wstrProcName;
	DWORD        dwProcID { };
	DWORD        dwWorkingSet { };
};

struct CDlgOpenProcess::MODULEDATA {
	std::wstring wstrModName;
	DWORD        dwWorkingSet { };
};

BEGIN_MESSAGE_MAP(CDlgOpenProcess, CDialogEx)
	ON_BN_CLICKED(IDC_OPENPROCESS_BTN_REFRESH, &CDlgOpenProcess::OnBnClickedRefresh)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_OPENPROCESS_LIST_MODULES, &CDlgOpenProcess::OnListModulesGetDispInfo)
	ON_NOTIFY(NM_DBLCLK, IDC_OPENPROCESS_LIST_MODULES, &CDlgOpenProcess::OnListModulesDblClick)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_OPENPROCESS_LIST_PROCS, &CDlgOpenProcess::OnListProcsGetDispInfo)
	ON_NOTIFY(lex::LISTEX_MSG_GETTOOLTIP, IDC_OPENPROCESS_LIST_PROCS, &CDlgOpenProcess::OnListProcsGetTooltip)
	ON_NOTIFY(NM_DBLCLK, IDC_OPENPROCESS_LIST_PROCS, &CDlgOpenProcess::OnListProcsDblClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OPENPROCESS_LIST_PROCS, &CDlgOpenProcess::OnListProcsItemChanged)
	ON_WM_CTLCOLOR()
	ON_WM_DRAWITEM()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MEASUREITEM()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

auto CDlgOpenProcess::GetOpenData()const->const std::vector<ut::DATAOPEN>&
{
	return m_vecOpenData;
}

void CDlgOpenProcess::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_btnOpen);
	DDX_Control(pDX, IDC_OPENPROCESS_STAT_INFO, m_statInfo);
}

void CDlgOpenProcess::EnableDynamicLayoutHelper(bool fEnable)
{
	if (fEnable && IsDynamicLayoutEnabled())
		return;

	EnableDynamicLayout(fEnable ? TRUE : FALSE);

	if (fEnable) {
		const auto pLayout = GetDynamicLayout();
		pLayout->Create(this);

		pLayout->AddItem(IDC_OPENPROCESS_LIST_PROCS, CMFCDynamicLayout::MoveNone(),
			CMFCDynamicLayout::SizeHorizontalAndVertical(50, 100));
		pLayout->AddItem(IDC_OPENPROCESS_LIST_MODULES, CMFCDynamicLayout::MoveHorizontal(50),
			CMFCDynamicLayout::SizeHorizontalAndVertical(50, 100));
		pLayout->AddItem(IDC_OPENPROCESS_BTN_REFRESH, CMFCDynamicLayout::MoveVertical(100),
			CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDC_OPENPROCESS_STAT_INFO, CMFCDynamicLayout::MoveVertical(100),
			CMFCDynamicLayout::SizeHorizontal(100));
		pLayout->AddItem(IDOK, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100),
			CMFCDynamicLayout::SizeNone());
		pLayout->AddItem(IDCANCEL, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100),
			CMFCDynamicLayout::SizeNone());
	}
}

void CDlgOpenProcess::OnBnClickedRefresh()
{
	RefreshProcs();
}

auto CDlgOpenProcess::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)->HBRUSH
{
	const auto hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd == &m_statInfo) {
		if (m_fProcReady) {
			pDC->SetTextColor(RGB(0, 250, 0));
		}
		else {
			pDC->SetTextColor(RGB(250, 0, 0));
		}
	}

	return hbr;
}

void CDlgOpenProcess::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl == IDC_OPENPROCESS_LIST_PROCS) {
		m_ListProcs.DrawItem(lpDrawItemStruct);
		return;
	}
	else if (nIDCtl == IDC_OPENPROCESS_LIST_MODULES) {
		m_ListModules.DrawItem(lpDrawItemStruct);
		return;
	}

	CDialogEx::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

BOOL CDlgOpenProcess::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_locale = std::locale("en_US.UTF-8");
	const lex::LISTEXCREATE lcs { .hWndParent { m_hWnd }, .uID { IDC_OPENPROCESS_LIST_PROCS }, .dwTTStyleCell { TTS_NOANIMATE },
		.dwTTDelayTime { 500 }, .dwTTShowTime { 3000 }, .ptTTOffset { 9, -20 }, .fDialogCtrl { true }, .fSortable { true } };
	m_ListProcs.Create(lcs);
	m_ListProcs.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
	m_ListProcs.InsertColumn(0, L"№", 0, 40);
	m_ListProcs.InsertColumn(1, L"Process Name", 0, 200);
	m_ListProcs.InsertColumn(2, L"Process ID", 0, 70);
	m_ListProcs.InsertColumn(3, L"Working Set", 0, 90);
	m_ListProcs.SetColumnSortMode(0, false);

	m_ListModules.CreateDialogCtrl(IDC_OPENPROCESS_LIST_MODULES, m_hWnd);
	m_ListModules.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
	m_ListModules.SetSortable(true);
	m_ListModules.InsertColumn(0, L"Module Name", 0, 150);
	m_ListModules.InsertColumn(1, L"Working Set", 0, 90);

	const auto hIcon = AfxGetApp()->LoadIconW(IDR_HEXER_FRAME);
	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);
	m_btnOpen.EnableWindow(FALSE);
	RefreshProcs();
	EnableDynamicLayoutHelper(true);

	return TRUE;
}

void CDlgOpenProcess::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_fCurInSplitter) {
		m_fLMDownResize = true;
		SetCapture();
		EnableDynamicLayoutHelper(false);
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CDlgOpenProcess::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_fLMDownResize = false;
	ReleaseCapture();
	EnableDynamicLayoutHelper(true);

	CDialogEx::OnLButtonUp(nFlags, point);
}

void CDlgOpenProcess::OnListModulesColumnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	const auto iColumn = m_ListModules.GetSortColumn();
	const auto fAscending = m_ListModules.GetSortAscending();
	std::sort(m_vecModules.begin(), m_vecModules.end(), [iColumn, fAscending](const MODULEDATA& lhs, const MODULEDATA& rhs) {
		int iCompare { };
		switch (iColumn) {
		case 0: //Module Name.
			iCompare = lhs.wstrModName.compare(rhs.wstrModName);
			break;
		case 1: //Working Set.
			iCompare = lhs.dwWorkingSet < rhs.dwWorkingSet ? -1 : (lhs.dwWorkingSet > rhs.dwWorkingSet ? 1 : 0);
			break;
		default:
			break;
		}

		return fAscending ? iCompare < 0 : iCompare > 0;
		});
	m_ListModules.RedrawWindow();
}

void CDlgOpenProcess::OnListModulesGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDI = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDI->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItem = pItem->iItem;

	switch (pItem->iSubItem) {
	case 0: //Module Name.
		pItem->pszText = m_vecModules[iItem].wstrModName.data();
		break;
	case 1: //Working Set.
		*std::format_to(pItem->pszText, m_locale, L"{:L}KB", m_vecModules[iItem].dwWorkingSet / 1024) = L'\0';
		break;
	default:
		break;
	}
}

void CDlgOpenProcess::OnListModulesDblClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
}

void CDlgOpenProcess::OnListProcsColumnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	const auto iColumn = m_ListProcs.GetSortColumn();
	const auto fAscending = m_ListProcs.GetSortAscending();
	std::sort(m_vecProcs.begin(), m_vecProcs.end(), [iColumn, fAscending](const PROCDATA& lhs, const PROCDATA& rhs) {
		int iCompare { };
		switch (iColumn) {
		case 0:
			break;
		case 1: //Process Name.
			iCompare = lhs.wstrProcName.compare(rhs.wstrProcName);
			break;
		case 2: //Process ID.
			iCompare = lhs.dwProcID < rhs.dwProcID ? -1 : (lhs.dwProcID > rhs.dwProcID ? 1 : 0);
			break;
		case 3: //Working Set.
			iCompare = lhs.dwWorkingSet < rhs.dwWorkingSet ? -1 : (lhs.dwWorkingSet > rhs.dwWorkingSet ? 1 : 0);
			break;
		default:
			break;
		}

		return fAscending ? iCompare < 0 : iCompare > 0;
		});
	m_ListProcs.RedrawWindow();
}

void CDlgOpenProcess::OnListProcsItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMI->iItem < 0 || pNMI->iSubItem < 0 || !(pNMI->uNewState & LVIS_SELECTED))
		return;

	const auto iItemID = pNMI->iItem;
	const auto hProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_vecProcs[iItemID].dwProcID);
	if (hProc == nullptr) {
		OnProcReady(false);
		m_ListModules.SetItemCountEx(0);
		m_ListModules.RedrawWindow();
		m_vecModules.clear();
		return;
	}

	m_vecModules.clear();
	HMODULE arrHModules[1024];
	DWORD dwCbNeeded { };
	if (::K32EnumProcessModules(hProc, arrHModules, sizeof(arrHModules), &dwCbNeeded)) {
		const DWORD dwModsCount = dwCbNeeded / sizeof(HMODULE); //How many modules the hProc has.
		for (auto i { 0UL }; i < dwModsCount; ++i) {
			if (wchar_t arrModName[MAX_PATH]; ::K32GetModuleFileNameExW(hProc, arrHModules[i], arrModName, MAX_PATH)) {
				MODULEINFO mi;
				::K32GetModuleInformation(hProc, arrHModules[i], &mi, sizeof(MODULEINFO));
				std::wstring_view wsvModPath { arrModName };
				const auto wsvModName = wsvModPath.substr(wsvModPath.find_last_of(L'\\') + 1);
				m_vecModules.emplace_back(wsvModName.data(), mi.SizeOfImage);
			}
		}
	}

	::CloseHandle(hProc);
	m_ListModules.SetItemCountEx(static_cast<int>(m_vecModules.size()));
	m_ListModules.RedrawWindow();
	OnProcReady(true);
}

void CDlgOpenProcess::OnListProcsDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		OnOK();
	}
}

void CDlgOpenProcess::OnListProcsGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDI = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDI->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItem = pItem->iItem;

	switch (pItem->iSubItem) {
	case 0: //№.
		*std::format_to(pItem->pszText, L"{}", iItem) = L'\0';
		break;
	case 1: //Process Name.
		pItem->pszText = m_vecProcs[iItem].wstrProcName.data();
		break;
	case 2: //Process ID.
		*std::format_to(pItem->pszText, L"{}", m_vecProcs[iItem].dwProcID) = L'\0';
		break;
	case 3: //Working Set.
		*std::format_to(pItem->pszText, m_locale, L"{:L}KB", m_vecProcs[iItem].dwWorkingSet / 1024) = L'\0';
		break;
	default:
		break;
	}
}

void CDlgOpenProcess::OnListProcsGetTooltip(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pTTI = reinterpret_cast<lex::LISTEXTTINFO*>(pNMHDR);
	const auto iItem = pTTI->iItem;
	const auto iSubItem = pTTI->iSubItem;
	if (iItem < 0 || iSubItem != 1) //First column.
		return;

	const auto hProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_vecProcs[iItem].dwProcID);
	if (hProc == nullptr)
		return;

	m_wstrTTProcPath.resize_and_overwrite(MAX_PATH, [hProc](wchar_t* pData, std::size_t sSize) {
		DWORD dwSize { static_cast<DWORD>(sSize) };
		const auto ret = ::QueryFullProcessImageNameW(hProc, 0, pData, &dwSize);
		return ret ? dwSize : 0; });
	::CloseHandle(hProc);

	pTTI->stData.pwszText = m_wstrTTProcPath.data();
}

void CDlgOpenProcess::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	if (nIDCtl == IDC_OPENPROCESS_LIST_PROCS) {
		m_ListProcs.MeasureItem(lpMeasureItemStruct);
		return;
	}
	else if (nIDCtl == IDC_OPENPROCESS_LIST_MODULES) {
		m_ListModules.MeasureItem(lpMeasureItemStruct);
		return;
	}

	CDialogEx::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

void CDlgOpenProcess::OnMouseMove(UINT nFlags, CPoint point)
{
	static constexpr auto iResAreaHalfWidth = 6;   //Area where cursor turns into resizable (IDC_SIZEWE).
	static constexpr auto iWidthBetweenLists = 1;  //Width between tree and list after resizing.
	static constexpr auto iMinLeftListWidth = 100; //Left list minimum allowed width.
	static const auto hCurResize = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_SIZEWE, IMAGE_CURSOR, 0, 0, LR_SHARED));
	static const auto hCurArrow = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED));

	CRect rcList;
	::GetWindowRect(m_ListModules.GetHWND(), rcList);
	ScreenToClient(rcList);

	if (m_fLMDownResize) {
		CRect rcTree;
		::GetWindowRect(m_ListProcs.GetHWND(), rcTree);
		ScreenToClient(rcTree);
		rcTree.right = point.x - iWidthBetweenLists;
		rcList.left = point.x;
		if (rcTree.Width() >= iMinLeftListWidth) {
			auto hdwp = BeginDeferWindowPos(2); //Simultaneously resizing lists.
			hdwp = DeferWindowPos(hdwp, m_ListProcs.GetHWND(), nullptr, rcTree.left, rcTree.top,
				rcTree.Width(), rcTree.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
			hdwp = DeferWindowPos(hdwp, m_ListModules.GetHWND(), nullptr, rcList.left, rcList.top,
				rcList.Width(), rcList.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
			EndDeferWindowPos(hdwp);
		}
	}
	else {
		if (const CRect rcSplitter(rcList.left - iResAreaHalfWidth, rcList.top, rcList.left + iResAreaHalfWidth, rcList.bottom);
			rcSplitter.PtInRect(point)) {
			m_fCurInSplitter = true;
			SetCursor(hCurResize);
			SetCapture();
		}
		else {
			m_fCurInSplitter = false;
			SetCursor(hCurArrow);
			ReleaseCapture();
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

BOOL CDlgOpenProcess::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam);

	if (pNMI->hdr.idFrom == IDC_OPENPROCESS_LIST_PROCS) {
		if (pNMI->hdr.code == LVN_COLUMNCLICK) {
			OnListProcsColumnClick(reinterpret_cast<NMHDR*>(lParam), pResult);
			return TRUE;
		}
	}
	else if (pNMI->hdr.idFrom == IDC_OPENPROCESS_LIST_MODULES) {
		if (pNMI->hdr.code == LVN_COLUMNCLICK) {
			OnListModulesColumnClick(reinterpret_cast<NMHDR*>(lParam), pResult);
			return TRUE;
		}
	}


	return CDialogEx::OnNotify(wParam, lParam, pResult);
}

void CDlgOpenProcess::OnOK()
{
	if (!m_fProcReady)
		return;

	m_vecOpenData.clear();
	int nItem { -1 };
	for (auto i { 0UL }; i < m_ListProcs.GetSelectedCount(); ++i) {
		nItem = m_ListProcs.GetNextItem(nItem, LVNI_SELECTED);
		auto& ref = m_vecProcs[nItem];
		m_vecOpenData.emplace_back(ut::DATAOPEN { .wstrDataPath { std::move(ref.wstrProcName) },
			.dwProcID { ref.dwProcID }, .eOpenMode { ut::EOpenMode::OPEN_PROC } });
	}

	CDialogEx::OnOK();
}

void CDlgOpenProcess::OnProcReady(bool fReady)
{
	std::wstring wstr;
	if (fReady) {
		wstr = L"Process ready to be opened";
	}
	else {
		wchar_t buffErr[MAX_PATH];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffErr, MAX_PATH, nullptr);
		(wstr += L"Process can't be opened: ") += buffErr;
	}

	m_fProcReady = fReady;
	m_btnOpen.EnableWindow(fReady);
	m_statInfo.SetWindowTextW(wstr.data());
}

void CDlgOpenProcess::RefreshProcs()
{
	m_vecProcs.clear();
	DWORD dwLevel { 1 };
	WTS_PROCESS_INFO_EXW* pWPI { };
	DWORD dwCount { };
	if (WTSEnumerateProcessesExW(WTS_CURRENT_SERVER_HANDLE, &dwLevel, WTS_ANY_SESSION,
		reinterpret_cast<LPWSTR*>(&pWPI), &dwCount) == FALSE) {
		MessageBoxW(L"Process enumeration failed", L"Error", MB_ICONERROR);
		return;
	}

	m_vecProcs.reserve(dwCount);
	for (auto i { 0UL }; i < dwCount; ++i) {
		m_vecProcs.emplace_back(pWPI[i].pProcessName, pWPI[i].ProcessId, pWPI[i].WorkingSetSize);
	}

	WTSFreeMemoryExW(WTS_TYPE_CLASS::WTSTypeProcessInfoLevel1, pWPI, dwCount);

	m_ListProcs.SetItemCountEx(static_cast<int>(m_vecProcs.size()));
	m_ListProcs.RedrawWindow();
}