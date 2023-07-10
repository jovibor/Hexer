/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxcontrolbars.h>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
import Utility;
import HexerDockablePane;
import DlgLogInfo;

class CHexerView;
namespace HEXCTRL { class IHexCtrl; }; //Forward declarations.
class CMainFrame final : public CMDIFrameWndEx
{
public:
	void AddLogEntry(const Ut::Log::LOGDATA& stData);
	int& GetChildFramesCount();
	[[nodiscard]] bool IsPaneVisible(UINT uPaneID); //Is Pane visible even if pane's window itself is tabbed and hidden atm (not active).
	[[nodiscard]] bool IsPaneActive(UINT uPaneID);  //Is Pane itself visible atm.
	void OnChildFrameActivate();
	void OnChildFrameCloseLast(); //When the last child frame is closed.
	void OnChildFrameFirstOpen(); //When the first child frame is opened.
	void ShowPane(UINT uPaneID, bool fShow, bool fActivate);
protected:
	[[nodiscard]] auto GetHexCtrl() -> HEXCTRL::IHexCtrl*;
	[[nodiscard]] auto GetHexerView() -> CHexerView*;
	[[nodiscard]] auto GetHWNDForPane(UINT uPaneID) -> HWND;
	[[nodiscard]] bool HasChildFrame();
	void HideAllPanes();
	afx_msg auto OnAddLogEntry(WPARAM wParam, LPARAM lParam) -> LRESULT;
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)override;
	afx_msg void OnClose();
	afx_msg BOOL OnEraseMDIClientBackground(CDC* pDC)override;
	afx_msg void OnViewCustomize();
	afx_msg void OnViewRangePanes(UINT uMenuID);
	afx_msg void OnUpdateRangePanes(CCmdUI* pCmdUI);
	[[nodiscard]] auto PaneIDToPtr(UINT uPaneID) -> CHexerDockablePane*;
	BOOL PreTranslateMessage(MSG* pMsg)override;
	void SavePanesSettings();
	DECLARE_DYNAMIC(CMainFrame);
	DECLARE_MESSAGE_MAP();
private:
	static auto MDIClientProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwData) -> LRESULT;
	static void MDIClientSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
	inline static CFont m_fontMDIClient;
	CMFCToolBar m_wndToolBar;
	CWnd* m_pWndMBtnCurrDown { };
	CHexerDockablePane m_paneFileProps;
	CHexerDockablePane m_paneDataInterp;
	CHexerDockablePane m_paneTemplMgr;
	CHexerDockablePane m_paneLogInfo;
	CDlgLogInfo m_dlgLogInfo;
	int m_iChildFrames { };    //Amount of active child frames.
	bool m_fClosing { false }; //Indicates that the app is closing now.
};