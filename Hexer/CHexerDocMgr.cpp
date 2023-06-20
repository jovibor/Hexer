/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerDocMgr.h"
#include "CHexerMDTemplate.h"

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

auto CHexerDocMgr::OpenDocumentFile(const Utility::FILEOPEN& fos)->CDocument*
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