/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxcontrolbars.h>
#include "CPaneMainFrame.h"
#include <string>
#include <vector>
import Utility;

class CMainFrame final : public CMDIFrameWndEx
{
public:
	int& GetChildFramesCount();
	void HidePanes();
	void OnOpenFirstTab(); //When the first tab is opened.
	void OnCloseLastTab(); //When the last tab is closed.
	void UpdatePaneFileProps(const Utility::FILEPROPS& stFP);
	void UpdatePaneDataInterp(HWND hWnd);
	void ShowPaneDataInterp();
protected:
	BOOL PreTranslateMessage(MSG* pMsg)override;
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)override;
	BOOL OnCloseDockingPane(CDockablePane* pWnd)override;
	afx_msg BOOL OnEraseMDIClientBackground(CDC* pDC)override;
	afx_msg void OnViewCustomize();
	afx_msg void OnViewFileProps();
	afx_msg void OnViewDataInterp();
	afx_msg void OnUpdateViewFileProps(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewDataInterp(CCmdUI* pCmdUI);
	void CreateGridFileProps();
	DECLARE_DYNAMIC(CMainFrame);
	DECLARE_MESSAGE_MAP();
private:
	static auto MDIClientProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwData) -> LRESULT;
	static void MDIClientSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
	inline static CFont m_fontMDIClient;
	inline static const Utility::HIDPIINFO m_HiDPIInfo { Utility::GetHiDPIInfo() };
	CMFCToolBar m_wndToolBar;
	CWnd* m_pWndMBtnCurrDown { };
	CPaneMainFrame m_paneDataInterp;
	CPaneMainFrame m_paneFileProps;
	enum class EPropName : std::uint8_t {
		FILE_PATH = 0x1, FILE_NAME, FILE_SIZE, PAGE_SIZE, IS_WRITABLE
	};
	CMFCPropertyGridCtrl m_wndGridFileProps;
	std::vector<CMFCPropertyGridProperty*> m_vecProps;
	CFont m_fntProperty;
	int m_iChildFrames { };    //Amount of active child frames.
	bool m_fClosing { false }; //Indicates that the app is closing now, to avoid dialogs' flickering on exit.
};