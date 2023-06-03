#pragma once
#include <afxcontrolbars.h>
#include "CPaneFileProps.h"
#include <span>
#include <string>
import Utility;

class CMainFrame final : public CMDIFrameWndEx
{
public:
	int& GetChildFramesCount();
	void HidePanes();
	void OnOpenFirstTab(); //When the first tab is opened.
	void OnCloseLastTab(); //When the last tab is closed.
	void UpdatePaneFileProps(const Utility::FILEPROPS& stFP);
protected:
	BOOL PreTranslateMessage(MSG* pMsg)override;
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)override;
	afx_msg BOOL OnEraseMDIClientBackground(CDC* pDC)override;
	afx_msg void OnViewCustomize();
	afx_msg void OnViewProperties();
	afx_msg void OnUpdateViewProperties(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileNew(CCmdUI* pCmdUI);
	BOOL OnCloseDockingPane(CDockablePane* pWnd)override;
	DECLARE_DYNAMIC(CMainFrame);
	DECLARE_MESSAGE_MAP();
private:
	static auto MDIClientProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwData) -> LRESULT;
	static void MDIClientSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
	inline static CFont m_fontMDIClient;
	inline static int m_iLOGPIXELSY { };
	CPaneFileProps m_wndFileProps;
	CMFCToolBar m_wndToolBar;
	CWnd* m_pWndMBtnCurrDown { };
	int m_iChildFrames { };    //Amount of active child frames.
	bool m_fClosing { false }; //Indicates that the app is closing now, to avoid dialogs' flickering on exit.
};