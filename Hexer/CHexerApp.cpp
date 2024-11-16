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
import DlgOpenProcess;
import DlgNewFile;
import DlgSettings;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CHexerApp theApp;

//CDlgAbout.
class CDlgAbout final : public CDialogEx {
public:
	explicit CDlgAbout()noexcept : CDialogEx(IDD_ABOUTBOX) { }
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

	const auto wstrDescr = std::format(L"Hexer v{}.{}.{}, based on the HexCtrl v{}.{}.{}\r\n"
		"Copyright © 2023-present Jovibor",
		Ut::HEXER_VERSION_MAJOR, Ut::HEXER_VERSION_MINOR, Ut::HEXER_VERSION_PATCH,
		HEXCTRL::HEXCTRL_VERSION_MAJOR, HEXCTRL::HEXCTRL_VERSION_MINOR, HEXCTRL::HEXCTRL_VERSION_PATCH);
	GetDlgItem(IDC_ABOUT_STATIC_VERSION)->SetWindowTextW(wstrDescr.data());
	GetDlgItem(IDC_STATIC_BUILDTIME)->SetWindowTextW(L"Built on: " STRWIDER(__DATE__) L" "  STRWIDER(__TIME__));

	return TRUE;
}


//CHexerMDTemplate.
class CHexerMDTemplate final : public CMultiDocTemplate {
public:
	CHexerMDTemplate(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass)
		: CMultiDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass) {
	}
	[[nodiscard]] auto OpenDocumentFile(const Ut::DATAOPEN& dos) -> CDocument*;
};

auto CHexerMDTemplate::OpenDocumentFile(const Ut::DATAOPEN& dos)->CDocument*
{
	//This code is copy-pasted from the original CMultiDocTemplate::OpenDocumentFile.
	//And adapted to work with the DATAOPEN struct.

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
	if (!pDocument->OnOpenDocument(dos)) {
		// user has be alerted to what failed in OnOpenDocument
		TRACE(traceAppMsg, 0, "CDocument::OnOpenDocument returned FALSE.\n");
		pFrame->DestroyWindow();
		return nullptr;
	}

	pDocument->OnDocumentEvent(CDocument::onAfterOpenDocument);
	InitialUpdateFrame(pFrame, pDocument, TRUE);
	return pDocument;
}


//CHexerDocMgr.
class CHexerDocMgr final : public CDocManager {
public:
	auto OpenDocumentCustom(LPCWSTR lpszFileName, bool fDontLNK) -> CDocument*;
	auto OpenDocumentCustom(const Ut::DATAOPEN& dos) -> CDocument*;
	auto OpenDocumentFile(LPCWSTR lpszFileName, BOOL bAddToMRU = FALSE) -> CDocument* override;
	DECLARE_DYNCREATE(CHexerDocMgr);
};

IMPLEMENT_DYNCREATE(CHexerDocMgr, CDocument)

auto CHexerDocMgr::OpenDocumentCustom(LPCWSTR lpszFileName, bool fDontLNK)->CDocument*
{
	return OpenDocumentCustom({ .wstrDataPath { fDontLNK ? lpszFileName : Ut::ResolveLNK(lpszFileName) },
		.eMode { Ut::EOpenMode::OPEN_FILE } });
}

auto CHexerDocMgr::OpenDocumentCustom(const Ut::DATAOPEN& dos)->CDocument*
{
	//This code below is copy-pasted from the original CDocManager::OpenDocumentFile.
	//We need to override this method to remove calls to AtlStrLen, AfxFullPath, AfxResolveShortcut
	//functions from the original method, because these functions can't handle paths like "\\?\PhysicalDisk".
	//Also this method is adapted to work with the DATAOPEN struct.

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
		const auto wstrUniqueDocName = CHexerDoc::GetUniqueDocName(dos);
		match = pTemplate->MatchDocType(wstrUniqueDocName.data(), pOpenDocument);
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

	return pBestTemplate->OpenDocumentFile(dos);
}

auto CHexerDocMgr::OpenDocumentFile(LPCWSTR lpszFileName, BOOL /*bAddToMRU*/)->CDocument*
{
	//This method also takes a part in the HDROP.

	return OpenDocumentCustom({ .wstrDataPath { Ut::ResolveLNK(lpszFileName) }, .eMode { Ut::EOpenMode::OPEN_FILE } });
}


//CHexerApp.
BEGIN_MESSAGE_MAP(CHexerApp, CWinAppEx)
	ON_COMMAND(IDM_FILE_NEWFILE, &CHexerApp::OnFileNewFile)
	ON_COMMAND(IDM_FILE_OPENFILE, &CHexerApp::OnFileOpenFile)
	ON_COMMAND(IDM_FILE_OPENDEVICE, &CHexerApp::OnFileOpenDevice)
	ON_COMMAND(IDM_FILE_OPENPROCESS, &CHexerApp::OnFileOpenProcess)
	ON_COMMAND(IDM_TOOLS_SETTINGS, &CHexerApp::OnToolsSettings)
	ON_COMMAND(ID_APP_ABOUT, &CHexerApp::OnAppAbout)
	ON_COMMAND_RANGE(IDM_FILE_RFL00, IDM_FILE_RFL19, &CHexerApp::OnFileRFL)
