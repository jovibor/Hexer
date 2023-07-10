module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "ListEx.h"
#include "resource.h"
#include <afxdialogex.h>
#include <chrono>
#include <format>
#include <string>
#include <vector>
export module DlgLogInfo;

import Utility;
using namespace Ut;
namespace lex = HEXCTRL::LISTEX;

export class CDlgLogInfo final : public CDialogEx
{
public:
	void AddLogEntry(const Ut::Log::LOGDATA& stData);
private:
	void ClearAll();
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnCommand(WPARAM wParam, LPARAM lParam)override;
	BOOL OnInitDialog()override;
	afx_msg void OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListGetIcon(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListRClick(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP();
private:
	enum class EMenuID : std::uint16_t {
		IDM_LIST_CLEARALL = 0x8001
	};
	lex::IListExPtr m_pList { lex::CreateListEx() };
	std::vector<Ut::Log::LOGDATA> m_vecData { };
	CImageList m_stImgList;
	CMenu m_menuList;
};

BEGIN_MESSAGE_MAP(CDlgLogInfo, CDialogEx)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_LOGINFO_LIST, &CDlgLogInfo::OnListGetDispInfo)
	ON_NOTIFY(lex::LISTEX_MSG_GETICON, IDC_LOGINFO_LIST, &CDlgLogInfo::OnListGetIcon)
	ON_NOTIFY(NM_RCLICK, IDC_LOGINFO_LIST, &CDlgLogInfo::OnListRClick)
END_MESSAGE_MAP()

void CDlgLogInfo::AddLogEntry(const Ut::Log::LOGDATA& stData)
{
	m_vecData.emplace_back(stData);

	if (IsWindow(m_hWnd)) {
		m_pList->SetItemCountEx(static_cast<int>(m_vecData.size()), LVSICF_NOSCROLL);
	}
}


//Private methods.

void CDlgLogInfo::ClearAll()
{
	m_vecData.clear();
	m_pList->SetItemCountEx(static_cast<int>(m_vecData.size()), LVSICF_NOSCROLL);
}

void CDlgLogInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CDlgLogInfo::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (static_cast<EMenuID>(LOWORD(wParam))) {
	case EMenuID::IDM_LIST_CLEARALL:
		ClearAll();
		return TRUE;
	default:
		break;
	}

	return CDialogEx::OnCommand(wParam, lParam);
}

BOOL CDlgLogInfo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pList->CreateDialogCtrl(IDC_LOGINFO_LIST, this);
	m_pList->SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	m_pList->InsertColumn(0, L"№", 0, 30);
	m_pList->InsertColumn(1, L"Time", 0, 70);
	m_pList->InsertColumn(2, L"Event", 0, 450);

	m_menuList.CreatePopupMenu();
	m_menuList.AppendMenuW(MF_STRING, static_cast<UINT_PTR>(EMenuID::IDM_LIST_CLEARALL), L"Clear All");

	const auto iIconSize = static_cast<int>(16 * Ut::GetHiDPIInfo().flDPIScale);
	m_stImgList.Create(iIconSize, iIconSize, ILC_COLOR | ILC_MASK, 2, 2);

	//Icons added in exact order: "msg_error, msg_warning, Info" as the enum values in the EMsgType.
	m_stImgList.Add(static_cast<HICON>(LoadImageW(nullptr, IDI_ERROR, IMAGE_ICON, 0, 0, LR_SHARED)));
	m_stImgList.Add(static_cast<HICON>(LoadImageW(nullptr, IDI_WARNING, IMAGE_ICON, 0, 0, LR_SHARED)));
	m_stImgList.Add(static_cast<HICON>(LoadImageW(nullptr, IDI_INFORMATION, IMAGE_ICON, 0, 0, LR_SHARED)));
	m_pList->SetImageList(&m_stImgList, LVSIL_NORMAL);
	m_pList->SetItemCountEx(static_cast<int>(m_vecData.size()), LVSICF_NOSCROLL); //m_vecData can already have data.

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(m_pList->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));

	return TRUE;
}

void CDlgLogInfo::OnListGetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pDI = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	const auto pItem = &pDI->item;

	if ((pItem->mask & LVIF_TEXT) == 0)
		return;

	const auto iItemID = pItem->iItem;
	if (iItemID < 0 || pItem->iSubItem < 0)
		return;

	const auto& refData = m_vecData[iItemID];
	switch (pItem->iSubItem) {
	case 0: //№.
		*std::format_to(pItem->pszText, L"{}", iItemID + 1) = L'\0';
		break;
	case 1: //Time.
		*std::format_to(pItem->pszText, L"{:%H:%M:%OS}", refData.tmloc) = L'\0';
		break;
	case 2: //Event.
		*std::format_to(pItem->pszText, L"{}", refData.wstrMsg) = L'\0';
		break;
	default:
		break;
	}
}

void CDlgLogInfo::OnListGetIcon(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	const auto pNMI = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
	if (pNMI->iItem < 0 || pNMI->iSubItem != 2) //Event field.
		return;

	pNMI->lParam = static_cast<int>(m_vecData[pNMI->iItem].eType); //Icon index in list's image list.
}

void CDlgLogInfo::OnListRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	POINT pt;
	GetCursorPos(&pt);
	m_menuList.TrackPopupMenuEx(TPM_LEFTALIGN, pt.x, pt.y, this, nullptr);
}