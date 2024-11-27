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
#include <cassert>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHexerDoc, CDocument)

BEGIN_MESSAGE_MAP(CHexerDoc, CDocument)
END_MESSAGE_MAP()

void CHexerDoc::ChangeDataAccessMode(Ut::DATAACCESS stDAC)
{
	m_stDataLoader.ChangeDataAccessMode(stDAC);
}

void CHexerDoc::ChangeDataIOMode(Ut::EDataIOMode eDataIOMode)
{
	m_stDataLoader.ChangeDataIOMode(eDataIOMode);
}

auto CHexerDoc::GetCacheSize()const->DWORD
{
	return m_stDataLoader.GetCacheSize();
}

auto CHexerDoc::GetDataAccessMode()const->Ut::DATAACCESS
{
	return m_stDataLoader.GetDataAccessMode();
}

auto CHexerDoc::GetDataIOMode()const->Ut::EDataIOMode
{
	return m_stDataLoader.GetDataIOMode();
}

auto CHexerDoc::GetDataPath()const->const std::wstring&
{
	return m_wstrDataPath;
}

auto CHexerDoc::GetDataSize()const->std::uint64_t
{
	return m_stDataLoader.GetDataSize();
}

auto CHexerDoc::GetDocIcon()const->HICON
{
	return m_hDocIcon;
}

auto CHexerDoc::GetFileMMAPData()const->std::byte*
{
	return m_stDataLoader.GetFileMMAPData();
}

auto CHexerDoc::GetFileName()const->const std::wstring&
{
	return m_wstrFileName;
}

auto CHexerDoc::GetMaxVirtOffset()const->std::uint64_t
{
	return m_stDataLoader.GetMaxVirtOffset();
}

auto CHexerDoc::GetMemPageSize()const->DWORD
{
	return m_stDataLoader.GetMemPageSize();
}

auto CHexerDoc::GetOpenMode()const->Ut::EOpenMode
{
	return m_stDataLoader.GetOpenMode();
}

auto CHexerDoc::GetProcID()const->DWORD
{
	return m_stDataLoader.GetProcID();
}

auto CHexerDoc::GetVecProcMemory()const->const std::vector<MEMORY_BASIC_INFORMATION>&
{
	return m_stDataLoader.GetVecProcMemory();
}

auto CHexerDoc::GetIHexVirtData()->HEXCTRL::IHexVirtData*
{
	return m_stDataLoader.GetIHexVirtData();
}

bool CHexerDoc::IsDataAccessRWINPLACE() const
{
	const auto stDAC = GetDataAccessMode();
	return stDAC.fMutable && stDAC.eDataAccessMode == Ut::EDataAccessMode::ACCESS_INPLACE;
}

bool CHexerDoc::IsDataAccessRWSAFE()const
{
	const auto stDAC = GetDataAccessMode();
	return stDAC.fMutable && stDAC.eDataAccessMode == Ut::EDataAccessMode::ACCESS_SAFE;
}

bool CHexerDoc::IsDataAccessRO()
{
	return !IsDataAccessRW();
}

bool CHexerDoc::IsDataAccessRW()
{
	return GetDataAccessMode().fMutable;
}

bool CHexerDoc::IsDataOKForDASAFE()const
{
	return m_stDataLoader.IsDataOKForDASAFE();
}

bool CHexerDoc::IsDataWritable()const
{
	return m_stDataLoader.IsDataWritable();
}

bool CHexerDoc::IsDevice()const
{
	return m_stDataLoader.IsDevice();
}

bool CHexerDoc::IsFile()const
{
	return m_stDataLoader.IsFile();
}

bool CHexerDoc::IsProcess()const
{
	return m_stDataLoader.IsProcess();
}

