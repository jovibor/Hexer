module;
/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "resource.h"
#include <afxdialogex.h>
#include <algorithm>
#include <setupapi.h>
#include <winioctl.h>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>
#pragma comment (lib, "SetupAPI.lib")
export module DlgOpenDevice;

import Utility;

//CDlgOpenDisk.

class CDlgOpenDrive final : public CDialogEx {
	struct DEVICE_DRIVE {
		std::wstring  wstrDrivePath;
		std::wstring  wstrFriendlyName;
		std::wstring  wstrBusType;
		std::wstring  wstrSize;
	};
public:
	[[nodiscard]] auto GetOpenData()const->std::vector<Ut::DATAOPEN>;
	[[nodiscard]] bool IsOK();
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	void OnCancel()override;
	afx_msg void OnListDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void OnOK()override;
	DECLARE_MESSAGE_MAP();
	[[nodiscard]] static auto GetDeviceDrives() -> std::vector<DEVICE_DRIVE>;
private:
	lex::IListExPtr m_pList { lex::CreateListEx() };
};

BEGIN_MESSAGE_MAP(CDlgOpenDrive, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_OPENDRIVE_LIST, &CDlgOpenDrive::OnListDblClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OPENDRIVE_LIST, &CDlgOpenDrive::OnListItemChanged)
END_MESSAGE_MAP()

auto CDlgOpenDrive::GetOpenData()const->std::vector<Ut::DATAOPEN>
{
	std::vector<Ut::DATAOPEN> vec;
	int nItem { -1 };
	for (auto i { 0UL }; i < m_pList->GetSelectedCount(); ++i) {
		nItem = m_pList->GetNextItem(nItem, LVNI_SELECTED);
		vec.emplace_back(Ut::DATAOPEN { .wstrDataPath { m_pList->GetItemText(nItem, 2) },
			.eOpenMode { Ut::EOpenMode::OPEN_DEVICE } });
	}

	return vec;
}

bool CDlgOpenDrive::IsOK()
{
	return m_pList->GetSelectedCount() > 0;
}


//CDlgOpenDisk Private methods.

void CDlgOpenDrive::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CDlgOpenDrive::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(IDC_OPENDRIVE_LIST, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));

	m_pList->Create({ .pParent { this }, .uID { IDC_OPENDRIVE_LIST }, .dwWidthGrid { 0 }, .fDialogCtrl { true } });
	m_pList->SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_pList->InsertColumn(0, L"Drive Name", 0, 200);
	m_pList->InsertColumn(1, L"Size", 0, 70);
	m_pList->InsertColumn(2, L"Path", 0, 120);
	m_pList->InsertColumn(3, L"Bus Type", 0, 100);

	auto vecDrives = GetDeviceDrives();
	std::ranges::sort(vecDrives, { }, &DEVICE_DRIVE::wstrDrivePath);

	for (const auto& [idx, drive] : vecDrives | std::views::enumerate) {
		const auto iidx = static_cast<int>(idx);
		m_pList->InsertItem(iidx, drive.wstrFriendlyName.data());
		m_pList->SetItemText(iidx, 1, drive.wstrSize.data());
		m_pList->SetItemText(iidx, 2, drive.wstrDrivePath.data());
		m_pList->SetItemText(iidx, 3, drive.wstrBusType.data());
	}

	return TRUE;
}

void CDlgOpenDrive::OnCancel()
{
	static_cast<CDialogEx*>(GetParent())->EndDialog(IDCANCEL);
}

void CDlgOpenDrive::OnListDblClick(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	if (const auto* const pNMI = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
		pNMI->iItem >= 0 && pNMI->iSubItem >= 0) {
		OnOK();
	}
}

void CDlgOpenDrive::OnListItemChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	static_cast<CDialogEx*>(GetParent())->GetDlgItem(IDOK)->EnableWindow(m_pList->GetSelectedCount() > 0);
}

void CDlgOpenDrive::OnOK()
{
	static_cast<CDialogEx*>(GetParent())->GetDlgItem(IDOK)->SendMessageW(BM_CLICK);
}

