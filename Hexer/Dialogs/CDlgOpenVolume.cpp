/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CDlgOpenVolume.h"
#include "StrToNum.h"
#include "resource.h"
#include <format>

using namespace Utility;

BEGIN_MESSAGE_MAP(CDlgOpenVolume, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_OPEN_VOLUME_LIST_VOLUMES, &CDlgOpenVolume::OnListDblClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OPEN_VOLUME_LIST_VOLUMES, &CDlgOpenVolume::OnListItemChanged)
END_MESSAGE_MAP()

auto CDlgOpenVolume::GetPaths()->std::vector<std::wstring>&
{
	return m_vecPaths;
}

void CDlgOpenVolume::SetIWbemServices(IWbemServices* pWbemServices)
{
	m_pWbemServices = pWbemServices;
}


//Private methods.

void CDlgOpenVolume::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPEN_VOLUME_LIST_VOLUMES, m_list);
}

BOOL CDlgOpenVolume::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_list.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"Drive", 0, 100);
	m_list.InsertColumn(1, L"Label", 0, 100);
	m_list.InsertColumn(2, L"Size", 0, 70);
	m_list.InsertColumn(3, L"Type", 0, 100);
	m_list.InsertColumn(4, L"File System", 0, 80);
	m_list.InsertColumn(5, L"Path", 0, 300);

	m_vecVolumes = GetVolumes(m_pWbemServices);
	int index = 0;
	for (const auto& it : m_vecVolumes) {
		m_list.InsertItem(index, it.wstrDriveLetter.data());
		m_list.SetItemText(index, 1, it.wstrFileSystemLabel.data());
		m_list.SetItemText(index, 2, std::format(L"{:.1f} GB", static_cast<double>(it.ullSize) / 1024 / 1024 / 1024).data());
		m_list.SetItemText(index, 3, it.wstrDriveType.data());
		m_list.SetItemText(index, 4, it.wstrFileSystem.data());
		m_list.SetItemText(index, 5, it.wstrPath.data());

		++index;
	}

	return TRUE;
}

void CDlgOpenVolume::OnOK()
{
	m_vecPaths.clear();
	int nItem { -1 };
	for (auto i { 0UL }; i < m_list.GetSelectedCount(); ++i) {
		nItem = m_list.GetNextItem(nItem, LVNI_SELECTED);
		auto& refPath = m_vecVolumes.at(nItem).wstrPath;
		if (refPath.ends_with(L'\\')) { //Remove a trailing slash to satisfy CreateFileW.
			refPath = refPath.substr(0, refPath.size() - 1);
		}

		m_vecPaths.emplace_back(std::move(refPath));
	}

	if (!m_vecPaths.empty()) {
		static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDOK);
	}
}

void CDlgOpenVolume::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

void CDlgOpenVolume::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		OnOK();
	}
}

void CDlgOpenVolume::OnListItemChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	GetDlgItem(IDOK)->EnableWindow(m_list.GetSelectedCount() > 0);
}

auto CDlgOpenVolume::GetVolumes(IWbemServices *pWbemServices)->std::vector<Utility::VOLUME>
{
	if (pWbemServices == nullptr) {
		return { };
	}

	CComPtr<IEnumWbemClassObject> pMSFT_Volume;
	pWbemServices->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM MSFT_Volume"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pMSFT_Volume);

	std::vector<VOLUME> vecRet;
	while (pMSFT_Volume) {
		IWbemClassObject* pStorage { };
		ULONG uRet { 0 };
		if (const auto hr = pMSFT_Volume->Next(WBEM_INFINITE, 1, &pStorage, &uRet); hr != S_OK) {
			break;
		}

		VOLUME stDisk;
		VARIANT varDriveLetter;
		pStorage->Get(L"DriveLetter", 0, &varDriveLetter, nullptr, nullptr);
		stDisk.wstrDriveLetter = std::wstring { varDriveLetter.uiVal } + L":\\";

		VARIANT varFileSystemLabel;
		pStorage->Get(L"FileSystemLabel", 0, &varFileSystemLabel, nullptr, nullptr);
		stDisk.wstrFileSystemLabel = varFileSystemLabel.bstrVal;

		VARIANT varFileSystem;
		pStorage->Get(L"FileSystem", 0, &varFileSystem, nullptr, nullptr);
		stDisk.wstrFileSystem = varFileSystem.bstrVal;

		VARIANT varDriveType;
		pStorage->Get(L"DriveType", 0, &varDriveType, nullptr, nullptr);
		switch (varDriveType.uintVal) {
		case 0:
			stDisk.wstrDriveType = L"Unknown";
			break;
		case 1:
			stDisk.wstrDriveType = L"Invalid Root Path";
			break;
		case 2:
			stDisk.wstrDriveType = L"Removable";
			break;
		case 3:
			stDisk.wstrDriveType = L"Fixed";
			break;
		case 4:
			stDisk.wstrDriveType = L"Remote";
			break;
		case 5:
			stDisk.wstrDriveType = L"CD-ROM";
			break;
		case 6:
			stDisk.wstrDriveType = L"RAM Disk";
			break;
		default:
			break;
		}

		VARIANT varPath;
		pStorage->Get(L"Path", 0, &varPath, nullptr, nullptr);
		stDisk.wstrPath = varPath.bstrVal;

		VARIANT varSize;
		pStorage->Get(L"Size", 0, &varSize, nullptr, nullptr);
		stDisk.ullSize = varSize.ullVal;
		if (const auto opt = stn::StrToULL(varSize.bstrVal); opt) {
			stDisk.ullSize = *opt;
		}

		vecRet.emplace_back(stDisk);
		pStorage->Release();
	}

	return vecRet;
}