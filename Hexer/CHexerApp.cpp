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
import Utility;

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

	const auto wstrDescr = std::format(L"Hexer Editor v{}.{}.{}, based on the HexCtrl v{}.{}.{}\r\n"
		"Copyright © 2023-2024 Jovibor",
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

	pDocument->OnDocumentEvent(CDocument::onAfterOpenDocument);
	InitialUpdateFrame(pFrame, pDocument, TRUE);
	return pDocument;
}


//CHexerDocMgr.
class CHexerDocMgr final : public CDocManager {
public:
	auto OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU) -> CDocument* override;
	auto OpenDocumentFile(Ut::FILEOPEN& fos) -> CDocument*;
	DECLARE_DYNCREATE(CHexerDocMgr);
};

IMPLEMENT_DYNCREATE(CHexerDocMgr, CDocument)

auto CHexerDocMgr::OpenDocumentFile(LPCTSTR lpszFileName, BOOL /*bAddToMRU*/)->CDocument*
{
	//This method also takes a part in a HDROP.

	Ut::FILEOPEN fos { .eMode { Ut::EOpenMode::OPEN_FILE }, .wstrFilePath { lpszFileName } };
	return OpenDocumentFile(fos);
}

auto CHexerDocMgr::OpenDocumentFile(Ut::FILEOPEN& fos)->CDocument*
{
	//Lambda to resolve .lnk files.
	const auto lmbResolveLNK = [](const wchar_t* pwszPath)->std::wstring {
		if (!std::wstring_view(pwszPath).ends_with(L".lnk")) {
			return pwszPath; //If it's not a `.lnk`, just return the path as is.
		}

		CComPtr<IShellLinkW> pIShellLinkW;
		pIShellLinkW.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER);
		CComPtr<IPersistFile> pIPersistFile;
		pIShellLinkW->QueryInterface(IID_PPV_ARGS(&pIPersistFile));
		pIPersistFile->Load(pwszPath, STGM_READ);

		std::wstring wstrPath;
		wstrPath.resize_and_overwrite(MAX_PATH, [pIShellLinkW = pIShellLinkW](wchar_t* pData, std::size_t sSize) {
			pIShellLinkW->GetPath(pData, static_cast<int>(sSize), nullptr, 0);
			return sSize; });
		wstrPath.resize(wstrPath.find_first_of(L'\0')); //Resize to the actual data size.

		return wstrPath;
		};
	if (fos.eMode != Ut::EOpenMode::NEW_FILE && fos.eMode != Ut::EOpenMode::OPEN_PROC) {
		fos.wstrFilePath = lmbResolveLNK(fos.wstrFilePath.data());
	}

	//This code below is copy-pasted from the original CDocManager::OpenDocumentFile.
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
		const auto wstrUniqueDocName = CHexerDoc::GetUniqueDocName(fos);
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

	return pBestTemplate->OpenDocumentFile(fos);
}


//CHexerApp.
BEGIN_MESSAGE_MAP(CHexerApp, CWinAppEx)
	ON_COMMAND(ID_FILE_NEW, &CHexerApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CHexerApp::OnFileOpen)
	ON_COMMAND(ID_APP_ABOUT, &CHexerApp::OnAppAbout)
	ON_COMMAND(IDM_FILE_OPENDEVICE, &CHexerApp::OnFileOpenDevice)
	ON_COMMAND(IDM_FILE_OPENPROCESS, &CHexerApp::OnFileOpenProcess)
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

auto CHexerApp::GetClassName()const->LPCWSTR
{
	//Just a "unique" string for a Window Class name.
	return L"HexerClassUnique{78306c63-7865}";
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
				Ut::FILEOPEN fos { .eMode { Ut::EOpenMode::OPEN_FILE }, .wstrFilePath { pwstrPath } };
				const auto pDoc = OpenDocumentFile(fos);
				fOpened = !fOpened ? pDoc != nullptr : true;
			}
			return fOpened;
		}
		return true;
		};

	while (!lmbFOD()) { }; //If no file has been opened (in multiple selection) show the "Open File Dialog" again.
}