auto CDlgOpenDrive::GetDeviceDrives()->std::vector<DEVICE_DRIVE>
{
	const std::unordered_map<STORAGE_BUS_TYPE, const wchar_t*> umapBusType {
		{ BusTypeUnknown, L"Unknown" }, { BusTypeScsi, L"SCSI" }, { BusTypeAtapi, L"ATAPI" },
		{ BusTypeAta, L"ATA" }, { BusType1394, L"1394" }, { BusTypeSsa, L"SSA" },
		{ BusTypeFibre, L"Fibre Channel" }, { BusTypeUsb, L"USB" }, { BusTypeRAID, L"RAID" },
		{ BusTypeiScsi, L"iSCSI" }, { BusTypeSas, L"SAS" }, { BusTypeSata, L"SATA" },
		{ BusTypeSd, L"SD" }, { BusTypeMmc, L"MMC" }, { BusTypeVirtual, L"Virtual" },
		{ BusTypeFileBackedVirtual, L"File Backed Virtual" }, { BusTypeSpaces, L"Storage Spaces" },
		{ BusTypeNvme, L"NVMe" }, { BusTypeSCM, L"SCM" }, { BusTypeUfs, L"UFC" },
		{ BusTypeNvmeof, L"NVMeof" }, { BusTypeMax, L"MAX" }, { BusTypeMaxReserved, L"Reserved" }
	};

	const auto hDevInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE) {
		return { };
	}

	std::vector<DEVICE_DRIVE> vecRet;
	SP_DEVICE_INTERFACE_DATA stDID { .cbSize { sizeof(SP_DEVICE_INTERFACE_DATA) } };
	for (auto iMemberIndex = 0; SetupDiEnumDeviceInterfaces(hDevInfo, nullptr, &GUID_DEVINTERFACE_DISK,
		iMemberIndex, &stDID) != FALSE; ++iMemberIndex) {
		DWORD dwSize { };
		SetupDiGetDeviceInterfaceDetailW(hDevInfo, &stDID, nullptr, 0, &dwSize, nullptr);
		if (dwSize == 0) {
			continue;
		}

		const auto unpBytes = std::make_unique<std::byte[]>(dwSize);
		const auto pDIDD = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(unpBytes.get());
		pDIDD->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		SetupDiGetDeviceInterfaceDetailW(hDevInfo, &stDID, pDIDD, dwSize, nullptr, nullptr);
		DEVICE_DRIVE stDrive;

		//Get Device info: Number, Size, Bus type.
		if (const auto hDevice = CreateFileW(pDIDD->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr); hDevice != INVALID_HANDLE_VALUE) {

			//Device number.
			if (STORAGE_DEVICE_NUMBER stDevNumber { }; DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER,
				nullptr, 0, &stDevNumber, sizeof(stDevNumber), nullptr, nullptr) != FALSE) {
				stDrive.wstrDrivePath = std::format(L"\\\\.\\PhysicalDrive{}", stDevNumber.DeviceNumber);
			}

			//Device Bus type.
			STORAGE_PROPERTY_QUERY stSPQ { .PropertyId = StorageDeviceProperty, .QueryType = PropertyStandardQuery };
			STORAGE_DEVICE_DESCRIPTOR stSDD { };
			if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &stSPQ, sizeof(stSPQ), &stSDD, sizeof(stSDD),
				nullptr, nullptr) != FALSE) {
				if (umapBusType.contains(stSDD.BusType)) {
					stDrive.wstrBusType = umapBusType.at(stSDD.BusType);
				}
			}

			//Device size.
			stDrive.wstrSize = std::format(L"{:.1f} GB",
				static_cast<double>(Ut::GetDeviceSize(stDrive.wstrDrivePath.data()).value_or(0)) / 1024 / 1024 / 1024);

			CloseHandle(hDevice);
		}

		//Device pretty/friendly name.
		if (SP_DEVINFO_DATA stDD { .cbSize { sizeof(SP_DEVINFO_DATA) } };
			SetupDiEnumDeviceInfo(hDevInfo, iMemberIndex, &stDD) != FALSE) {
			wchar_t buffFrendlyName[MAX_PATH];
			if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &stDD, SPDRP_FRIENDLYNAME,
				nullptr, reinterpret_cast<PBYTE>(buffFrendlyName), MAX_PATH, nullptr) != FALSE) {
				stDrive.wstrFriendlyName = buffFrendlyName;
			}
		}

		vecRet.emplace_back(stDrive);
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);

	return vecRet;
}


//CDlgOpenVolume.

