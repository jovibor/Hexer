/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxcontrolbars.h>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

import DlgFileInfo;
import DlgLogInfo;
import HexerDockablePane;
import Utility;

class CHexerView;
namespace HEXCTRL { class IHexCtrl; }; //Forward declarations.
class CMainFrame final : public CMDIFrameWndEx {
public:
	void AddLogEntry(const Ut::Log::LOGINFO& stData);
	[[nodiscard]] int& GetChildFramesCount();
	[[nodiscard]] bool IsAppClosing()const;
	[[nodiscard]] bool IsPaneVisible(UINT uPaneID); //Is Pane visible even if pane's window itself is tabbed and hidden atm (not active).
	[[nodiscard]] bool IsPaneActive(UINT uPaneID);  //Is Pane itself visible atm.
	void OnChildFrameActivate();
	void OnChildFrameCloseLast(); //When the last child frame is closed.
	void OnChildFrameOpenFirst(); //When the first child frame is opened.
	void ShowPane(UINT uPaneID, bool fShow, bool fActivate);
	void UpdatePaneFileInfo();
protected:
	[[nodiscard]] auto GetHexCtrl() -> HEXCTRL::IHexCtrl*;
	[[nodiscard]] auto GetHexerView() -> CHexerView*;
	[[nodiscard]] auto GetHWNDForPane(UINT uPaneID) -> HWND;
	[[nodiscard]] auto GetPanesMap() -> const std::unordered_map<UINT, CHexerDockablePane*>&;
	[[nodiscard]] auto GetPtrFromPaneID(UINT uPaneID) -> CHexerDockablePane*;
	[[nodiscard]] bool HasChildFrame();
	void HideAllPanes();
	afx_msg auto OnAddLogEntry(WPARAM wParam, LPARAM lParam) -> LRESULT;
	afx_msg void OnClose();
	BOOL OnCloseDockingPane(CDockablePane* pPane)override;
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)override;
	afx_msg BOOL OnEraseMDIClientBackground(CDC* pDC)override;
	afx_msg void OnUpdateRangePanes(CCmdUI* pCmdUI);
	afx_msg void OnViewCustomize();
	afx_msg void OnViewRangePanes(UINT uMenuID);
	BOOL PreCreateWindow(CREATESTRUCT& cs)override;
	BOOL PreTranslateMessage(MSG* pMsg)override;
	void SaveHexCtrlSettings();
	void SavePaneData(UINT uPaneID);
	void SavePanesSettings();
	DECLARE_DYNAMIC(CMainFrame);
	DECLARE_MESSAGE_MAP();
private:
	static auto MDIClientProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwData) -> LRESULT;
	static void MDIClientSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
	inline static CFont m_fontMDIClient;
	CMFCToolBar m_wndToolBar;
	CWnd* m_pWndMBtnCurrDown { };
	CHexerDockablePane m_paneFileInfo;
	CHexerDockablePane m_paneBkmMgr;
	CHexerDockablePane m_paneDataInterp;
	CHexerDockablePane m_paneTemplMgr;
	CHexerDockablePane m_paneLogInfo;
	CDlgFileInfo m_dlgFileInfo;
	CDlgLogInfo m_dlgLogInfo;
	int m_iChildFrames { };    //Amount of active child frames.
	bool m_fClosing { false }; //Indicates that the app is closing now.
};