bool CHexerDoc::OnOpenDocument(const Ut::DATAOPEN& dos)
{
	m_wstrDataPath = dos.wstrDataPath;
	m_wstrFileName = m_wstrDataPath.substr(m_wstrDataPath.find_last_of(L'\\') + 1); //Doc name with the .extension.
	auto& refSett = theApp.GetAppSettings();
	if (const auto expOpen = m_stDataLoader.Open(dos, refSett.GetGeneralSettings().stDAC,
		refSett.GetGeneralSettings().eDataIOMode); !expOpen) {
		refSett.RFLRemoveFromList(dos);
		const auto wstrLog = std::format(L"{} open failed: {} \r\n{}", Ut::GetWstrEOpenMode(GetOpenMode()), GetFileName(),
			Ut::GetLastErrorWstr(expOpen.error()));
		MessageBoxW(AfxGetMainWnd()->m_hWnd, wstrLog.data(), L"Opening error", MB_ICONERROR);
		Ut::Log::AddLogEntryError(wstrLog);

		return false;
	}

	refSett.RFLAddToList(dos);
	refSett.LOLAddToList(dos);
	m_strPathName = GetUniqueDocName(dos).data();
	m_bEmbedded = FALSE;
	SetTitle(GetDocTitle(dos).data());
	const auto iResID = [this]() {
		using enum Ut::EOpenMode;
		switch (GetOpenMode()) {
		case OPEN_FILE:
		case NEW_FILE:
			return IDB_FILE;
		case OPEN_DEVICE:
			return IDB_DEVICE;
		case OPEN_PROC:
			return IDB_PROCESS;
		default:
			return 0;
		}
		}();
	m_hDocIcon = Ut::HICONfromHBITMAP(Ut::GetHBITMAP(iResID));
	const auto wstrLog = std::format(L"{} opened: {} ({})", Ut::GetWstrEOpenMode(GetOpenMode()), GetFileName(),
		Ut::GetWstrDATAACCESS(GetDataAccessMode()));
	Ut::Log::AddLogEntryInfo(wstrLog);
	m_fOpened = true;

	return true;
}

void CHexerDoc::SaveDataToDisk()
{
	m_stDataLoader.SaveDataToDisk();
}


//Private methods.

auto CHexerDoc::GetMainFrame()const->CMainFrame*
{
	return static_cast<CMainFrame*>(AfxGetMainWnd());
}

BOOL CHexerDoc::OnOpenDocument(LPCWSTR lpszPathName)
{
	return OnOpenDocument({ .wstrDataPath { lpszPathName }, .eOpenMode { Ut::EOpenMode::OPEN_FILE } });
}

void CHexerDoc::OnCloseDocument()
{
	//Doing below only when closing an individual opened document, not the whole App.
	if (m_fOpened && !GetMainFrame()->IsAppClosing()) {
		std::wstring wstrInfo;
		if (IsProcess()) {
			wstrInfo = std::format(L"{} closed: {} (ID: {})",
				Ut::GetWstrEOpenMode(GetOpenMode()), GetFileName(), GetProcID());
		}
		else {
			(((wstrInfo += Ut::GetWstrEOpenMode(GetOpenMode())) += L" closed: ") += GetFileName());
		}

		Ut::Log::AddLogEntryInfo(wstrInfo);
		theApp.GetAppSettings().LOLRemoveFromList({ .wstrDataPath { GetDataPath() }, .dwProcID { GetProcID() } });
	}

	CDocument::OnCloseDocument();
}

auto CHexerDoc::GetUniqueDocName(const Ut::DATAOPEN& dos)->std::wstring
{
	if (dos.eOpenMode == Ut::EOpenMode::OPEN_PROC) {
		return std::format(L"Process: {} (ID: {})", dos.wstrDataPath, dos.dwProcID);
	}
	else {
		return dos.wstrDataPath;
	}
}

auto CHexerDoc::GetDocTitle(const Ut::DATAOPEN& dos)->std::wstring
{
	using enum Ut::EOpenMode;
	if (dos.eOpenMode == OPEN_PROC) {
		return GetUniqueDocName(dos);
	}

	const auto nName = dos.wstrDataPath.find_last_of(L'\\');
	assert(nName != std::wstring::npos);
	if (nName == std::wstring::npos) {
		return { };
	}

	switch (dos.eOpenMode) {
	case OPEN_DEVICE:
		return std::format(L"{}: {}", Ut::GetWstrEOpenMode(OPEN_DEVICE), dos.wstrDataPath.substr(nName + 1));
	default:
		return dos.wstrDataPath.substr(nName + 1);
	}
}