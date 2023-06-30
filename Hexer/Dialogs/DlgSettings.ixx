module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "resource.h"
#include <afxdialogex.h>
export module DlgSettings;

export class CDlgSettings : public CDialogEx
{
public:
	CDlgSettings(CWnd* pParent = nullptr);
private:
	void DoDataExchange(CDataExchange* pDX)override;
	DECLARE_MESSAGE_MAP();
};

BEGIN_MESSAGE_MAP(CDlgSettings, CDialogEx)
END_MESSAGE_MAP()

CDlgSettings::CDlgSettings(CWnd* pParent) : CDialogEx(IDD_SETTINGS, pParent)
{
}

void CDlgSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}