class CDlgOpenVolume final : public CDialogEx {
	struct DEVICE_VOLUME {
		std::wstring  wstrVolumePath;
		std::wstring  wstrMountPoint;
		std::wstring  wstrFileSystem;
		std::wstring  wstrLabel;
		std::wstring  wstrSize;
		std::wstring  wstrDrivePath;
		std::wstring  wstrDriveType;
	};
public:
	[[nodiscard]] auto GetOpenData()const->std::vector<Ut::DATAOPEN>;
	[[nodiscard]] bool IsOK();
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	void OnCancel()override;
	afx_msg void OnListDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void OnOK()override;
	DECLARE_MESSAGE_MAP();
	[[nodiscard]] static auto GetDeviceVolumes() -> std::vector<DEVICE_VOLUME>;
private:
	lex::IListExPtr m_pList { lex::CreateListEx() };
};

BEGIN_MESSAGE_MAP(CDlgOpenVolume, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_OPENVOLUME_LIST, &CDlgOpenVolume::OnListDblClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_OPENVOLUME_LIST, &CDlgOpenVolume::OnListItemChanged)
END_MESSAGE_MAP()

auto CDlgOpenVolume::GetOpenData()const->std::vector<Ut::DATAOPEN>
{
	std::vector<Ut::DATAOPEN> vec;
	int nItem { -1 };
	for (auto i { 0UL }; i < m_pList->GetSelectedCount(); ++i) {
		nItem = m_pList->GetNextItem(nItem, LVNI_SELECTED);
		std::wstring wstrVolPath = m_pList->GetItemText(nItem, 6).GetString(); //Volume Path column.
		if (wstrVolPath.ends_with(L'\\')) { //Remove trailing slash to satisfy CreateFileW in DataLoader.
			wstrVolPath = wstrVolPath.substr(0, wstrVolPath.size() - 1);
		}

		vec.emplace_back(Ut::DATAOPEN { .wstrDataPath { std::move(wstrVolPath) }, .eOpenMode { Ut::EOpenMode::OPEN_DEVICE } });
	}

	return vec;
}

bool CDlgOpenVolume::IsOK()
{
	return m_pList->GetSelectedCount() > 0;
}


//CDlgOpenVolume Private methods.

void CDlgOpenVolume::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CDlgOpenVolume::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(IDC_OPENVOLUME_LIST, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));

	m_pList->Create({ .pParent { this }, .uID { IDC_OPENVOLUME_LIST }, .dwWidthGrid { 0 }, .fDialogCtrl { true } });
	m_pList->SetExtendedStyle(LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_pList->InsertColumn(0, L"Mount Point", 0, 80);
	m_pList->InsertColumn(1, L"Resides In", 0, 110);
	m_pList->InsertColumn(2, L"Label", 0, 100);
	m_pList->InsertColumn(3, L"Size", 0, 70);
	m_pList->InsertColumn(4, L"Drive Type", 0, 80);
	m_pList->InsertColumn(5, L"File System", 0, 80);
	m_pList->InsertColumn(6, L"Volume Path", 0, 300);

	auto vecVolumes = GetDeviceVolumes();
	const auto rngTail = std::ranges::partition(vecVolumes, [](const DEVICE_VOLUME& ref) { return !ref.wstrMountPoint.empty(); });
	std::ranges::sort(rngTail, { }, &DEVICE_VOLUME::wstrDrivePath);
	const auto itEnd = std::ranges::partition_point(vecVolumes, [](const DEVICE_VOLUME& ref) { return !ref.wstrMountPoint.empty(); });
	std::ranges::sort(vecVolumes.begin(), itEnd, { }, &DEVICE_VOLUME::wstrMountPoint);

	for (const auto& [idx, vol] : vecVolumes | std::views::enumerate) {
		const auto iidx = static_cast<int>(idx);
		m_pList->InsertItem(iidx, vol.wstrMountPoint.data());
		m_pList->SetItemText(iidx, 1, vol.wstrDrivePath.data());
		m_pList->SetItemText(iidx, 2, vol.wstrLabel.data());
		m_pList->SetItemText(iidx, 3, vol.wstrSize.data());
		m_pList->SetItemText(iidx, 4, vol.wstrDriveType.data());
		m_pList->SetItemText(iidx, 5, vol.wstrFileSystem.data());
		m_pList->SetItemText(iidx, 6, vol.wstrVolumePath.data());
	}

	return TRUE;
}

