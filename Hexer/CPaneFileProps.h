/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxcontrolbars.h>
#include <span>
#include <string>
#include <vector>
import Utility;

class CPaneFileProps final : public CDockablePane
{
public:
	void UpdatePaneFileProps(const Utility::FILEPROPS& stFP);
private:
	void AdjustLayout();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP();
private:
	enum class EPropName : std::uint8_t {
		FILE_PATH = 0x1, FILE_NAME, FILE_SIZE, PAGE_SIZE, IS_WRITABLE
	};
	CMFCPropertyGridCtrl m_wndProperty;
	std::vector<CMFCPropertyGridProperty*> m_vecProps;
	CFont m_fntProperty;

};