auto CHexerApp::OpenDocumentFile(Ut::FILEOPEN& fos)->CDocument*
{
	ENSURE_VALID(m_pDocManager);
	return static_cast<CHexerDocMgr*>(m_pDocManager)->OpenDocumentFile(fos);
}


//CHexerApp private methods.

BOOL CHexerApp::InitInstance()
{
	CWinAppEx::InitInstance();

	//Adjusting current process privileges to SE_DEBUG_NAME level,
	//to be able to open Processes that's blocked otherwise.
	TOKEN_PRIVILEGES tkp { .PrivilegeCount { 1 } };
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
	HANDLE hToken { };
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr, nullptr);

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

	const auto iSizeIcon = static_cast<int>(16 * Ut::GetHiDPIInfo().flDPIScale);
	const auto hBMPDisk = static_cast<HBITMAP>(LoadImageW(AfxGetInstanceHandle(), MAKEINTRESOURCEW(IDB_OPENDEVICE), IMAGE_BITMAP,
		iSizeIcon, iSizeIcon, LR_CREATEDIBSECTION));

	MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP }, .hbmpItem { hBMPDisk } };
	const auto pFileMenu = pMainFrame->GetMenu()->GetSubMenu(0); //"File" sub-menu.
	pFileMenu->SetMenuItemInfoW(2, &mii, TRUE); //Setting the icon for the "Open Device..." menu.
	const auto pRFSubMenu = pFileMenu->GetSubMenu(4); //"Recent Files" sub-menu.
	GetAppSettings().RFLInitialize(pRFSubMenu->m_hMenu, IDM_FILE_RFL00, hBMPDisk, GetAppSettings().GetGeneralSettings().dwRFLSize);
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
			OnFileOpen();
		}
		break;
	case CAppSettings::EStartup::RESTORE_LAST_OPENED:
		for (const auto& wstr : GetAppSettings().GetLastOpenedFromReg()) {
			Ut::FILEOPEN fos { .eMode { Ut::EOpenMode::OPEN_FILE }, .wstrFilePath { wstr } };
			OpenDocumentFile(fos);
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

	return TRUE;
}

int CHexerApp::ExitInstance()
{
	GetAppSettings().SaveSettings(Ut::GetAppName());

	return CWinAppEx::ExitInstance();
}

void CHexerApp::OnAppAbout()
{
	CDlgAbout aboutDlg;
	aboutDlg.DoModal();
}

void CHexerApp::OnFileNew()
{
	if (CDlgNewFile dlg; dlg.DoModal() == IDOK) {
		auto fos = dlg.GetNewFileInfo();
		OpenDocumentFile(fos);
	}
}

void CHexerApp::OnFileOpenDevice()
{
	if (CDlgOpenDevice dlg(AfxGetMainWnd()); dlg.DoModal() == IDOK) {
		for (const auto& wstrPath : dlg.GetPaths()) {
			Ut::FILEOPEN fos { .eMode { Ut::EOpenMode::OPEN_DEVICE }, .wstrFilePath { wstrPath } };
			OpenDocumentFile(fos);
		}
	}
}

void CHexerApp::OnFileOpenProcess()
{
	if (CDlgOpenProcess dlg(AfxGetMainWnd()); dlg.DoModal() == IDOK) {
		for (const auto& ref : dlg.GetProcesses()) {
			Ut::FILEOPEN fos { .eMode { Ut::EOpenMode::OPEN_PROC }, .wstrFilePath { ref.wstrProcName },
				.dwProcID { ref.dwProcID } };
			OpenDocumentFile(fos);
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
			pDocTempl->GetNextDoc(posDoc)->UpdateAllViews(nullptr, Ut::WM_APP_SETTINGS_CHANGED);
		}
	}

	m_stAppSettings.OnSettingsChanged();
}

void CHexerApp::OnFileRFL(UINT uID)
{
	Ut::FILEOPEN fos { .eMode { Ut::EOpenMode::OPEN_FILE }, .wstrFilePath { GetAppSettings().RFLGetPathFromID(uID) } };
	OpenDocumentFile(fos);
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