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
#include "HexCtrl.h"
#include <cassert>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>
export module DlgProcMemory;

import Utility;

export class CDlgProcMemory final : public CDialogEx {
public:
	CDlgProcMemory(CWnd* pParent = nullptr) : CDialogEx(IDD_PROCMEMORY, pParent) { }
	void Init(HEXCTRL::IHexCtrl* pHexCtrl, const std::vector<MEMORY_BASIC_INFORMATION>& vec);
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	afx_msg void OnListProcMemoryColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListProcMemoryGetColor(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListProcMemoryGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListProcMemoryItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)override;
	DECLARE_MESSAGE_MAP();
private:
	lex::CListEx m_pListProcMem;
	HEXCTRL::IHexCtrl* m_pHexCtrl { };
	const std::vector<MEMORY_BASIC_INFORMATION>* m_pVecProcMemory { };
};

BEGIN_MESSAGE_MAP(CDlgProcMemory, CDialogEx)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_PROCMEMORY_LIST, &CDlgProcMemory::OnListProcMemoryGetDispInfo)
	ON_NOTIFY(lex::LISTEX_MSG_GETCOLOR, IDC_PROCMEMORY_LIST, &CDlgProcMemory::OnListProcMemoryGetColor)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PROCMEMORY_LIST, &CDlgProcMemory::OnListProcMemoryItemChanged)
END_MESSAGE_MAP()

void CDlgProcMemory::Init(HEXCTRL::IHexCtrl* pHexCtrl, const std::vector<MEMORY_BASIC_INFORMATION>& vec)
{
	m_pHexCtrl = pHexCtrl;
	m_pVecProcMemory = &vec;
}

void CDlgProcMemory::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CDlgProcMemory::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	assert(m_pVecProcMemory != nullptr);

	m_pListProcMem.CreateDialogCtrl(IDC_PROCMEMORY_LIST, m_hWnd);
	m_pListProcMem.InsertColumn(0, L"Address Range", LVCFMT_CENTER, 170);
	m_pListProcMem.InsertColumn(1, L"Size", LVCFMT_CENTER, 70, -1, LVCFMT_CENTER);
	m_pListProcMem.InsertColumn(2, L"Protection", LVCFMT_CENTER, 180);
	m_pListProcMem.InsertColumn(3, L"Type", LVCFMT_CENTER, 100);
	m_pListProcMem.SetItemCountEx(static_cast<int>(m_pVecProcMemory->size()));

	const auto hIcon = AfxGetApp()->LoadIconW(IDR_HEXER_FRAME);
	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	return TRUE;
}

void CDlgProcMemory::OnListProcMemoryColumnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
}

void CDlgProcMemory::OnListProcMemoryItemChanged(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	const auto iItem = pNMI->iItem;
	const auto iSubItem = pNMI->iSubItem;
	if (iItem < 0 || iSubItem < 0 || !(pNMI->uNewState & LVIS_SELECTED))
		return;

	const auto ullOffset = m_pHexCtrl->GetOffset(reinterpret_cast<ULONGLONG>((*m_pVecProcMemory)[iItem].BaseAddress), false);
	m_pHexCtrl->SetCaretPos(ullOffset, true, false);
	m_pHexCtrl->GoToOffset(ullOffset, -1);
}

void CDlgProcMemory::OnListProcMemoryGetColor(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pLCI = reinterpret_cast<lex::PLISTEXCOLORINFO>(pNMHDR);
	if (pLCI->iSubItem == 2) { //Protection.
		constexpr DWORD dwWritable = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
		if ((*m_pVecProcMemory)[pLCI->iItem].Protect & dwWritable) {
			pLCI->stClr = { RGB(80, 200, 80), RGB(250, 250, 250) };
		}
	}
}

void CDlgProcMemory::OnListProcMemoryGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	static const std::unordered_map<DWORD, const wchar_t*> umapMemProtect {
		{ PAGE_NOACCESS, L"PAGE_NOACCESS" },
		{ PAGE_READONLY, L"PAGE_READONLY" },
		{ PAGE_READWRITE, L"PAGE_READWRITE" },
		{ PAGE_WRITECOPY, L"PAGE_WRITECOPY" },
		{ PAGE_EXECUTE, L"PAGE_EXECUTE" },
		{ PAGE_EXECUTE_READ, L"PAGE_EXECUTE_READ" },
		{ PAGE_EXECUTE_READWRITE, L"PAGE_EXECUTE_READWRITE" },
		{ PAGE_EXECUTE_WRITECOPY, L"PAGE_EXECUTE_WRITECOPY" },
		{ PAGE_GUARD, L"PAGE_GUARD" },
		{ PAGE_NOCACHE, L"PAGE_NOCACHE" },
		{ PAGE_WRITECOMBINE, L"PAGE_WRITECOMBINE" },
		{ PAGE_TARGETS_INVALID, L"PAGE_TARGETS_INVALID" },
		{ PAGE_ENCLAVE_DECOMMIT, L"PAGE_ENCLAVE_DECOMMIT" },
		{ PAGE_ENCLAVE_THREAD_CONTROL, L"PAGE_ENCLAVE_THREAD_CONTROL" },
		{ PAGE_ENCLAVE_UNVALIDATED, L"PAGE_ENCLAVE_UNVALIDATED" }
	};
	static const std::unordered_map<DWORD, const wchar_t*> umapMemType {
		{ MEM_IMAGE, L"MEM_IMAGE" }, { MEM_MAPPED, L"MEM_MAPPED" }, { MEM_PRIVATE, L"MEM_PRIVATE" }
	};

	const auto pDI = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDI->item;
	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItem = pItem->iItem;
	const auto iSubItem = pItem->iSubItem;
	assert(m_pVecProcMemory != nullptr);
	const auto& ref = (*m_pVecProcMemory)[iItem];

	switch (iSubItem) {
	case 0: //Offset range.
	{
		const auto ullOffset = reinterpret_cast<std::uint64_t>(ref.BaseAddress);
		*std::format_to(pItem->pszText, L"{:X}h-{:X}h", ullOffset, ullOffset + ref.RegionSize) = L'\0';
	}
	break;
	case 1: //Size.
		*std::format_to(pItem->pszText, L"{:X}h", ref.RegionSize) = L'\0';
		break;
	case 2: //Protection.
	{
		std::wstring wstrData;
		for (const auto& [dwFlag, pwszData] : umapMemProtect) {
			if (dwFlag & ref.Protect) {
				if (!wstrData.empty()) {
					wstrData += L" | ";
				}
				wstrData += pwszData;
			}
		}
		*std::format_to(pItem->pszText, L"{}", wstrData) = L'\0';
	}
	break;
	case 3: //Type.
		if (const auto it = umapMemType.find(ref.Type); it != umapMemType.end()) {
			*std::format_to(pItem->pszText, L"{}", it->second) = L'\0';
		}
		break;
	default:
		break;
	}
}

BOOL CDlgProcMemory::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT * pResult)
{
	const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam);

	if (pNMI->hdr.idFrom == IDC_PROCMEMORY_LIST) {
		if (pNMI->hdr.code == LVN_COLUMNCLICK) {
			OnListProcMemoryColumnClick(reinterpret_cast<NMHDR*>(lParam), pResult);
			return TRUE;
		}
	}

	return CDialogEx::OnNotify(wParam, lParam, pResult);
}