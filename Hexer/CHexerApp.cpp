/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "resource.h"
#include "CHexerApp.h"
#include "CMainFrame.h"
#include "CChildFrame.h"
#include "CHexerDoc.h"
#include "CHexerView.h"
#include <afxdialogex.h>
#include <format>
import DlgOpenDevice;
import DlgNewFile;
import DlgSettings;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CHexerApp theApp;

//CDlgAbout.
class CDlgAbout final : public CDialogEx {
public:
	explicit CDlgAbout()noexcept : CDialogEx(IDD_ABOUTBOX) {}
private:
	BOOL OnInitDialog()override;
	DECLARE_MESSAGE_MAP();
};

BEGIN_MESSAGE_MAP(CDlgAbout, CDialogEx)
END_MESSAGE_MAP()

#define STR2WIDE(x) L##x
#define STRWIDER(x) STR2WIDE(x)

BOOL CDlgAbout::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	const auto wstrVerHexer = std::format(L"Hexer Hexadecimal Editor v{}.{}.{}",
		Ut::HEXER_VERSION_MAJOR, Ut::HEXER_VERSION_MINOR, Ut::HEXER_VERSION_PATCH);
	GetDlgItem(IDC_LINK_HEXER)->SetWindowTextW(wstrVerHexer.data());
	const auto wstrVerHexCtrl = std::format(L"HexCtrl v{}.{}.{}",
		HEXCTRL::HEXCTRL_VERSION_MAJOR, HEXCTRL::HEXCTRL_VERSION_MINOR, HEXCTRL::HEXCTRL_VERSION_PATCH);
	GetDlgItem(IDC_LINK_HEXCTRL)->SetWindowTextW(wstrVerHexCtrl.data());
	GetDlgItem(IDC_STATIC_BUILDTIME)->SetWindowTextW(L"Built on: " STRWIDER(__DATE__) L" "  STRWIDER(__TIME__));

	return TRUE;
}


//CHexerMDTemplate.
class CHexerMDTemplate final : public CMultiDocTemplate {
public:
	CHexerMDTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass)
		: CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {}
	[[nodiscard]] auto OpenDocumentFile(const Ut::FILEOPEN& fos) -> CDocument*;
};

auto CHexerMDTemplate::OpenDocumentFile(const Ut::FILEOPEN& fos)->CDocument*
{
	//This code is copy-pasted from the original CMultiDocTemplate::OpenDocumentFile.
	//And adapted to work with the FILEOPEN struct.

	auto pDocument = static_cast<CHexerDoc*>(CreateNewDocument());
	if (pDocument == nullptr) {
		TRACE(traceAppMsg, 0, "CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return nullptr;
	}
	ASSERT_VALID(pDocument);

	const auto bAutoDelete = pDocument->m_bAutoDelete;
	pDocument->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
	CFrameWnd* pFrame = CreateNewFrame(pDocument, nullptr);
	pDocument->m_bAutoDelete = bAutoDelete;
	if (pFrame == nullptr) {
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		delete pDocument;       // explicit delete on error
		return nullptr;
	}
	ASSERT_VALID(pFrame);

	const CWaitCursor wait;
	if (!pDocument->OnOpenDocument(fos)) {
		// user has be alerted to what failed in OnOpenDocument
		TRACE(traceAppMsg, 0, "CDocument::OnOpenDocument returned FALSE.\n");
		pFrame->DestroyWindow();
		return nullptr;
	}
	pDocument->SetPathName(fos.wstrFilePath.data(), FALSE);
	pDocument->OnDocumentEvent(CDocument::onAfterOpenDocument);

	InitialUpdateFrame(pFrame, pDocument, TRUE);
	return pDocument;
}


//CHexerDocMgr.
class CHexerDocMgr final : public CDocManager {
public:
	auto OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU) -> CDocument* override;
	auto OpenDocumentFile(const Ut::FILEOPEN& fos) -> CDocument*;
	DECLARE_DYNCREATE(CHexerDocMgr);
};

IMPLEMENT_DYNCREATE(CHexerDocMgr, CDocument)