END_MESSAGE_MAP()

auto CHexerApp::GetAppSettings()->CAppSettings&
{
	return m_stAppSettings;
}

auto CHexerApp::GetClassName()const->LPCWSTR
{
	//Just a "unique" string for a Window Class name.
	return L"HexerClassUnique{78306c63-7865}";
}

void CHexerApp::NotifyTabsOnSettingsChange()
{
	auto posDocTempl = GetFirstDocTemplatePosition();
	while (posDocTempl != nullptr) {
		const auto pDocTempl = GetNextDocTemplate(posDocTempl);
		auto posDoc = pDocTempl->GetFirstDocPosition();
		while (posDoc != nullptr) {
			pDocTempl->GetNextDoc(posDoc)->UpdateAllViews(nullptr, Ut::WM_APP_SETTINGS_CHANGED);
		}
	}
}

void CHexerApp::OnFileOpenFile()
{
	const auto lmbFOD = [this]()->bool {
		CFileDialog fd(TRUE, nullptr, nullptr, OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT
			| OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NODEREFERENCELINKS, L"All files (*.*)|*.*||");
		constexpr auto dwIDChkLNK = 1UL;
		fd.AddCheckButton(dwIDChkLNK, L"Don't resolve .lnk", FALSE);
		if (fd.DoModal() != IDOK)
			return true;

		BOOL iChecked;
		fd.GetCheckButtonState(dwIDChkLNK, iChecked);
		const auto fDontLNK = iChecked == TRUE;
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
			const auto pDoc = OpenDocumentCustom(pwstrPath, fDontLNK);
			fOpened = !fOpened ? pDoc != nullptr : true;
		}
		return fOpened;
		};

	while (!lmbFOD()) { }; //If no file has been opened (in multiple selection) show the "Open File Dialog" again.
}

auto CHexerApp::OpenDocumentCustom(LPCWSTR pwszPath, bool fDontLNK)->CDocument*
{
	ENSURE_VALID(m_pDocManager);
	return static_cast<CHexerDocMgr*>(m_pDocManager)->OpenDocumentCustom(pwszPath, fDontLNK);
}

auto CHexerApp::OpenDocumentCustom(const Ut::DATAOPEN& dos)->CDocument*
{
	ENSURE_VALID(m_pDocManager);
	return static_cast<CHexerDocMgr*>(m_pDocManager)->OpenDocumentCustom(dos);
}

auto CHexerApp::OpenDocumentFile(LPCWSTR pwszPath)->CDocument*
{
	ENSURE_VALID(m_pDocManager);
	return static_cast<CHexerDocMgr*>(m_pDocManager)->OpenDocumentFile(pwszPath);
}


//CHexerApp private methods.

