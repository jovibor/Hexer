/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CDlgOpenDisk.h"
#include "StrToNum.h"
#include "resource.h"
#include <Wbemidl.h>
#include <format>

#pragma comment(lib, "wbemuuid.lib") //For Wbemidl.h
using namespace Utility;

IMPLEMENT_DYNAMIC(CDlgOpenDisk, CDialogEx)

BEGIN_MESSAGE_MAP(CDlgOpenDisk, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_OPEN_DISK_LIST_DISKS, &CDlgOpenDisk::OnListDblClick)
END_MESSAGE_MAP()

auto CDlgOpenDisk::GetPaths()->std::vector<std::wstring>&
{
	return m_vecPaths;
}

void CDlgOpenDisk::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPEN_DISK_LIST_DISKS, m_list);
}

BOOL CDlgOpenDisk::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_list.SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_list.InsertColumn(0, L"Disk", 0, 150);
	m_list.InsertColumn(1, L"Size", 0, 70);
	m_list.InsertColumn(2, L"Path", 0, 120);
	m_list.InsertColumn(3, L"Media Type", 0, 80);
	m_list.InsertColumn(4, L"Bus Type", 0, 100);

	m_vecPhysicalDisks = GetPhysicalDisks();
	int index = 0;
	for (const auto& it : m_vecPhysicalDisks) {
		m_list.InsertItem(index, it.wstrFriendlyName.data());
		m_list.SetItemText(index, 1, std::format(L"{:.1f} GB", static_cast<double>(it.ullSize) / 1024 / 1024 / 1024).data());
		m_list.SetItemText(index, 2, it.wstrPath.data());
		if (g_mapMediaType.contains(it.eMediaType)) {
			m_list.SetItemText(index, 3, g_mapMediaType.at(it.eMediaType).data());
		}

		if (g_mapBusType.contains(it.eBusType)) {
			m_list.SetItemText(index, 4, g_mapBusType.at(it.eBusType).data());
		}

		++index;
	}

	return TRUE;
}

void CDlgOpenDisk::OnOK()
{
	m_vecPaths.clear();
	int nItem { -1 };
	for (auto i { 0UL }; i < m_list.GetSelectedCount(); ++i) {
		nItem = m_list.GetNextItem(nItem, LVNI_SELECTED);
		m_vecPaths.emplace_back(std::move(m_vecPhysicalDisks.at(nItem).wstrPath));
	}

	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDOK);
}

void CDlgOpenDisk::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

void CDlgOpenDisk::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		OnOK();
	}
}

auto CDlgOpenDisk::GetPhysicalDisks()->std::vector<PHYSICALDISK>
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);

	IWbemLocator *pWbemLocator { };
	CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pWbemLocator));

	IWbemServices *pWbemServices { };
	pWbemLocator->ConnectServer(_bstr_t(L"Root\\Microsoft\\Windows\\Storage"), nullptr, nullptr, nullptr, 0,
		nullptr, nullptr, &pWbemServices);
	CoSetProxyBlanket(pWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

	IEnumWbemClassObject* pMSFT_PhysicalDisk { };
	pWbemServices->ExecQuery(_bstr_t(L"WQL"), _bstr_t(L"SELECT * FROM MSFT_PhysicalDisk"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &pMSFT_PhysicalDisk);

	std::vector<Utility::PHYSICALDISK> vecRet;
	while (pMSFT_PhysicalDisk) {
		IWbemClassObject *pStorage { };
		ULONG uRet { 0 };
		if (const auto hr = pMSFT_PhysicalDisk->Next(WBEM_INFINITE, 1, &pStorage, &uRet); hr != S_OK) {
			break;
		}

		PHYSICALDISK stDisk;
		VARIANT varFriendlyName;
		pStorage->Get(L"FriendlyName", 0, &varFriendlyName, nullptr, nullptr);
		stDisk.wstrFriendlyName = varFriendlyName.bstrVal;

		VARIANT varDeviceId;
		pStorage->Get(L"DeviceId", 0, &varDeviceId, nullptr, nullptr);
		stDisk.wstrPath = std::wstring { L"\\\\.\\PhysicalDrive" } + varDeviceId.bstrVal;

		VARIANT varSize;
		pStorage->Get(L"Size", 0, &varSize, nullptr, nullptr);
		if (const auto opt = stn::StrToULL(varSize.bstrVal); opt) {
			stDisk.ullSize = *opt;
		}

		VARIANT varBusType;
		pStorage->Get(L"BusType", 0, &varBusType, nullptr, nullptr);
		stDisk.eBusType = static_cast<Utility::EBusType>(varBusType.uiVal);

		VARIANT varMediaType;
		pStorage->Get(L"MediaType", 0, &varMediaType, nullptr, nullptr);
		stDisk.eMediaType = static_cast<Utility::EMediaType>(varMediaType.uiVal);

		vecRet.emplace_back(stDisk);
		pStorage->Release();
	}

	pMSFT_PhysicalDisk->Release();
	pWbemServices->Release();
	pWbemLocator->Release();
	CoUninitialize();

	return vecRet;
}