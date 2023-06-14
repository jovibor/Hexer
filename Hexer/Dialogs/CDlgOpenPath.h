/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxdialogex.h>
#include <string>
#include <vector>

class CDlgOpenPath final : public CDialogEx
{
public:
	[[nodiscard]] auto GetPaths() -> std::vector<std::wstring>&;
protected:
	void DoDataExchange(CDataExchange* pDX)override;
	void OnOK()override;
	void OnCancel()override;
	void OnComboPathEdit();
	DECLARE_MESSAGE_MAP();
private:
	CComboBox m_stComboPath;
	std::vector<std::wstring> m_vecPaths; //Paths to open.
};