void CDlgOpenVolume::OnCancel()
{
	static_cast<CDialogEx*>(GetParent())->EndDialog(IDCANCEL);
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
	static_cast<CDialogEx*>(GetParent())->GetDlgItem(IDOK)->EnableWindow(m_pList->GetSelectedCount() > 0);
}

void CDlgOpenVolume::OnOK()
{
	static_cast<CDialogEx*>(GetParent())->GetDlgItem(IDOK)->SendMessageW(BM_CLICK);
}

auto CDlgOpenVolume::GetDeviceVolumes()->std::vector<DEVICE_VOLUME>
{
	//This data is returned from the GetDriveTypeW.
	const std::unordered_map<UINT, const wchar_t*> umapDriveType {
		{ 0, L"Unknown" }, { 1, L"Invalid Root Path" }, { 2, L"Removable" },
		{ 3, L"Fixed" }, { 4, L"Remote" }, { 5, L"CD-ROM" }, { 6, L"RAM Disk" }
	};

	std::vector<DEVICE_VOLUME> vecRet;
	const auto hDevInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_VOLUME, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE) {
		return { };
	}

	SP_DEVICE_INTERFACE_DATA stDID { .cbSize { sizeof(SP_DEVICE_INTERFACE_DATA) } };
	for (auto iMemberIndex = 0; SetupDiEnumDeviceInterfaces(hDevInfo, nullptr, &GUID_DEVINTERFACE_VOLUME,
		iMemberIndex, &stDID) != FALSE; ++iMemberIndex) {
		DWORD dwSize { };
		SetupDiGetDeviceInterfaceDetailW(hDevInfo, &stDID, nullptr, 0, &dwSize, nullptr);
		if (dwSize == 0) {
			continue;
		}

		const auto unpBytes = std::make_unique<std::byte[]>(dwSize);
		const auto pDIDD = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(unpBytes.get());
		pDIDD->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		SetupDiGetDeviceInterfaceDetailW(hDevInfo, &stDID, pDIDD, dwSize, nullptr, nullptr);

		//pDIDD->DevicePath is a RAW Volume path.
		//To convert it to a \\?\Volume{GUID}\ form, add trailing slash first (\).
		//Only then pass it to the GetVolumeNameForVolumeMountPointW function.
		DEVICE_VOLUME stVolume { .wstrVolumePath { pDIDD->DevicePath } };
		stVolume.wstrVolumePath += '\\';
		wchar_t buffVolPath[MAX_PATH];
		GetVolumeNameForVolumeMountPointW(stVolume.wstrVolumePath.data(), buffVolPath, MAX_PATH);
		stVolume.wstrVolumePath = buffVolPath;

		wchar_t buffVolLabel[128];
		wchar_t buffFSName[128];
		GetVolumeInformationW(buffVolPath, buffVolLabel, 128, nullptr, nullptr, nullptr, buffFSName, 128);
		wchar_t buffMountPaths[MAX_PATH];
		GetVolumePathNamesForVolumeNameW(buffVolPath, buffMountPaths, MAX_PATH, nullptr);
		stVolume.wstrMountPoint = buffMountPaths;
		stVolume.wstrFileSystem = buffFSName;
		stVolume.wstrLabel = buffVolLabel;

		//Drive type.
		if (const auto uVolType = GetDriveTypeW(buffVolPath); umapDriveType.contains(uVolType)) {
			stVolume.wstrDriveType = umapDriveType.at(uVolType);
		}

		bool fwstrPath { false };
		if (stVolume.wstrVolumePath.back() == L'\\') { //DeviceIoControl doesn't work with paths ending with slash.
			stVolume.wstrVolumePath.back() = 0;        //Temporarily change trailing slash to zero.
			fwstrPath = true;
		}

		//Get disk number this Volume resides in.
		if (const auto hHandleVol = CreateFileW(stVolume.wstrVolumePath.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr, OPEN_EXISTING, 0, nullptr); hHandleVol != INVALID_HANDLE_VALUE) {
			STORAGE_DEVICE_NUMBER stSDN { };
			if (DeviceIoControl(hHandleVol, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &stSDN, sizeof(stSDN),
				nullptr, nullptr) != FALSE) {
				stVolume.wstrDrivePath = std::format(L"\\\\.\\PhysicalDrive{}", stSDN.DeviceNumber);
			}
			CloseHandle(hHandleVol);
		}

		//Volume size.
		stVolume.wstrSize = std::format(L"{:.1f} GB",
				static_cast<double>(Ut::GetDeviceSize(stVolume.wstrVolumePath.data()).value_or(0)) / 1024 / 1024 / 1024);

		if (fwstrPath) {
			stVolume.wstrVolumePath.back() = L'\\';
		}

		vecRet.emplace_back(stVolume);
	}

	return vecRet;
}