BOOL CHexerApp::InitInstance()
{
	CWinAppEx::InitInstance();

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew) {
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	}

	//If a file is being opened by dropping on the App's shortcut, or through Windows context menu.
	const auto fHDROP = cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen;
	HANDLE hMutex { };

	GetAppSettings().LoadSettings(Ut::GetAppName());

	if (!GetAppSettings().GetGeneralSettings().fMultipleInst) { //Single.
		//Check for the already running app instance.
		//The WaitForSingleObject call is needed for cases when multiple files are opened simultaneously,
		//like dropping on app's shortcut. In such cases the first process must have some time-room
		//to create itself, create a main window, etc...
		//Hence, we wait for the hMutex to be released at the end of the InitInstance() function in the main process.
		//Only then we are seeking for the main window, at that point it's created for sure.
		if (hMutex = CreateMutexW(nullptr, TRUE, GetClassName()); GetLastError() == ERROR_ALREADY_EXISTS) {
			WaitForSingleObject(hMutex, 2000); //Wait maximum for two sec (more than enough).
		}

		if (const auto hWnd = FindWindowExW(nullptr, nullptr, GetClassName(), nullptr); hWnd != nullptr) {
			ShowWindow(hWnd, SW_SHOWNORMAL);
			SetForegroundWindow(hWnd);

			if (fHDROP) {
				const COPYDATASTRUCT cds { .dwData { 1 }, //Just a random ID. It's checked in the CMainFrame::OnCopyData.
					.cbData { cmdInfo.m_strFileName.GetLength() * 2 + sizeof(wchar_t) }, //Size with nullterminator.
					.lpData { reinterpret_cast<PVOID>(const_cast<wchar_t*>(cmdInfo.m_strFileName.GetString())) } };
				SendMessageW(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
			}

			return FALSE;
		}
	}

	//Adjusting current process privileges to SE_DEBUG_NAME level,
	//to be able to open Processes that's blocked otherwise.
	TOKEN_PRIVILEGES tkp { .PrivilegeCount { 1 } };
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
	HANDLE hToken { };
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr, nullptr);

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
	pMainFrame->LoadFrame(IDR_HEXER_FRAME); //Also used in the CMainFrame::PreCreateWindow.
	m_pMainWnd = pMainFrame;
	Ut::SetMainWnd(pMainFrame->m_hWnd);

	const auto hBMPFile = Ut::GetHBITMAP(IDB_FILE);
	const auto hBMPDevice = Ut::GetHBITMAP(IDB_DEVICE);
	const auto hBMPProcess = Ut::GetHBITMAP(IDB_PROCESS);
	MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { hBMPFile } };
	const auto pFileMenu = pMainFrame->GetMenu()->GetSubMenu(0); //"File" sub-menu.
	pFileMenu->SetMenuItemInfoW(1, &mii, TRUE); //Icon for the "Open File..." menu.
	mii.hbmpItem = hBMPDevice;
	pFileMenu->SetMenuItemInfoW(2, &mii, TRUE); //Icon for the "Open Device..." menu.
	mii.hbmpItem = hBMPProcess;
	pFileMenu->SetMenuItemInfoW(3, &mii, TRUE); //Icon for the "Open Process..." menu.
	const auto pRFLSubMenu = pFileMenu->GetSubMenu(4); //"Recent Files List" sub-menu.
	GetAppSettings().RFLInitialize(pRFLSubMenu->m_hMenu, IDM_FILE_RFL00, hBMPFile, hBMPDevice, hBMPProcess);
	DrawMenuBar(pMainFrame->m_hWnd);

	//For Drag'n Drop to work, even in elevated mode.
	//helgeklein.com/blog/2010/03/how-to-enable-drag-and-drop-for-an-elevated-mfc-application-on-vistawindows-7/
	ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
	ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
	DragAcceptFiles(m_pMainWnd->m_hWnd, TRUE);

	//First we process App's startup options, only then ProcessShellCommand.
	//So that if we restore Last Opened Files and at the same time it was a file drop
	//on a shortcut, the file dropped will de opened in the last tab.
	const auto& refGeneral = GetAppSettings().GetGeneralSettings();
	switch (refGeneral.eStartup) {
	case CAppSettings::EStartup::DO_NOTHING:
		break;
	case CAppSettings::EStartup::SHOW_FOD:
		if (!fHDROP) { //Not showing dialog if it was a drop.
			OnFileOpenFile();
		}
		break;
	case CAppSettings::EStartup::RESTORE_LAST_OPENED:
		for (const auto& ref : GetAppSettings().GetLastOpenedList()) {
			OpenDocumentCustom(ref);
		}
		break;
	default:
		break;
	};

	if (!ProcessShellCommand(cmdInfo)) {
		return FALSE;
	}

	if (hMutex) {
		::ReleaseMutex(hMutex);
	}

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_fMainAppInSingleAppMode = true;

	return TRUE;
}

int CHexerApp::ExitInstance()
{
	if (m_fMainAppInSingleAppMode) { //To make sure it's the main app, when in the Single App mode. 
		GetAppSettings().SaveSettings();
	}

	return CWinAppEx::ExitInstance();
}

void CHexerApp::OnAppAbout()
{
	CDlgAbout aboutDlg;
	aboutDlg.DoModal();
}

void CHexerApp::OnFileNewFile()
{
	if (CDlgNewFile dlg; dlg.DoModal() == IDOK) {
		OpenDocumentCustom(dlg.GetNewFileInfo());
	}
}

void CHexerApp::OnFileOpenDevice()
{
	if (CDlgOpenDevice dlg(AfxGetMainWnd()); dlg.DoModal() == IDOK) {
		for (const auto& wstrPath : dlg.GetPaths()) {
			OpenDocumentCustom({ .wstrDataPath { wstrPath }, .eMode { Ut::EOpenMode::OPEN_DEVICE } });
		}
	}
}

void CHexerApp::OnFileOpenProcess()
{
	if (CDlgOpenProcess dlg(AfxGetMainWnd()); dlg.DoModal() == IDOK) {
		for (const auto& ref : dlg.GetProcesses()) {
			OpenDocumentCustom({ .wstrDataPath { ref.wstrProcName }, .dwProcID { ref.dwProcID },
				.eMode { Ut::EOpenMode::OPEN_PROC } });
		}
	}
}

void CHexerApp::OnToolsSettings()
{
	CDlgSettings dlg;
	dlg.DoModal(m_stAppSettings);
}

void CHexerApp::OnFileRFL(UINT uID)
{
	OpenDocumentCustom(GetAppSettings().RFLGetDataFromMenuID(uID));
}