auto CHexerDocMgr::OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU)->CDocument*
{
	//This code is copy-pasted from the original CDocManager::OpenDocumentFile.
	//We need to override this method to remove calls to AtlStrLen, AfxFullPath, AfxResolveShortcut
	//functions from the original method, because these functions can't handle paths like "\\?\PhysicalDisk".
	//This method also takes a part in HDROP.

	if (lpszFileName == nullptr) {
		AfxThrowInvalidArgException();
	}
	// find the highest confidence
	POSITION pos = m_templateList.GetHeadPosition();
	CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
	CDocTemplate* pBestTemplate = nullptr;
	CDocument* pOpenDocument = nullptr;

	while (pos != nullptr) {
		const auto pTemplate = static_cast<CDocTemplate*>(m_templateList.GetNext(pos));
		ASSERT_KINDOF(CDocTemplate, pTemplate);

		CDocTemplate::Confidence match;
		ASSERT(pOpenDocument == nullptr);
		match = pTemplate->MatchDocType(lpszFileName, pOpenDocument);
		if (match > bestMatch) {
			bestMatch = match;
			pBestTemplate = pTemplate;
		}
		if (match == CDocTemplate::yesAlreadyOpen)
			break;      // stop here
	}

	if (pOpenDocument != nullptr) {
		auto posOpenDoc = pOpenDocument->GetFirstViewPosition();
		if (posOpenDoc != nullptr) {
			const auto pView = pOpenDocument->GetNextView(posOpenDoc); // get first one
			ASSERT_VALID(pView);
			const auto pFrame = pView->GetParentFrame();

			if (pFrame == nullptr) {
				TRACE(traceAppMsg, 0, "Error: Can not find a frame for document to activate.\n");
			}
			else {
				pFrame->ActivateFrame();

				if (pFrame->GetParent() != nullptr) {
					if (const auto pAppFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd); pFrame != pAppFrame) {
						ASSERT_KINDOF(CFrameWnd, pAppFrame);
						pAppFrame->ActivateFrame();
					}
				}
			}
		}
		else {
			TRACE(traceAppMsg, 0, "Error: Can not find a view for document to activate.\n");
		}

		return pOpenDocument;
	}

	if (pBestTemplate == nullptr) {
		AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
		return nullptr;
	}

	return pBestTemplate->OpenDocumentFile(lpszFileName, bAddToMRU, TRUE);
}

auto CHexerDocMgr::OpenDocumentFile(const Ut::FILEOPEN& fos)->CDocument*
{
	//This code is copy-pasted from the original CDocManager::OpenDocumentFile.
	//We need to override this method to remove calls to AtlStrLen, AfxFullPath, AfxResolveShortcut
	//functions from the original method, because these functions can't handle paths like "\\?\PhysicalDisk".
	//Also this method is adapted to work with the FILEOPEN struct.

	// find the highest confidence
	POSITION pos = m_templateList.GetHeadPosition();
	CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
	CHexerMDTemplate* pBestTemplate = nullptr;
	CDocument* pOpenDocument = nullptr;

	while (pos != nullptr) {
		auto pTemplate = static_cast<CHexerMDTemplate*>(m_templateList.GetNext(pos));
		ASSERT_KINDOF(CDocTemplate, pTemplate);

		CDocTemplate::Confidence match;
		ASSERT(pOpenDocument == nullptr);
		match = pTemplate->MatchDocType(fos.wstrFilePath.data(), pOpenDocument);
		if (match > bestMatch) {
			bestMatch = match;
			pBestTemplate = pTemplate;
		}
		if (match == CDocTemplate::yesAlreadyOpen)
			break;      // stop here
	}

	if (pOpenDocument != nullptr) {
		auto posOpenDoc = pOpenDocument->GetFirstViewPosition();
		if (posOpenDoc != nullptr) {
			const auto pView = pOpenDocument->GetNextView(posOpenDoc); // get first one
			ASSERT_VALID(pView);
			const auto pFrame = pView->GetParentFrame();

			if (pFrame == nullptr) {
				TRACE(traceAppMsg, 0, "Error: Can not find a frame for document to activate.\n");
			}
			else {
				pFrame->ActivateFrame();

				if (pFrame->GetParent() != nullptr) {
					if (const auto pAppFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd); pFrame != pAppFrame) {
						ASSERT_KINDOF(CFrameWnd, pAppFrame);
						pAppFrame->ActivateFrame();
					}
				}
			}
		}
		else {
			TRACE(traceAppMsg, 0, "Error: Can not find a view for document to activate.\n");
		}

		return pOpenDocument;
	}

	if (pBestTemplate == nullptr) {
		AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
		return nullptr;
	}

	return pBestTemplate->OpenDocumentFile(fos);
}


//CHexerApp.
BEGIN_MESSAGE_MAP(CHexerApp, CWinAppEx)
	ON_COMMAND(ID_FILE_NEW, &CHexerApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CHexerApp::OnFileOpen)
	ON_COMMAND(ID_APP_ABOUT, &CHexerApp::OnAppAbout)
	ON_COMMAND(IDM_FILE_OPENDEVICE, &CHexerApp::OnFileOpenDevice)
	ON_COMMAND(IDM_TOOLS_SETTINGS, &CHexerApp::OnToolsSettings)
	ON_COMMAND_RANGE(IDM_FILE_RFL00, IDM_FILE_RFL19, &CHexerApp::OnFileRFL)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEW, &CHexerApp::OnUpdateFileNew)
	ON_UPDATE_COMMAND_UI(IDM_TOOLS_SETTINGS, &CHexerApp::OnUpdateToolsSettings)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_FILE_RFL00, IDM_FILE_RFL19, &CHexerApp::OnUpdateFileRFL)