//CDlgOpenPath.

class CDlgOpenPath final : public CDialogEx {
public:
	[[nodiscard]] auto GetOpenData() -> std::vector<Ut::DATAOPEN>;
	[[nodiscard]] bool IsOK();
private:
	void DoDataExchange(CDataExchange* pDX)override;
	void OnCancel()override;
	void OnComboPathEdit();
	void OnOK()override;
	DECLARE_MESSAGE_MAP();
private:
	CComboBox m_stComboPath;
};

BEGIN_MESSAGE_MAP(CDlgOpenPath, CDialogEx)
	ON_CBN_EDITUPDATE(IDC_OPENPATH_COMBO_PATH, &CDlgOpenPath::OnComboPathEdit)
END_MESSAGE_MAP()

auto CDlgOpenPath::GetOpenData()->std::vector<Ut::DATAOPEN>
{
	if (m_stComboPath.GetWindowTextLengthW() == 0)
		return { };

	CString cstrText;
	m_stComboPath.GetWindowTextW(cstrText);

	return { Ut::DATAOPEN { .wstrDataPath { cstrText }, .eOpenMode { Ut::EOpenMode::OPEN_DEVICE } } };
}

bool CDlgOpenPath::IsOK()
{
	return m_stComboPath.GetWindowTextLengthW() > 0;
}



//CDlgOpenPath Private methods.

void CDlgOpenPath::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPENPATH_COMBO_PATH, m_stComboPath);
}

void CDlgOpenPath::OnCancel()
{
	static_cast<CDialogEx*>(GetParent())->EndDialog(IDCANCEL);
}

void CDlgOpenPath::OnComboPathEdit()
{
	static_cast<CDialogEx*>(GetParent())->GetDlgItem(IDOK)->EnableWindow(m_stComboPath.GetWindowTextLengthW() > 0);
}

void CDlgOpenPath::OnOK()
{
	static_cast<CDialogEx*>(GetParent())->GetDlgItem(IDOK)->SendMessageW(BM_CLICK);
}


//CDlgOpenDevice.

export class CDlgOpenDevice final : public CDialogEx {
public:
	CDlgOpenDevice(CWnd* pParent = nullptr) : CDialogEx(IDD_OPENDEVICE, pParent) { }
	INT_PTR DoModal(int iTab = 0);
	[[nodiscard]] auto GetOpenData()const->const std::vector<Ut::DATAOPEN>&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	afx_msg auto OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) -> HBRUSH;
	BOOL OnInitDialog()override;
	void OnOK()override;
	afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	BOOL PreTranslateMessage(MSG* pMsg)override;
	void SetCurrentTab(int iTab);
	DECLARE_MESSAGE_MAP();
private:
	CTabCtrl m_tabMain;
	CRect m_rcWnd; //Dialog rect to set minimum size in the OnGetMinMaxInfo.
	std::unique_ptr<CDlgOpenDrive> m_pDlgDrives { std::make_unique<CDlgOpenDrive>() };
	std::unique_ptr<CDlgOpenVolume> m_pDlgVolumes { std::make_unique<CDlgOpenVolume>() };
	std::unique_ptr<CDlgOpenPath> m_pDlgPath { std::make_unique<CDlgOpenPath>() };
	std::vector<Ut::DATAOPEN> m_vecOpenData;
	int m_iCurTab { }; //Current tab ID.
};


BEGIN_MESSAGE_MAP(CDlgOpenDevice, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_OPENDEVICE_TAB, &CDlgOpenDevice::OnTabSelChanged)
	ON_WM_CTLCOLOR()
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

INT_PTR CDlgOpenDevice::DoModal(int iTab)
{
	m_iCurTab = iTab;
	return CDialogEx::DoModal();
}

