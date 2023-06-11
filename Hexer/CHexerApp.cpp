/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "resource.h"
#include "CHexerApp.h"
#include "CMainFrame.h"
#include "CChildFrame.h"
#include "CHexerDocMgr.h"
#include "CHexerDoc.h"
#include "CHexerView.h"
#include "CDlgOpenDevice.h"
#include <format>
import Utility;

using namespace Utility;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CHexerApp theApp;

class CAboutDlg final : public CDialogEx
{
public:
	explicit CAboutDlg()noexcept : CDialogEx(IDD_ABOUTBOX) {}
private:
	BOOL OnInitDialog()override;
	DECLARE_MESSAGE_MAP();
};

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

#define STR2WIDE(x) L##x
#define STRWIDER(x) STR2WIDE(x)

BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	const auto wstrVerHexer = std::format(L"Hexer hexadecimal editor v{}.{}.{}",
		Utility::HEXER_VERSION_MAJOR, Utility::HEXER_VERSION_MINOR, Utility::HEXER_VERSION_PATCH);
	GetDlgItem(IDC_LINK_HEXER)->SetWindowTextW(wstrVerHexer.data());

	const auto wstrVerHexCtrl = std::format(L"HexCtrl, v{}.{}.{}",
		HEXCTRL::HEXCTRL_VERSION_MAJOR, HEXCTRL::HEXCTRL_VERSION_MINOR, HEXCTRL::HEXCTRL_VERSION_PATCH);
	GetDlgItem(IDC_LINK_HEXCTRL)->SetWindowTextW(wstrVerHexCtrl.data());

	GetDlgItem(IDC_STATIC_BUILDTIME)->SetWindowTextW(L"Built on: " STRWIDER(__DATE__) L" "  STRWIDER(__TIME__));

	return TRUE;
}

//CHexerApp.

BEGIN_MESSAGE_MAP(CHexerApp, CWinAppEx)
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CHexerApp::OnFileOpen)
	ON_COMMAND(IDM_FILE_OPENDEVICE, &CHexerApp::OnFileOpenDevice)
	ON_COMMAND(ID_APP_ABOUT, &CHexerApp::OnAppAbout)
	ON_COMMAND_RANGE(IDM_FILE_RFL00, IDM_FILE_RFL19, &CHexerApp::OnFileRFL)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEW, &CHexerApp::OnUpdateFileNew)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_FILE_RFL00, IDM_FILE_RFL19, &CHexerApp::OnUpdateFileRFL)
END_MESSAGE_MAP()

void CHexerApp::AddToRFL(std::wstring_view wsvPath)
{
	m_stRFL.AddToRFL(wsvPath);
}

auto CHexerApp::GetAppSettings()->CAppSettings&
{
	return m_stAppSettings;
}

void CHexerApp::OnFileOpen()
{
	const auto lmbFOD = [this]()->bool {
		CFileDialog fd(TRUE, nullptr, nullptr, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ALLOWMULTISELECT |
			OFN_DONTADDTORECENT | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, L"All files (*.*)|*.*||");

		if (fd.DoModal() == IDOK) {
			CComPtr<IFileOpenDialog> pIFOD = fd.GetIFileOpenDialog();
			CComPtr<IShellItemArray> pResults;
			pIFOD->GetResults(&pResults);

			bool fOpened { false };
			DWORD dwCount { };
			pResults->GetCount(&dwCount);
			for (auto i = 0U; i < dwCount; ++i) {
				CComPtr<IShellItem> pItem;
				pResults->GetItemAt(i, &pItem);
				CComHeapPtr<wchar_t> pwstrPath;
				pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwstrPath);
				const auto pDoc = OpenDocumentFile(pwstrPath);
				fOpened = !fOpened ? pDoc != nullptr : true;
			}
			return fOpened;
		}
		return true;
	};

	while (!lmbFOD()) { }; //If no file has been opened (in multiple selection) show the "Open File Dialog" again.
}


//Private methods.

void CHexerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CHexerApp::OnFileOpenDevice()
{
	if (CDlgOpenDevice dlg; dlg.DoModal() == IDOK) {
		MessageBoxW(0, L"Not implemented yet (in process).", L"", MB_ICONEXCLAMATION);
		/*for (const auto& wstrPath : dlg.GetPaths()) {
			OpenDocumentFile(wstrPath.data());
		}*/
	}
}

void CHexerApp::OnFileRFL(UINT uID)
{
	OpenDocumentFile(m_stRFL.GetPathFromRFL(uID).data());
}

void CHexerApp::OnUpdateFileNew(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(FALSE);
}

void CHexerApp::OnUpdateFileRFL(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

BOOL CHexerApp::InitInstance()
{
	CWinAppEx::InitInstance();

	EnableTaskbarInteraction();
	SetRegistryKey(g_wstrAppName);
	LoadStdProfileSettings(0); //Disable default "Recent File List".
	InitTooltipManager();
	m_stAppSettings.LoadSettings(g_wstrAppName);

	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	m_pDocManager = new CHexerDocMgr;
	const auto pDocTemplate = new CMultiDocTemplate(IDR_MAINFRAME, RUNTIME_CLASS(CHexerDoc),
		RUNTIME_CLASS(CChildFrame), RUNTIME_CLASS(CHexerView));
	AddDocTemplate(pDocTemplate);

	//Main menu resource was redefined as IDR_HEXER_FRAME.
	//This stops MFC from setting two different app menus:
	//One when no child windows is opened, and one when any child window is opened.

	const auto pMainFrame = new CMainFrame;
	pMainFrame->LoadFrame(IDR_HEXER_FRAME);
	m_pMainWnd = pMainFrame;

	const auto pFileMenu = pMainFrame->GetMenu()->GetSubMenu(0); //"File" sub-menu.
	const auto pRecentFilesMenu = pFileMenu->GetSubMenu(3);      //"Recent Files" sub-menu.
	m_stRFL.Initialize(pRecentFilesMenu->m_hMenu, IDM_FILE_RFL00, &m_stAppSettings.GetRFL());
	DrawMenuBar(pMainFrame->m_hWnd);

	//For Drag'n Drop to work, even in elevated mode.
	//helgeklein.com/blog/2010/03/how-to-enable-drag-and-drop-for-an-elevated-mfc-application-on-vistawindows-7/
	ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
	ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
	DragAcceptFiles(m_pMainWnd->m_hWnd, TRUE);

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew) {
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	}

	if (!ProcessShellCommand(cmdInfo)) {
		return FALSE;
	}

	pMainFrame->HidePanes(); //By default MFC shows all Panes at startup if they were visible on app's close.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CHexerApp::ExitInstance()
{
	m_stAppSettings.SaveSettings(g_wstrAppName);

	return CWinAppEx::ExitInstance();
}