END_MESSAGE_MAP()

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
				const auto pDoc = OpenDocumentFile(Ut::FILEOPEN { .wstrFilePath { pwstrPath }, .fNewFile { false } });
				fOpened = !fOpened ? pDoc != nullptr : true;
			}
			return fOpened;
		}
		return true;
		};

	while (!lmbFOD()) { }; //If no file has been opened (in multiple selection) show the "Open File Dialog" again.
}


//CHexerApp private methods.

BOOL CHexerApp::InitInstance()
{
	CWinAppEx::InitInstance();

	EnableTaskbarInteraction(FALSE);
	SetRegistryKey(Ut::GetAppName().data());
	InitTooltipManager();

	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	m_pDocManager = new CHexerDocMgr;
	const auto pDocTemplate = new CHexerMDTemplate(IDR_MAINFRAME, RUNTIME_CLASS(CHexerDoc),
		RUNTIME_CLASS(CChildFrame), RUNTIME_CLASS(CHexerView));
	AddDocTemplate(pDocTemplate);

	//Main menu resource was redefined as IDR_HEXER_FRAME.
	//This stops MFC from setting two different app menus:
	//One when no child windows is opened, and one when any child window is opened.

	const auto pMainFrame = new CMainFrame;
	pMainFrame->LoadFrame(IDR_HEXER_FRAME);
	m_pMainWnd = pMainFrame;

	const auto iSizeIcon = static_cast<int>(16 * Ut::GetHiDPIInfo().flDPIScale);
	const auto hBMPDisk = static_cast<HBITMAP>(LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDB_OPENDEVICE), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));

	MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { hBMPDisk } };
	const auto pFileMenu = pMainFrame->GetMenu()->GetSubMenu(0); //"File" sub-menu.
	pFileMenu->SetMenuItemInfoW(2, &mii, TRUE); //Setting the icon for the "Open Device..." menu.
	const auto pRFSubMenu = pFileMenu->GetSubMenu(3); //"Recent Files" sub-menu.
	GetAppSettings().LoadSettings(Ut::GetAppName());
	GetAppSettings().RFLInitialize(pRFSubMenu->m_hMenu, IDM_FILE_RFL00, hBMPDisk, GetAppSettings().GetGeneralSettings().dwRFLSize);
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

	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	const auto& refGeneral = GetAppSettings().GetGeneralSettings();
	switch (refGeneral.eStartup) {
	case CAppSettings::EStartup::DO_NOTHING:
		break;
	case CAppSettings::EStartup::SHOW_FOD:
		OnFileOpen();
		break;
	case CAppSettings::EStartup::RESTORE_LAST_OPENED:
		for (const auto& wstr : GetAppSettings().GetLastOpenedFromReg()) {
			OpenDocumentFile(Ut::FILEOPEN { .wstrFilePath { wstr }, .fNewFile { false } });
		}
		break;
	default:
		break;
	};

	return TRUE;
}

int CHexerApp::ExitInstance()
{
	GetAppSettings().SaveSettings(Ut::GetAppName());

	return CWinAppEx::ExitInstance();
}

auto CHexerApp::OpenDocumentFile(const Ut::FILEOPEN& fos)->CDocument*
{
	ENSURE_VALID(m_pDocManager);
	return static_cast<CHexerDocMgr*>(m_pDocManager)->OpenDocumentFile(fos);
}

void CHexerApp::OnAppAbout()
{
	CDlgAbout aboutDlg;
	aboutDlg.DoModal();
}

void CHexerApp::OnFileNew()
{
	if (CDlgNewFile dlg; dlg.DoModal() == IDOK) {
		OpenDocumentFile(dlg.GetNewFileInfo());
	}
}

void CHexerApp::OnFileOpenDevice()
{
	if (CDlgOpenDevice dlg(AfxGetMainWnd()); dlg.DoModal() == IDOK) {
		for (const auto& wstrPath : dlg.GetPaths()) {
			OpenDocumentFile({ .wstrFilePath { wstrPath }, .fNewFile { false } });
		}
	}
}

void CHexerApp::OnToolsSettings()
{
	CDlgSettings dlg;
	if (dlg.DoModal(m_stAppSettings) != IDOK)
		return;

	auto posDocTempl = GetFirstDocTemplatePosition();
	while (posDocTempl != nullptr) {
		const auto pDocTempl = GetNextDocTemplate(posDocTempl);
		auto posDoc = pDocTempl->GetFirstDocPosition();
		while (posDoc != nullptr) {
			pDocTempl->GetNextDoc(posDoc)->UpdateAllViews(nullptr, MSG_APP_SETTINGS_CHANGED);
		}
	}
}

void CHexerApp::OnFileRFL(UINT uID)
{
	OpenDocumentFile({ .wstrFilePath { GetAppSettings().RFLGetPathFromID(uID) }, .fNewFile { false } });
}

void CHexerApp::OnUpdateFileNew(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CHexerApp::OnUpdateFileRFL(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CHexerApp::OnUpdateToolsSettings(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}