auto CDlgOpenDevice::GetOpenData()const->const std::vector<Ut::DATAOPEN>&
{
	return m_vecOpenData;
}


//CDlgOpenDevice Private methods.

void CDlgOpenDevice::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPENDEVICE_TAB, m_tabMain);
}

auto CDlgOpenDevice::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)->HBRUSH
{
	const auto hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_STATIC) {
		pDC->SetTextColor(RGB(0, 0, 250));
	}
	return hbr;
}

BOOL CDlgOpenDevice::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	GetWindowRect(m_rcWnd);

	m_tabMain.InsertItem(0, L"Physical Drives");
	m_tabMain.InsertItem(1, L"Volumes");
	m_tabMain.InsertItem(3, L"Path");

	CRect rcTab;
	m_tabMain.GetItemRect(0, rcTab);
	CRect rcClient;
	GetClientRect(rcClient);
	CRect rcOK;
	GetDlgItem(IDOK)->GetWindowRect(rcOK);
	ScreenToClient(rcOK);

	//Child dialogs coordinates.
	const auto iX = rcTab.left;
	const auto iY = rcTab.bottom + 1;
	const auto iWidth = rcClient.Width();
	const auto iHeight = rcOK.top - rcTab.Height() - (rcClient.bottom - rcOK.bottom);

	m_pDlgDrives->Create(IDD_OPENDEVICE_DRIVE, this);
	m_pDlgDrives->SetWindowPos(nullptr, iX, iY, iWidth, iHeight, SWP_NOZORDER | SWP_HIDEWINDOW);
	m_pDlgVolumes->Create(IDD_OPENDEVICE_VOLUME, this);
	m_pDlgVolumes->SetWindowPos(nullptr, iX, iY, iWidth, iHeight, SWP_NOZORDER | SWP_HIDEWINDOW);
	m_pDlgPath->Create(IDD_OPENDEVICE_PATH, this);
	m_pDlgPath->SetWindowPos(nullptr, iX, iY, iWidth, iHeight, SWP_NOZORDER | SWP_HIDEWINDOW);

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(m_pDlgDrives->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgVolumes->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgPath->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(IDOK, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());
	pLayout->AddItem(IDCANCEL, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());
	pLayout->AddItem(IDC_OPENDEVICE_STATIC_INFO, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());

	SetCurrentTab(m_iCurTab);

	const auto hIcon = AfxGetApp()->LoadIconW(IDR_HEXER_FRAME);
	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	return TRUE;
}

void CDlgOpenDevice::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = m_rcWnd.Width();
	lpMMI->ptMinTrackSize.y = m_rcWnd.Height();

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CDlgOpenDevice::OnOK()
{
	m_vecOpenData.clear();

	switch (m_iCurTab) {
	case 0:
		m_vecOpenData = m_pDlgDrives->GetOpenData();
		break;
	case 1:
		m_vecOpenData = m_pDlgVolumes->GetOpenData();
		break;
	case 2:
		m_vecOpenData = m_pDlgPath->GetOpenData();
		break;
	default:
		break;
	}

	if (!m_vecOpenData.empty()) {
		CDialogEx::OnOK();
	}
}

void CDlgOpenDevice::OnTabSelChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	SetCurrentTab(m_tabMain.GetCurSel());
}

BOOL CDlgOpenDevice::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		OnOK(); //To prevent triggering "Cancel" button on Enter press, when "Open" is disabled.
		return FALSE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CDlgOpenDevice::SetCurrentTab(int iTab)
{
	m_iCurTab = iTab;
	m_tabMain.SetCurSel(iTab);

	bool fEnableOK { };
	switch (iTab) {
	case 0:
		m_pDlgVolumes->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_HIDE);
		m_pDlgDrives->ShowWindow(SW_SHOW);
		fEnableOK = m_pDlgDrives->IsOK();
		break;
	case 1:
		m_pDlgDrives->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_HIDE);
		m_pDlgVolumes->ShowWindow(SW_SHOW);
		fEnableOK = m_pDlgVolumes->IsOK();
		break;
	case 2:
		m_pDlgDrives->ShowWindow(SW_HIDE);
		m_pDlgVolumes->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_SHOW);
		fEnableOK = m_pDlgPath->IsOK();
		break;
	default:
		std::unreachable();
	}

	GetDlgItem(IDOK)->EnableWindow(fEnableOK);
}